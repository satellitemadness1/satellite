// The thread handle. See satellite_thread/thread_handle.hpp for what a thread
// shares with the walk that started it, and why the OS thread is a fresh one.

#include "satellite_thread/thread_handle.hpp"

#include "evaluator/machine.hpp"

#include <csignal>
#include <cstring>
#include <ctime>
#include <mutex>
#include <pthread.h>
#include <system_error>
#include <utility>

namespace satellite::thread {

namespace {

// THE CHILD'S OWN VIEW OF ITSELF. Two thread_locals and a free function,
// because evaluator/machine.hpp's Policy carries `bool (*interrupted)()` -- a
// plain function pointer with nowhere to put a handle.
//
// THAT SIGNATURE IS NOT A LIMITATION TO ROUTE AROUND, IT IS THE SEAM. machine
// .hpp says why it is a function and not a read: "`satl`'s arms pass
// interrupt_requested; tests/eval_test passes a function of its own and can
// interrupt a run without a signal ever being raised." A thread_local is the
// one thing that can add per-thread state to a per-process signature without
// widening it for every walk that has no thread in it.
thread_local ThreadHandle *self = nullptr;
thread_local bool (*inherited)() = nullptr;

// STOPPED OR INTERRUPTED, AND THE WALK CANNOT TELL THEM APART ON PURPOSE. Both
// mean "stop at the next statement boundary" and both produce Ending::
// Interrupted; what separates them is who asked and what the answer is worth,
// and that is decided in close_all() where the asking happened. A Ctrl-C
// reaches every thread because `inherited` is the same process-wide flag the
// parent reads.
bool stopped_or_interrupted()
{
    if (self != nullptr && self->stop.load(std::memory_order_relaxed))
        return true;
    return inherited != nullptr && inherited();
}

// THE WAKE SIGNAL, AND WHY THE STOP FLAG IS NOT ENOUGH ON ITS OWN.
//
// close_all() raises `stop` and the child sees it at its next STATEMENT
// BOUNDARY -- which is the right mechanism and is M11's, and which reaches a
// walk that is walking. A walk sitting inside `satellite.time.sleep(3600)` is
// not walking: it is in clock_nanosleep, it will not reach a statement boundary
// for an hour, and close_all() would wait for it.
//
// MEASURED BEFORE THIS EXISTED, and it is the one thing in this milestone that
// was found by running it rather than by reading: a thread given a 0.3 s head
// start into a long sleep hung `satl` at exit, while the same program with no
// head start exited in 3 ms because the stop flag arrived before the thread had
// reached its first statement. A bug whose appearance depends on a race between
// the child getting started and the parent finishing is exactly the kind this
// tree writes down. MILESTONES/M23.md §3.3.
//
// SO THE FLAG IS THE MESSAGE AND THE SIGNAL IS THE ALARM CLOCK, which is
// system_facts/interrupt.cpp's own sentence about its wake pipe, one mechanism
// over and for the identical reason. The handler does NOTHING: its whole job is
// to make a blocked syscall come back EINTR, at which point sleep_nanoseconds
// asks the interrupted hook, the hook reads `stop`, and the ordinary path takes
// over.
//
// SIGUSR2 AND NOT SIGINT, WHICH WAS THE FIRST IDEA AND IS WRONG. Reusing SIGINT
// would print "SATELLITE: CTRL+C RECEIVED" to a person who pressed nothing, set
// the process-wide interrupt count, and -- on the second thread closed --
// ESCALATE inside the handler to `_exit`, because interrupt.cpp treats a second
// arrival as "the first one did not land". Three wrong things from one reused
// number.
//
// AND IT IS pthread_kill AND NOT raise/kill, so the signal lands on the thread
// that is asleep instead of on whichever one the kernel picks. That is the
// difference interrupt.cpp needed a PIPE to work around for a process-directed
// SIGINT; here the target is known, so the simpler mechanism is available.
constexpr int kWake = SIGUSR2;

// NOTHING, ON PURPOSE. An empty handler is async-signal-safe by construction
// and EINTR is the entire message.
void woken(int) {}

void arm_the_wake()
{
    // installed EXACTLY ONCE, interrupt.cpp's `exchange` and its reason: two
    // threads calling launch() at the same moment would both see false and both
    // install, which is harmless and would make the idempotence a coincidence.
    static std::atomic<bool> armed{false};
    if (armed.exchange(true))
        return;

    struct sigaction action;
    std::memset(&action, 0, sizeof action);
    action.sa_handler = woken;
    sigemptyset(&action.sa_mask);

    // NO SA_RESTART, for interrupt.cpp's reason word for word: a restarted
    // clock_nanosleep would go straight back to sleep and this whole mechanism
    // would be a signal nobody could feel.
    action.sa_flags = 0;
    sigaction(kWake, &action, nullptr);
}

// KEEP ASKING UNTIL IT ANSWERS. One signal is not enough and the reason is a
// race with no lock that can close it: `stop` is set, then the signal is sent,
// and the thread may be BETWEEN those two instants -- past its last statement
// boundary and not yet inside the syscall -- in which case the wake is
// delivered to a thread that is not blocked, does nothing, and the thread then
// blocks for an hour.
//
// SO IT IS A RETRY AND NOT A TIMEOUT, WHICH IS THE DISTINCTION THAT MAKES IT
// ALLOWED. SCRATCH.md/NO_LIMITS.md's rule is about ceilings on what a program
// may do; this puts no ceiling on anything. It ends when the thread says it has
// ended, it waits a millisecond between asks so it is not a spin, and a thread
// that never comes back is a thread in an uninterruptible syscall -- which no
// mechanism in user space can reach and which a timeout would only let us LIE
// about.
void keep_waking(const Thr &handle)
{
    const pthread_t which = handle->worker.native_handle();
    while (!handle->finished.load(std::memory_order_acquire)) {
        // SAFE ON A THREAD THAT HAS ALREADY ENDED. The pthread_t stays valid
        // until join() or detach(), so the worst case is ESRCH, which is the
        // loop's own exit condition arriving by another road.
        pthread_kill(which, kWake);
        const struct timespec pause{0, 1000000}; // 1 ms
        nanosleep(&pause, nullptr);
    }
}

// THE REGISTRY OF LIVE THREADS -- what close_all() closes.
//
// A FUNCTION-LOCAL STATIC AND NOT A NAMESPACE ONE, which is the shape v1's
// watchdog has and is worth the two extra characters: a namespace static is
// constructed before main in an order nothing specifies, and this one is
// touched by `start()` on a path that may run very early in a prompt session.
struct Registry {
    std::mutex lock;
    std::vector<Thr> live;
};

Registry &registry()
{
    static Registry the;
    return the;
}

// ONE THREAD'S WHOLE LIFE. Build a Machine over the shared program, run the
// capsule, put the answer where join() will find it.
//
// NOTHING HERE MAY THROW AND NOTHING HERE DOES, which is the same promise
// machine_limits/pool.hpp extracts from a pool body and for the same reason:
// this runs at the top of a thread with no handler above it, so an escaping
// exception is std::terminate. PLAN §7 is why there is nothing to escape --
// every refusal in this tree is a Diagnostic returned, not thrown.
void run_the_body(const Thr &handle)
{
    self = handle.get();
    inherited = handle->policy.interrupted;

    eval::Policy mine = handle->policy;
    mine.interrupted = stopped_or_interrupted;

    eval::Machine machine(*handle->program, *handle->ast, mine,
                          handle->globals);

    // THE TOP LEVEL IS NOT RE-RUN, AND THAT IS THE POINT OF SHARING THE
    // GLOBALS. `satellite.library`'s initialisers ran once, on the main walk,
    // before any capsule did -- DESIGN §7.2's order. A thread that ran them
    // again would reset every global the program had changed since, which is a
    // program silently losing work rather than a race.
    handle->answer = machine.call(handle->body->capsule,
                                  handle->body->arguments);
    handle->problems = machine.problems();
    handle->ending = machine.ending();

    // LAST, AND AFTER EVERYTHING ELSE IS WRITTEN. close_all() reads `finished`
    // to decide whether this thread ran to its own end or was stopped, and a
    // store published before `answer` was would make that answer a lie.
    handle->finished.store(true, std::memory_order_release);
}

} // namespace

bool launch(const Thr &handle, std::string &why)
{
    handle->started.store(true, std::memory_order_relaxed);

    // SHARED BEFORE THE CHILD EXISTS. evaluator/globals.hpp's safety argument
    // is entirely about this line's position: the store is sequenced before
    // std::thread's constructor, and that constructor is a synchronisation
    // point C++ requires to publish it. There is no window in which one walk
    // thinks the globals are private while another is reading them.
    handle->globals->share();

    {
        std::lock_guard<std::mutex> held(registry().lock);
        registry().live.push_back(handle);
    }

    arm_the_wake();

    try {
        handle->worker = std::thread(run_the_body, handle);
    } catch (const std::system_error &refused) {
        // THE MACHINE SAID NO. std::thread's constructor is the one thing in
        // this tree that throws, which machine_limits/pool.cpp already had to
        // know -- it catches the same exception for the same reason and calls a
        // pool that could not be fully built "a SMALLER pool". There is no
        // smaller answer available here: the program asked for a thread and
        // there is not one, so it is told, with the operating system's own
        // sentence attached.
        handle->started.store(false, std::memory_order_relaxed);
        why = refused.what();
        std::lock_guard<std::mutex> held(registry().lock);
        if (!registry().live.empty() && registry().live.back() == handle)
            registry().live.pop_back();
        return false;
    }
    return true;
}

void wait(const Thr &handle)
{
    if (handle->worker.joinable())
        handle->worker.join();
    handle->joined = true;

    // TAKEN OUT OF THE REGISTRY, because close_all() exists to close what
    // nobody closed and this one has been. Leaving it in would make its
    // problems get reported twice -- once by the join that raised them and once
    // at the end of the run.
    std::lock_guard<std::mutex> held(registry().lock);
    for (size_t i = 0; i < registry().live.size(); i++)
        if (registry().live[i] == handle) {
            registry().live.erase(registry().live.begin() +
                                  static_cast<long>(i));
            break;
        }
}

std::vector<errors::Diagnostic> close_all()
{
    // TAKEN OUT FROM UNDER THE LOCK FIRST, AND NOT WALKED UNDER IT. A child
    // still running may be inside `wait()` on a handle of its own -- a thread
    // that started a thread -- and joining while holding this lock would be
    // this function waiting for a thread waiting for this lock.
    std::vector<Thr> taken;
    {
        std::lock_guard<std::mutex> held(registry().lock);
        taken.swap(registry().live);
    }

    std::vector<errors::Diagnostic> unreported;
    for (const Thr &handle : taken) {
        handle->stop.store(true, std::memory_order_relaxed);
        if (handle->worker.joinable()) {
            // THE FLAG, THEN THE ALARM CLOCK, THEN THE WAIT -- in that order,
            // because the flag is what the thread acts on and the signal only
            // makes it look.
            keep_waking(handle);
            handle->worker.join();
        }

        // ASKED AFTER THE JOIN, WHERE IT IS A FACT. The ending says which of
        // the two things happened to this thread, and `Interrupted` is the one
        // this function caused -- see the header for the race that reading
        // `finished` beforehand had.
        if (!handle->joined && handle->ending != eval::Ending::Interrupted)
            for (const errors::Diagnostic &problem : handle->problems)
                unreported.push_back(problem);
    }
    return unreported;
}

} // namespace satellite::thread
