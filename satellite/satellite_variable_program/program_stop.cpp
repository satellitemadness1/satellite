// satellite/satellite_variable_program/program_stop.cpp -- the header says what this is. Moved
// here from bytecode/program_calls.cpp as it was (2026-10-01, STEP 5); each fix the first review
// asked for still says where it is made what it answers.

#include "program_stop.hpp"

#include "program_spawn.hpp"
#include "program_watch.hpp"
#include "../machine/critical_report.hpp"
#include "../machine/s_codes.hpp"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <mutex>
#include <sys/syscall.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace satellite004 {
namespace {

constexpr auto kStopGrace = std::chrono::seconds(5);            // SIGTERM, this long, then SIGKILL
constexpr int kLongestStopWait = 200;                           // ms a process is given to show it has stopped

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

} // namespace

// A RUN'S WATCHER, kept to be joined: once it has finished, or at the end of the run.
void keep_watcher(pthread_t id, const ProgramHandle &program, std::shared_ptr<std::atomic<bool>> finished)
{
    const std::lock_guard<std::mutex> hold(watchers().lock);
    watchers().threads.push_back(WatcherThread{id, program, std::move(finished)});
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
// Answers 0, or the machine's reason for refusing -- EPERM for a program that runs as another user.
int signal_the_program(satellite_program &program, int signal)
{
#ifdef SYS_pidfd_send_signal
    if (program.pidfd >= 0)
        return syscall(SYS_pidfd_send_signal, program.pidfd, signal, nullptr, 0U) == 0 ? 0 : errno;
#endif
    if (program.pid > 0)
        return kill(program.pid, signal) == 0 ? 0 : errno;
    return 0;
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
//
// ONE THE MACHINE WILL NOT LET satl SIGNAL -- it runs as another user, as sudo and pkexec make it --
// is waited for until it ends by itself, and S743 says so once on the screen: before the review of
// steps 2-5 the signals failed in silence and the wait looked like satl had hung.
void stop_it(satellite_program &program, std::uint64_t only)
{
    std::vector<ProcessSeen> under;
    std::uint64_t run = 0;
    bool not_permitted = false;
    CriticalReport report;
    {
        std::unique_lock<std::mutex> hold(program.lock);
        // ONLY THE RUN ASKED ABOUT, when one was -- end()'s: once that run has ended, one another
        // thread started since is not this call's to stop (the review of steps 2-5).
        const auto its_run_ended = [&program, only] { return only != 0 && program.runs_ended >= only; };
        // A RUN CLAIMED AND NOT SPAWNED YET -- another thread is inside start() -- has no pid for
        // a moment; it is waited for, briefly.
        while (!its_run_ended() && program.running() && program.pid <= 0)
            program.ended_signal.wait_for(hold, std::chrono::milliseconds(10));
        if (its_run_ended() || !program.running())
            return;
        run = program.runs_started;
        // EACH ONE WAITED FOR UNTIL IT HAS STOPPED: a SIGSTOP lands when the process next runs, and
        // a scan made before that missed the sleep sh forked in between (found 2026-10-01 by
        // tests/program_end.satl, a tree ended at once after its start()).
        not_permitted = signal_the_program(program, SIGSTOP) == EPERM;
        if (not_permitted)
            report = program.started_at;
        wait_until_stopped(seen_now(program.pid), not_permitted ? 0 : kLongestStopWait);
        for (int pass = 0; pass < 64; ++pass) {
            bool more = false;
            for (const ProcessSeen &found : processes_under(program.pid)) {
                const bool known = std::any_of(under.begin(), under.end(), [&found](const ProcessSeen &each) {
                    return each.pid == found.pid && each.started == found.started;
                });
                if (known)
                    continue;
                signal_if_still_there(found, SIGSTOP);
                wait_until_stopped(found, kLongestStopWait);
                under.push_back(found);
                more = true;
            }
            if (!more)
                break;
        }
        signal_the_program(program, SIGTERM);
        for (const ProcessSeen &each : under)
            signal_if_still_there(each, SIGTERM);
        signal_the_program(program, SIGCONT);
    }
    for (const ProcessSeen &each : under)
        signal_if_still_there(each, SIGCONT);
    // SAID AFTER THE PROGRAM'S LOCK IS LET GO: nothing takes the console's lock while it holds one.
    if (not_permitted) {
        const SCode named = s_code_for(program_not_stopped);
        report.code = named.code;
        report.name = named.name;
        report.description = named.means;
        print_notice(report);
    }
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
            signal_the_program(program, SIGKILL);
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
            std::string name;
            {
                const std::lock_guard<std::mutex> hold(program.lock);
                // ONE THAT COULD NOT START NEVER RAN, so there was nothing to wait for; and a run that is
                // already failing has its own report, which this would only bury.
                if (program.joined || !program.could_start || run_ended_with != success)
                    continue;
                report = program.started_at;
                code = program.code;
                name = program.name;   // under the lock: a thread still running may be in a start()
            }
            const SCode named = s_code_for(program_never_joined);
            report.code = named.code;
            report.name = named.name;
            report.description = named.means;
            report.notes.push_back(name + " was started here, and nothing waited for it: " +
                                   (was_running ? std::string("it was still running, so satellite stopped it")
                                                : "it had ended, with code " + std::to_string(code)));
            report.notes.push_back("write " + name + ".join() -- or .code() or .return() -- after its start()");
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
