// satellite/bytecode/program_calls.cpp -- the header says what these are for, the author's words
// for each, and which choices are mine. A fresh reader found twelve defects in the first version
// (2026-10-01); each fix says where it is made what it answers. The watcher -- the thread a run
// has -- is satellite_variable_program/program_watch.cpp, and stopping a program -- end(), and the
// end of the run -- is program_stop.cpp.

#include "program_calls.hpp"

#include "word_codes.hpp"
#include "../display/printing_satellite.hpp"
#include "../machine/console_lock.hpp"
#include "../machine/critical_report.hpp"
#include "../machine/s_codes.hpp"
#include "../machine/thread_stop.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_variable_program/program_spawn.hpp"
#include "../satellite_variable_program/program_stop.hpp"
#include "../satellite_variable_program/program_watch.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <sys/eventfd.h>
#include <sys/wait.h>
#include <unistd.h>

namespace satellite004 {
namespace {

constexpr auto kJoinLooksUp = std::chrono::milliseconds(100);   // how often a waiting join() looks up

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

Value start(const ProgramHandle &which, const std::string &name, bool hidden, ExpressionContext &context)
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
        program.input_waiting.clear();
        program.input_closing = false;
        program.input_gone = false;
        program.input_wake = -1;
    }
    remember(which);
    join_finished_watchers();
    // satl'S OWN WORDS WRITTEN BEFORE THIS LINE GO FIRST, then the program's -- handed over under
    // the console's lock, as every other caller does (the review: unlocked, another thread's write
    // could land in a buffer already in the ring).
    // start("hide") -- "the output will be displayed unless my_program.start("hide") is called" (the
    // author): its output and its errors go to /dev/null, so there is no pipe, nothing for the
    // screen, and no writer more on the ring.
    if (!hidden) {
        const ConsoleHold one_hand_off;
        hand_over_what_std_cout_holds();
    }
    // A WRITER MORE, BEFORE ITS THREAD EXISTS (machine/console_lock.hpp's programs_watched).
    if (!hidden)
        programs_watched().fetch_add(1, std::memory_order_acq_rel);
    const auto a_writer_fewer = [hidden] {
        if (!hidden)
            programs_watched().fetch_sub(1, std::memory_order_acq_rel);
    };
    // ITS INPUT IS A PIPE satl HOLDS (STEP 4): what pass() types goes in it, and join() or end() closes
    // it. The write end never blocks -- the watcher writes as the program reads -- and the wake is how
    // pass() and join() tell the watcher to look again.
    int output_ends[2] = {-1, -1};
    int input_ends[2] = {-1, -1};
    const int wake = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    ProgramStart begun;
    std::string the_machine_said;
    if (wake < 0 || pipe2(input_ends, O_CLOEXEC) != 0)
        the_machine_said = std::string("the machine would not make a pipe for its input: ") + std::strerror(errno);
    else if (!hidden && pipe2(output_ends, O_CLOEXEC) != 0)
        the_machine_said = std::string("the machine would not make a pipe for its output: ") + std::strerror(errno);
    if (!the_machine_said.empty()) {
        begun.code = 126;
        begun.why = the_machine_said;
    } else {
        begun = start_a_program(program.words, hidden ? -1 : output_ends[1], input_ends[0]);
    }
    for (const int end : {output_ends[1], input_ends[0]})
        if (end >= 0)
            close(end);   // the program's ends, which it has now
    const auto close_satls_ends = [&output_ends, &input_ends, wake] {
        for (const int end : {output_ends[0], input_ends[1], wake})
            if (end >= 0)
                close(end);
    };
    if (begun.pid <= 0) {
        close_satls_ends();
        never_ran(program, run, begun.code, begun.why);
        forget(which);
        a_writer_fewer();
        return Value::of_program(which);
    }
    fcntl(input_ends[1], F_SETFL, fcntl(input_ends[1], F_GETFL) | O_NONBLOCK);
    {
        const std::lock_guard<std::mutex> hold(program.lock);
        program.could_start = true;
        program.pid = begun.pid;
        program.pidfd = begun.pidfd;
        program.input_wake = wake;
    }
    program.ended_signal.notify_all();   // a stop_it() waiting for the pid
    auto finished = std::make_shared<std::atomic<bool>>(false);
    auto watch = std::make_unique<Watch>(
        Watch{which, run, output_ends[0], input_ends[1], wake, begun.pidfd, begun.pid, finished});
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
            signal_the_program(program, SIGKILL);
        }
        int status = 0;
        while (waitpid(begun.pid, &status, 0) < 0 && errno == EINTR) {
        }
        {
            const std::lock_guard<std::mutex> hold(program.lock);
            close_satls_ends();
            if (begun.pidfd >= 0)
                close(begun.pidfd);
            program.pidfd = -1;
            program.input_wake = -1;
        }
        never_ran(program, run, 126,
                  std::string("the machine would not make a thread to watch it: ") + std::strerror(refused));
        forget(which);
        a_writer_fewer();
        return Value::of_program(which);
    }
    watch.release();   // the watcher owns it now
    keep_watcher(id, which, std::move(finished));
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

