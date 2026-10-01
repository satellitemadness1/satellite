// satellite/bytecode/program_calls.cpp -- the header says what these are for, the author's words
// for each, and which choices are mine. A fresh reader found twelve defects in the first version
// (2026-10-01); each fix says where it is made what it answers.

#include "program_calls.hpp"

#include "word_codes.hpp"
#include "../display/printing_satellite.hpp"
#include "../machine/console_lock.hpp"
#include "../machine/critical_report.hpp"
#include "../machine/s_codes.hpp"
#include "../machine/thread_stop.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_variable_program/program_spawn.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace satellite004 {
namespace {

constexpr int kQuietBeforeAHalfLine = 50;                       // ms: a prompt with no end yet is shown then
constexpr std::size_t kLongestLineHeld = 64 * 1024;             // a longer line is shown in pieces this size
constexpr auto kStopGrace = std::chrono::seconds(5);            // SIGTERM, this long, then SIGKILL
constexpr auto kJoinLooksUp = std::chrono::milliseconds(100);   // how often a waiting join() looks up

// THE END-OF-RUN LIST: every program with a run started that no join() has finished with yet.
// `listed` on each program is kept under this list's lock.
struct Unjoined {
    std::mutex lock;
    std::vector<ProgramHandle> programs;
};

Unjoined &unjoined()
{
    static Unjoined one;
    return one;
}

// EVERY WATCHER NOT JOINED YET. A watcher outlives its program's run while something the program
// left running still writes to the pipe, so watchers are joined from here, apart from the runs.
struct WatcherThread {
    pthread_t id;
    ProgramHandle program;
    std::shared_ptr<std::atomic<bool>> finished;
};

struct Watchers {
    std::mutex lock;
    std::vector<WatcherThread> threads;
};

Watchers &watchers()
{
    static Watchers one;
    return one;
}

// THE RUN IS CLOSING: readable from the moment close_every_program begins until it has joined
// every watcher, so a watcher still showing what a program left running stops there.
int closing_fd()
{
    static const int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    return fd;
}

// WHAT A WATCHER IS HANDED.
struct Watch {
    ProgramHandle program;
    std::uint64_t run;                          // which run of it this is
    int out;                                    // the read end of its output
    int pidfd;
    pid_t pid;
    std::shared_ptr<std::atomic<bool>> finished;
};

// A PIECE OF THE PROGRAM'S OUTPUT, onto the screen in turn with satl's own lines -- through the
// printing satellite's ring, as satl's own words go (display/printing_satellite.hpp).
void to_the_screen(std::string &piece)
{
    if (piece.empty())
        return;
    const ConsoleHold one_piece;
    display_bytes(std::move(piece));
    piece.clear();
}

// WHAT HAS COME, ONTO THE SCREEN UP TO ITS LAST END OF LINE. Only the new bytes are searched, so a
// line with no end costs what its bytes cost (the review: an 80 MB line took 20.8 s searched from
// its start every time), and one longer than kLongestLineHeld goes in pieces of that size.
void new_output(std::string &waiting, const char *bytes, std::size_t size)
{
    const void *last = memrchr(bytes, '\n', size);
    if (last == nullptr) {
        waiting.append(bytes, size);
    } else {
        const std::size_t through = static_cast<std::size_t>(static_cast<const char *>(last) - bytes) + 1;
        waiting.append(bytes, through);
        to_the_screen(waiting);
        waiting.assign(bytes + through, size - through);
    }
    if (waiting.size() >= kLongestLineHeld)
        to_the_screen(waiting);
}

// THE PROGRAM'S OUTPUT WHILE IT RUNS. Answers whether the pipe is still open after it has ended --
// something it started may hold the pipe for longer than it lives.
bool read_while_it_runs(const Watch &watch, std::string &waiting, std::string &chunk)
{
    for (;;) {
        pollfd ready[2] = {{watch.out, POLLIN, 0}, {watch.pidfd, POLLIN, 0}};
        const nfds_t count = watch.pidfd >= 0 ? 2 : 1;
        const int seen = poll(ready, count, waiting.empty() ? -1 : kQuietBeforeAHalfLine);
        if (seen < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }
        if (seen == 0) {
            to_the_screen(waiting);   // quiet, with a line half written: a prompt waiting for an answer
            continue;
        }
        if (ready[0].revents != 0) {
            const ssize_t got = read(watch.out, chunk.data(), chunk.size());
            if (got > 0)
                new_output(waiting, chunk.data(), static_cast<std::size_t>(got));
            else if (got == 0 || (errno != EINTR && errno != EAGAIN))
                return false;   // every writer has closed it
        }
        // IT HAS ENDED. What is in the pipe NOW is read -- exactly that much, so a writer it left
        // behind cannot keep this here (the review: a drain read forty pipes' worth after sh had gone).
        if (count == 2 && (ready[1].revents & POLLIN) != 0) {
            int in_the_pipe = 0;
            if (ioctl(watch.out, FIONREAD, &in_the_pipe) == 0) {
                while (in_the_pipe > 0) {
                    const std::size_t want = std::min(chunk.size(), static_cast<std::size_t>(in_the_pipe));
                    const ssize_t got = read(watch.out, chunk.data(), want);
                    if (got < 0 && errno == EINTR)
                        continue;
                    if (got <= 0)
                        break;
                    new_output(waiting, chunk.data(), static_cast<std::size_t>(got));
                    in_the_pipe -= static_cast<int>(got);
                }
            }
            return true;
        }
    }
}

// WHAT THE PROGRAM LEFT RUNNING WRITES AFTER IT HAS ENDED is shown as a terminal would show it,
// until the pipe closes or the run does (the review: closing the pipe at the program's end killed
// what it left behind with SIGPIPE, and lost its last lines).
void read_what_it_left(const Watch &watch, std::string &waiting, std::string &chunk)
{
    for (;;) {
        pollfd ready[2] = {{watch.out, POLLIN, 0}, {closing_fd(), POLLIN, 0}};
        const int seen = poll(ready, 2, waiting.empty() ? -1 : kQuietBeforeAHalfLine);
        if (seen < 0) {
            if (errno == EINTR)
                continue;
            return;
        }
        if (seen == 0) {
            to_the_screen(waiting);
            continue;
        }
        if (ready[0].revents != 0) {
            const ssize_t got = read(watch.out, chunk.data(), chunk.size());
            if (got > 0)
                new_output(waiting, chunk.data(), static_cast<std::size_t>(got));
            else if (got == 0 || (errno != EINTR && errno != EAGAIN))
                return;
        }
        if (ready[1].revents != 0)
            return;
    }
}

std::string signal_named(int signal)
{
    const char *short_name = sigabbrev_np(signal);
    return std::to_string(signal) + (short_name != nullptr ? std::string(" (SIG") + short_name + ")" : std::string());
}

// THE WATCHER: one OS thread a run, as a satellite thread has (thread_calls.hpp says why a pool
// would not do). It is the only thing that reaps the program, so the pid cannot be handed to
// another process while the run is going.
void *watch_the_program(void *given)
{
    std::unique_ptr<Watch> watch(static_cast<Watch *>(given));
    satellite_program &program = *watch->program;
    std::string waiting;
    std::string chunk;
    bool still_open = false;
    if (watch->out >= 0) {
        chunk.assign(64 * 1024, '\0');
        still_open = read_while_it_runs(*watch, waiting, chunk);
        to_the_screen(waiting);   // its last line, without the end it never wrote
    }
    int status = 0;
    while (waitpid(watch->pid, &status, 0) < 0 && errno == EINTR) {
    }
    {
        const std::lock_guard<std::mutex> hold(program.lock);
        if (WIFSIGNALED(status)) {
            program.code = 128 + WTERMSIG(status);
            program.why = "it was ended by signal " + signal_named(WTERMSIG(status));
        } else {
            program.code = WEXITSTATUS(status);
            program.why = program.code == 0 ? std::string() : "it ended with code " + std::to_string(program.code);
        }
        if (watch->pidfd >= 0)
            close(watch->pidfd);
        program.pidfd = -1;
        program.pid = 0;
        program.runs_ended = watch->run;
    }
    program.ended_signal.notify_all();
    if (still_open && closing_fd() >= 0) {
        read_what_it_left(*watch, waiting, chunk);
        to_the_screen(waiting);
    }
    if (watch->out >= 0)
        close(watch->out);
    watch->finished->store(true, std::memory_order_release);
    // ONE WRITER FEWER, AFTER ITS LAST HAND-OFF (machine/console_lock.hpp's programs_watched).
    programs_watched().fetch_sub(1, std::memory_order_acq_rel);
    return nullptr;
}

// WATCHERS THAT HAVE FINISHED, joined now, so a run that starts a program a million times keeps
// a list of the ones still going.
void join_finished_watchers()
{
    std::vector<WatcherThread> done;
    {
        const std::lock_guard<std::mutex> hold(watchers().lock);
        std::vector<WatcherThread> &threads = watchers().threads;
        const auto still = std::stable_partition(threads.begin(), threads.end(), [](const WatcherThread &each) {
            return !each.finished->load(std::memory_order_acquire);
        });
        done.assign(std::make_move_iterator(still), std::make_move_iterator(threads.end()));
        threads.erase(still, threads.end());
    }
    for (const WatcherThread &each : done)
        pthread_join(each.id, nullptr);
}

// UNDER program.lock: through the pidfd, which can never reach a process given this pid later.
void send(satellite_program &program, int signal)
{
#ifdef SYS_pidfd_send_signal
    if (program.pidfd >= 0) {
        syscall(SYS_pidfd_send_signal, program.pidfd, signal, nullptr, 0U);
        return;
    }
#endif
    if (program.pid > 0)
        kill(program.pid, signal);
}

// ASKED TO STOP -- the program and everything it started -- and killed if it has not a little
// later (the review: "satellite stopped it" left the program's own `sleep` running). Answers once
// the run has ended.
//
// FROZEN FIRST, THEN STOPPED. A program that has only just started may be forking as it is asked:
// `sh -c "sleep 7.31; echo never"` stopped a millisecond after its start() was found with nothing
// under it, then sh ended on SIGTERM, and the sleep it had forked meanwhile was handed to the
// session's reaper and lived on (found 2026-10-01, by the test the review asked for). So the
// program is SIGSTOPped, everything under it is found and SIGSTOPped too, again until a pass finds
// nothing new -- a stopped process starts nothing -- and only then is all of it sent SIGTERM and
// SIGCONT together.
void stop_it(satellite_program &program)
{
    std::vector<ProcessSeen> under;
    std::uint64_t run = 0;
    {
        std::unique_lock<std::mutex> hold(program.lock);
        // A RUN CLAIMED AND NOT SPAWNED YET -- another thread is inside start() -- has no pid for
        // a moment; it is waited for, briefly.
        while (program.running() && program.pid <= 0)
            program.ended_signal.wait_for(hold, std::chrono::milliseconds(10));
        if (!program.running())
            return;
        run = program.runs_started;
        send(program, SIGSTOP);
        for (int pass = 0; pass < 64; ++pass) {
            bool more = false;
            for (const ProcessSeen &found : processes_under(program.pid)) {
                const bool known = std::any_of(under.begin(), under.end(), [&found](const ProcessSeen &each) {
                    return each.pid == found.pid && each.started == found.started;
                });
                if (known)
                    continue;
                signal_if_still_there(found, SIGSTOP);
                under.push_back(found);
                more = true;
            }
            if (!more)
                break;
        }
        send(program, SIGTERM);
        for (const ProcessSeen &each : under)
            signal_if_still_there(each, SIGTERM);
        send(program, SIGCONT);
    }
    for (const ProcessSeen &each : under)
        signal_if_still_there(each, SIGCONT);
    const auto ended = [&program, run] {
        const std::lock_guard<std::mutex> hold(program.lock);
        return program.runs_ended >= run;
    };
    const auto deadline = std::chrono::steady_clock::now() + kStopGrace;
    while (std::chrono::steady_clock::now() < deadline) {
        if (ended() && std::none_of(under.begin(), under.end(), still_there))
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    {
        const std::lock_guard<std::mutex> hold(program.lock);
        if (program.runs_ended < run)
            send(program, SIGKILL);
    }
    for (const ProcessSeen &each : under)
        signal_if_still_there(each, SIGKILL);
    std::unique_lock<std::mutex> hold(program.lock);
    program.ended_signal.wait(hold, [&program, run] { return program.runs_ended >= run; });
}

// ON THE END-OF-RUN LIST once, however often it is started.
void remember(const ProgramHandle &which)
{
    const std::lock_guard<std::mutex> hold(unjoined().lock);
    if (which->listed)
        return;
    which->listed = true;
    unjoined().programs.push_back(which);
}

// OFF IT AGAIN once its run is over -- unless another thread has started the next one, which the
// end of the run is still owed.
void forget(const ProgramHandle &which)
{
    const std::lock_guard<std::mutex> hold(unjoined().lock);
    if (!which->listed)
        return;
    {
        const std::lock_guard<std::mutex> its(which->lock);
        if (which->running())
            return;
    }
    which->listed = false;
    std::vector<ProgramHandle> &programs = unjoined().programs;
    programs.erase(std::remove(programs.begin(), programs.end(), which), programs.end());
}

// A RUN THAT NEVER RAN: ended at once, ok() false, error() saying why.
void never_ran(satellite_program &program, std::uint64_t run, long long code, const std::string &why)
{
    {
        const std::lock_guard<std::mutex> hold(program.lock);
        program.could_start = false;
        program.code = code;
        program.why = why;
        program.pid = 0;
        program.runs_ended = run;
    }
    program.ended_signal.notify_all();
}

Value start(const ProgramHandle &which, const std::string &name, ExpressionContext &context)
{
    satellite_program &program = *which;
    CriticalReport where;
    place_here(where, context.state);
    std::uint64_t run = 0;
    {
        // THE RUN IS CLAIMED HERE, BEFORE ANYTHING IS SPAWNED (the review: two threads both passed a
        // check of `running` that was only set after the spawn, and both started it).
        const std::lock_guard<std::mutex> hold(program.lock);
        if (program.running()) {
            context.refuse(program_already_running, name + ".start() -- " + name + " is still running; " + name +
                                                        ".join() waits for it to end, and then it may start again");
            return Value();
        }
        run = ++program.runs_started;
        program.started = true;
        program.joined = false;
        program.could_start = false;
        program.why.clear();
        program.code = 0;
        program.name = name;
        program.started_at = where;
    }
    remember(which);
    join_finished_watchers();
    // satl'S OWN WORDS WRITTEN BEFORE THIS LINE GO FIRST, then the program's -- handed over under
    // the console's lock, as every other caller does (the review: unlocked, another thread's write
    // could land in a buffer already in the ring).
    {
        const ConsoleHold one_hand_off;
        hand_over_what_std_cout_holds();
    }
    // A WRITER MORE, BEFORE ITS THREAD EXISTS (machine/console_lock.hpp's programs_watched).
    programs_watched().fetch_add(1, std::memory_order_acq_rel);
    int pipe_ends[2] = {-1, -1};
    ProgramStart begun;
    if (pipe2(pipe_ends, O_CLOEXEC) != 0) {
        begun.code = 126;
        begun.why = std::string("the machine would not make a pipe for its output: ") + std::strerror(errno);
    } else {
        begun = start_a_program(program.words, pipe_ends[1]);
        close(pipe_ends[1]);
    }
    if (begun.pid <= 0) {
        if (pipe_ends[0] >= 0)
            close(pipe_ends[0]);
        never_ran(program, run, begun.code, begun.why);
        forget(which);
        programs_watched().fetch_sub(1, std::memory_order_acq_rel);
        return Value::of_program(which);
    }
    {
        const std::lock_guard<std::mutex> hold(program.lock);
        program.could_start = true;
        program.pid = begun.pid;
        program.pidfd = begun.pidfd;
    }
    program.ended_signal.notify_all();   // a stop_it() waiting for the pid
    auto finished = std::make_shared<std::atomic<bool>>(false);
    auto watch = std::make_unique<Watch>(Watch{which, run, pipe_ends[0], begun.pidfd, begun.pid, finished});
    // THE WATCHER TAKES NO SIGNAL (the review: one could take a process-wide SIGWINCH meant for the
    // prompt): every signal is blocked while it is made, and a thread starts with its maker's mask.
    sigset_t all, before;
    sigfillset(&all);
    pthread_sigmask(SIG_BLOCK, &all, &before);
    pthread_attr_t shape;
    pthread_attr_init(&shape);
    pthread_attr_setstacksize(&shape, 1024 * 1024);
    pthread_t id{};
    const int refused = pthread_create(&id, &shape, watch_the_program, watch.get());
    pthread_attr_destroy(&shape);
    pthread_sigmask(SIG_SETMASK, &before, nullptr);
    if (refused != 0) {
        // NO WATCHER, SO NO RUN: it is ended at once, and is a program that could not start.
        {
            const std::lock_guard<std::mutex> hold(program.lock);
            send(program, SIGKILL);
        }
        int status = 0;
        while (waitpid(begun.pid, &status, 0) < 0 && errno == EINTR) {
        }
        close(pipe_ends[0]);
        if (begun.pidfd >= 0)
            close(begun.pidfd);
        {
            const std::lock_guard<std::mutex> hold(program.lock);
            program.pidfd = -1;
        }
        never_ran(program, run, 126,
                  std::string("the machine would not make a thread to watch it: ") + std::strerror(refused));
        forget(which);
        programs_watched().fetch_sub(1, std::memory_order_acq_rel);
        return Value::of_program(which);
    }
    watch.release();   // the watcher owns it now
    {
        const std::lock_guard<std::mutex> hold(watchers().lock);
        watchers().threads.push_back(WatcherThread{id, which, std::move(finished)});
    }
    return Value::of_program(which);
}

// error()'s words as a string. They are made of a program's name, which came from a satellite
// string, and the machine's English -- so this cannot fail, and is still asked.
Value a_string(const std::string &text)
{
    Value out;
    std::size_t bad_offset = 0;
    if (Value::of_utf8(text, out, bad_offset) != success)
        Value::of_utf8("its reason held a byte that is not part of any character", out, bad_offset);
    return out;
}

// join(), code() and return(): "wait for the program to finish ... and return the error code".
// IT WAITS FOR THE RUN THAT WAS GOING WHEN IT WAS REACHED, and it looks up every tenth of a second:
// a thread asked to stop, and satellite.return(satellite) on another thread, end its statement
// as they would between two (the review: a join on a server held the end of the run forever).
Value join(const ProgramHandle &which, ExpressionContext &context)
{
    satellite_program &program = *which;
    long long code = 0;
    {
        std::unique_lock<std::mutex> hold(program.lock);
        const std::uint64_t run = program.runs_started;
        program.joined = true;   // "both .start() and .join()": this run's join was written, and reached
        while (program.runs_ended < run) {
            program.ended_signal.wait_for(hold, kJoinLooksUp);
            if (stop_of_this_thread != nullptr && stop_of_this_thread->load(std::memory_order_relaxed)) {
                context.refuse(thread_stopped, "a join() on a thread asked to stop");
                context.reported = true;
                return Value();
            }
            if (program_quit().load(std::memory_order_relaxed)) {
                context.refuse(program_returned, "a join() when satellite.return(satellite) was reached");
                context.reported = true;
                return Value();
            }
        }
        code = program.code;
    }
    forget(which);
    return Value::of_number(satellite_number::from_signed(code));
}

} // namespace

bool is_program_type(token::Code word)
{
    return word == word::code_of(1, 6, 23);
}

signed long long int program_on_store(token::Code holds, Value &value, std::string &why)
{
    if (!is_program_type(holds) || value.is_program())
        return success;
    std::vector<std::string> words;
    if (const satellite_string *text = value.as_string()) {
        words.push_back(text->to_utf8());
    } else if (const ListHandle *list = value.as_list(); list != nullptr && *list != nullptr) {
        const std::vector<Value> &items = (*list)->items;
        for (std::size_t at = 0; at < items.size(); ++at) {
            const satellite_string *word = items[at].as_string();
            if (word == nullptr) {
                why = "item " + std::to_string(at + 1) + " of it is " + items[at].kind_name() +
                      ", and every word of a program -- its name and each argument -- is a string";
                return types_do_not_meet;
            }
            words.push_back(word->to_utf8());
        }
    } else {
        return success;   // the shape check after this says what it holds instead
    }
    if (words.empty() || words.front().empty()) {
        why = words.empty() ? "the list is empty, and a program needs at least its own name"
                            : "its first word is empty, and that is where the program's name goes";
        return types_do_not_meet;
    }
    auto program = std::make_shared<satellite_program>();
    program->words = std::move(words);
    value = Value::of_program(std::move(program));
    return success;
}

int program_method_arity(token::Code method)
{
    if (method == token::start_token || method == token::ok_token || method == token::error_text_token ||
        method == token::join_token || method == token::code_token)
        return 0;
    return -1;
}

std::string program_methods_are()
{
    return "a program has .start(), .ok(), .error(), .join(), .code() and .return()";
}

Value call_program_method(token::Code method, const ProgramHandle &which, const std::vector<Value> &arguments,
                          bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    const std::string spelling = token::method_name_of(method);
    if (program_method_arity(method) < 0) {
        context.refuse(types_do_not_meet, name + "." + spelling + " -- " + program_methods_are());
        return Value();
    }
    if (!had_parentheses || !arguments.empty()) {
        context.refuse(satl_line_not_understood, name + "." + spelling + "() takes nothing, in its brackets");
        return Value();
    }
    if (which == nullptr) {
        context.refuse(satl_line_not_understood, name + " holds no program yet -- give it one with = \"name\" or "
                                                        "= {\"name\", \"argument\"}");
        return Value();
    }
    if (method == token::start_token)
        return start(which, name, context);
    satellite_program &program = *which;
    std::string why;
    bool could_start = false;
    {
        const std::lock_guard<std::mutex> hold(program.lock);
        if (!program.started) {
            context.refuse(program_not_started, name + "." + spelling + "() -- " + name + " was never started; " +
                                                    name + ".start() comes first");
            return Value();
        }
        why = program.why;
        could_start = program.could_start;
    }
    if (method == token::ok_token)
        return Value::of_bool(could_start);
    if (method == token::error_text_token)
        return a_string(why);
    return join(which, context);
}

signed long long int close_every_program(signed long long int run_ended_with)
{
    if (closing_fd() >= 0) {
        const std::uint64_t one = 1;
        [[maybe_unused]] const ssize_t said = write(closing_fd(), &one, sizeof one);
    }
    signed long long int answer = success;
    // EVERY PROGRAM ON THE LIST, stopped if it runs, and reported if no join() was ever reached for
    // it. A thread may start another while these close, so this goes round until a pass finds none.
    for (;;) {
        std::vector<ProgramHandle> closing;
        {
            const std::lock_guard<std::mutex> hold(unjoined().lock);
            closing.swap(unjoined().programs);
            for (const ProgramHandle &each : closing)
                each->listed = false;
        }
        if (closing.empty())
            break;
        for (const ProgramHandle &each : closing) {
            satellite_program &program = *each;
            bool was_running = false;
            {
                const std::lock_guard<std::mutex> hold(program.lock);
                was_running = program.running();
            }
            stop_it(program);
            CriticalReport report;
            long long code = 0;
            {
                const std::lock_guard<std::mutex> hold(program.lock);
                // ONE THAT COULD NOT START NEVER RAN, so there was nothing to wait for; and a run that is
                // already failing has its own report, which this would only bury.
                if (program.joined || !program.could_start || run_ended_with != success)
                    continue;
                report = program.started_at;
                code = program.code;
            }
            const SCode named = s_code_for(program_never_joined);
            report.code = named.code;
            report.name = named.name;
            report.description = named.means;
            report.notes.push_back(program.name + " was started here, and nothing waited for it: " +
                                   (was_running ? std::string("it was still running, so satellite stopped it")
                                                : "it had ended, with code " + std::to_string(code)));
            report.notes.push_back("write " + program.name + ".join() -- or .code() or .return() -- after its start()");
            print_critical(report);
            answer = program_never_joined;
        }
    }
    // EVERY WATCHER JOINED. The closing event stops one still showing what a program left running;
    // a run started after the list above was taken is stopped first.
    for (;;) {
        std::vector<WatcherThread> joining;
        {
            const std::lock_guard<std::mutex> hold(watchers().lock);
            joining.swap(watchers().threads);
        }
        if (joining.empty())
            break;
        for (const WatcherThread &each : joining) {
            stop_it(*each.program);
            pthread_join(each.id, nullptr);
        }
    }
    // THE CLOSING EVENT TAKEN BACK, so a window's presses or the prompt can start programs again.
    if (closing_fd() >= 0) {
        std::uint64_t taken = 0;
        while (read(closing_fd(), &taken, sizeof taken) > 0) {
        }
    }
    return answer;
}

} // namespace satellite004