// UNDER program.lock: the input closes once the watcher has written everything pass() typed --
// so a program reading to its end (sort, cat, wc) gets that end and finishes.
void close_the_input(satellite_program &program)
{
    if (program.input_closing)
        return;
    program.input_closing = true;
    if (program.input_wake >= 0) {
        const std::uint64_t one = 1;
        [[maybe_unused]] const ssize_t said = write(program.input_wake, &one, sizeof one);
    }
}

// ONE TYPED LINE: a value's text and an end of line, as a person would type it and press Enter.
bool typed_line(const Value &value, std::string &typed, std::string &why)
{
    satellite_string text;
    if (value.to_string(text, why) != success)
        return false;
    typed += text.to_utf8();
    typed += '\n';
    return true;
}

// pass(text) and pass({"one", "two"}) -- "typed input while the program is running" (the author,
// 2026-10-01): each a line, written to the program's input as it reads, never waited on here. It
// answers true when the program was running and could still take input, and false when it had
// ended, its input had been closed by join() or end(), or the program closed it.
Value pass(const ProgramHandle &which, const Value &given, const std::string &name, ExpressionContext &context)
{
    std::string typed;
    std::string why;
    bool made = true;
    if (const ListHandle *list = given.as_list(); list != nullptr && *list != nullptr) {
        for (const Value &item : (*list)->items)
            made = made && typed_line(item, typed, why);
    } else {
        made = typed_line(given, typed, why);
    }
    if (!made) {
        context.refuse(types_do_not_meet, name + ".pass() was given something with no text to type -- " + why);
        return Value();
    }
    satellite_program &program = *which;
    const std::lock_guard<std::mutex> hold(program.lock);
    if (!program.running() || program.input_closing || program.input_gone)
        return Value::of_bool(false);
    program.input_waiting += typed;
    if (program.input_wake >= 0) {
        const std::uint64_t one = 1;
        [[maybe_unused]] const ssize_t said = write(program.input_wake, &one, sizeof one);
    }
    return Value::of_bool(true);
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
        close_the_input(program);
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

// end(), exit(), quit() and shutdown() -- the author, 2026-10-01: ".shutdown() kill the process, and
// same with .end() ... .end() .exit() and .quit() and shutdown() all do the same thing". Stopped as
// the end of the run stops one -- frozen, then it and everything under it asked with SIGTERM, and
// killed five seconds later if it has not gone -- and then answered as join() answers, with its
// exit code: 143 for SIGTERM, 137 for SIGKILL, its own when it had already ended. It counts as
// joined: a program ended on purpose is not one nothing waited for.
Value end(const ProgramHandle &which)
{
    satellite_program &program = *which;
    {
        const std::lock_guard<std::mutex> hold(program.lock);
        program.joined = true;
        close_the_input(program);
    }
    stop_it(program);
    long long code = 0;
    {
        const std::lock_guard<std::mutex> hold(program.lock);
        program.joined = true;   // and a run another thread started meanwhile is this one's too
        code = program.code;
    }
    forget(which);
    return Value::of_number(satellite_number::from_signed(code));
}

} // namespace

bool is_program_type(token::Code word)
{
    return word == word::code_of(1, 6, 23) || is_bash_type(word);
}

bool is_bash_type(token::Code word)
{
    return word == word::code_of(1, 6, 24);
}

signed long long int program_on_store(token::Code holds, Value &value, std::string &why)
{
    if (!is_program_type(holds) || value.is_program())
        return success;
    // A BASH LINE (STEP 5) IS ONE STRING, which bash reads. A list is refused, not guessed at: it
    // could be lines of a script, or a line and the words bash hands it as $1, $2 -- the author's.
    if (is_bash_type(holds)) {
        const satellite_string *text = value.as_string();
        if (text == nullptr && value.as_list() != nullptr) {
            why = "it holds a list -- a bash line is one string, and bash reads its spaces, quotes, ; and | "
                  "itself; a program and its arguments as a list is satellite.variable.program";
            return types_do_not_meet;
        }
        if (text == nullptr)
            return success;   // the shape check after this says what it holds instead
        std::string line = text->to_utf8();
        if (line.empty()) {
            why = "the line is empty -- there is nothing in it for bash to run";
            return types_do_not_meet;
        }
        auto program = std::make_shared<satellite_program>();
        program->words = {"bash", "-c", "--", std::move(line)};
        program->bash = true;
        value = Value::of_program(std::move(program));
        return success;
    }
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
    if (method == token::start_token || method == token::pass_token)
        return 1;                        // start() or start("hide"); pass(text)
    if (method == token::ok_token || method == token::error_text_token || method == token::join_token ||
        method == token::code_token || method == token::end_token)
        return 0;
    return -1;
}

int program_method_least(token::Code method)
{
    return method == token::pass_token ? 1 : 0;
}

std::string program_method_takes(token::Code method)
{
    if (method == token::pass_token)
        return "() takes one thing to type in: a string, or a list of them, one line each";
    return method == token::start_token ? "() takes nothing, or \"hide\"" : "() takes nothing, in its brackets";
}

std::string program_methods_are(bool bash)
{
    return std::string(bash ? "a bash line" : "a program") +
           " has .start(), .ok(), .error(), .join(), .code(), .return(), .end(), .exit(), .quit(), "
           ".shutdown() and .pass()";
}

Value call_program_method(token::Code method, const ProgramHandle &which, const std::vector<Value> &arguments,
                          bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    const std::string spelling = token::method_name_of(method);
    if (program_method_arity(method) < 0) {
        context.refuse(types_do_not_meet,
                       name + "." + spelling + " -- " + program_methods_are(which != nullptr && which->bash));
        return Value();
    }
    if (!had_parentheses || arguments.size() > static_cast<std::size_t>(program_method_arity(method)) ||
        arguments.size() < static_cast<std::size_t>(program_method_least(method))) {
        context.refuse(satl_line_not_understood, name + "." + spelling + program_method_takes(method));
        return Value();
    }
    if (which == nullptr) {
        context.refuse(satl_line_not_understood, name + " holds no program yet -- give it one with = \"name\" or "
                                                        "= {\"name\", \"argument\"}");
        return Value();
    }
    if (method == token::start_token) {
        bool hidden = false;
        if (!arguments.empty()) {
            const satellite_string *how = arguments.front().as_string();
            hidden = how != nullptr && how->to_utf8() == "hide";
            if (!hidden) {
                satellite_string shown;
                std::string why;
                const std::string given = arguments.front().to_string(shown, why) == success ? shown.to_utf8() : "that";
                context.refuse(satl_line_not_understood, name + ".start(" + given + ") -- start() takes nothing, or "
                                                             "\"hide\" to run it with its output thrown away");
                return Value();
            }
        }
        return start(which, name, hidden, context);
    }
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
    if (method == token::end_token)
        return end(which);
    if (method == token::pass_token)
        return pass(which, arguments.front(), name, context);
    return join(which, context);
}

} // namespace satellite004
