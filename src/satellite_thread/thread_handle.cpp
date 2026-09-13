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

// THE CHILD'S OWN VIEW OF ITSELF. A thread_local and a free function,
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

// STOPPED OR INTERRUPTED, AND THE WALK CANNOT TELL THEM APART ON PURPOSE. Both
// mean "stop at the next statement boundary" and both produce Ending::
// Interrupted; what separates them is who asked and what the answer is worth,
// and that is decided in close_all() where the asking happened.
//
// THIS THREAD, THEN EVERY THREAD ABOVE IT, THEN THE PROCESS -- THREAD.md D8.
// The first version read `self->stop` and then called an `inherited` hook
// copied from the parent's policy; for a thread started by a thread that hook
// was this function, and the child spun in it for ever. The chain is the
// parents' handles, and the last question is the root hook, which is never
// this function: interrupt_root() below makes sure of it.
bool stopped_or_interrupted()
{
    ThreadHandle *const me = self;
    for (ThreadHandle *at = me; at != nullptr; at = at->parent.get())
        if (at->stop.load(std::memory_order_relaxed))
            return true;
    return me != nullptr && me->root != nullptr && me->root();
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
void keep_waking(const std::vector<Thr> &handles)
{
    // EVERY THREAD AT ONCE, AND NOT ONE AFTER ANOTHER -- found by T1's review
    // and reproduced by tests/thread_test/programs/join_sleeping_child.satl.
    // Waking in launch order hung the run: a parent joining its sleeping child
    // comes first, cannot end until the child does, and the child was never
    // signalled because the loop was still waiting on the parent.
    for (;;) {
        bool all_ended = true;
        for (const Thr &handle : handles) {
            // UNDER THE LOCK, WHICH IS WHAT MAKES THE SIGNAL SAFE -- THREAD.md
            // D5. The note above this used to say the worst case was ESRCH;
            // that holds only until somebody joins, and after a join the
            // pthread_t may name nothing or another thread. A thread that has
            // not ended cannot have been joined, and `ended` is written under
            // this lock, so the signal goes only to a thread that exists.
            std::lock_guard<std::mutex> held(handle->lock);
            if (handle->ended || handle->failed)
                continue;
            all_ended = false;
            if (handle->worker.joinable())
                pthread_kill(handle->worker.native_handle(), kWake);
        }
        if (all_ended)
            return;
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

    eval::Policy mine = handle->policy;
    mine.interrupted = stopped_or_interrupted;

    eval::Machine machine(*handle->program, *handle->ast, mine,
                          handle->globals);

    // THE TOP LEVEL IS NOT RE-RUN, AND THAT IS THE POINT OF SHARING THE
    // GLOBALS. `satellite.library`'s initialisers ran once, on the main walk,
    // before any capsule did -- DESIGN §7.2's order. A thread that ran them
    // again would reset every global the program had changed since, which is a
    // program silently losing work rather than a race.
    Value answer = machine.call(handle->body->capsule, handle->body->arguments);

    // PUBLISHED UNDER THE LOCK AND ALL AT ONCE. A joiner waits on `ended`, so
    // everything it reads is written before it can wake; `finished` goes last
    // for the renderer, which reads it without the lock.
    std::lock_guard<std::mutex> held(handle->lock);
    handle->answer = std::move(answer);
    handle->problems = machine.problems();
    handle->ending = machine.ending();
    handle->ended = true;
    handle->finished.store(true, std::memory_order_release);
    handle->changed.notify_all();
}

// EXACTLY ONE std::thread::join(), WHOEVER ASKS -- THREAD.md D5 and D9. The
// first caller to find the thread ended claims the join and does it without
// the lock; every other caller, a program's second join() or close_all(),
// waits for `reaped`. Answers false for a thread the machine never made.
bool reap(const Thr &handle)
{
    std::unique_lock<std::mutex> held(handle->lock);
    handle->changed.wait(held, [&] { return handle->ended || handle->failed; });
    if (!handle->ended)
        return false;
    if (handle->claimed) {
        handle->changed.wait(held, [&] { return handle->reaped; });
        return true;
    }
    handle->claimed = true;
    held.unlock();
    handle->worker.join();
    held.lock();
    handle->reaped = true;
    handle->changed.notify_all();
    return true;
}

} // namespace

bool (*interrupt_root(const eval::Policy &policy))()
{
    // A THREAD'S WALK HAS stopped_or_interrupted AS ITS HOOK, so a thread made
    // on a thread takes the root its maker was given; the program's own walk
    // has no `self` and its hook is the root.
    return self != nullptr ? self->root : policy.interrupted;
}

bool launch(const Thr &handle, std::string &why)
{
    // THE WALK THAT STARTS IT IS ITS PARENT, whoever made the handle. A handle
    // made on the main walk and started by a worker is closed with the worker.
    if (self != nullptr)
        handle->parent = self->shared_from_this();

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

    // THE WORKER IS ASSIGNED UNDER THE HANDLE'S LOCK, so close_all() or a
    // joiner on another thread never reads it half-written, and the child --
    // which takes the lock to publish its answer -- cannot end before it is.
    std::unique_lock<std::mutex> held(handle->lock);
    try {
        handle->failed = false;
        handle->worker = std::thread(run_the_body, handle);
    } catch (const std::system_error &refused) {
        // THE MACHINE SAID NO. std::thread's constructor is the one thing in
        // this tree that throws, which machine_limits/pool.cpp already had to
        // know -- it catches the same exception for the same reason and calls a
        // pool that could not be fully built "a SMALLER pool". There is no
        // smaller answer available here: the program asked for a thread and
        // there is not one, so it is told, with the operating system's own
        // sentence attached. Any joiner already waiting is woken to S1403.
        handle->failed = true;
        handle->started.store(false, std::memory_order_relaxed);
        handle->changed.notify_all();
        held.unlock();
        why = refused.what();
        std::lock_guard<std::mutex> in(registry().lock);
        for (size_t i = registry().live.size(); i > 0; i--)
            if (registry().live[i - 1] == handle) {
                registry().live.erase(registry().live.begin() +
                                      static_cast<long>(i - 1));
                break;
            }
        return false;
    }
    return true;
}

// WHO IS WAITING FOR WHOM -- found by T1's review, and reproduced by
// tests/thread_test/programs/join_itself.satl. A thread that reached its own
// handle and joined it waited for ever, and so did two threads joining each
// other; std::thread::join() used to throw for the first, and nothing caught
// the second. So every thread's join records the handle it waits on, under
// one lock, and a join whose chain of waits leads back to the asker is
// refused before it sleeps. The program's own walk is never waited on -- no
// thread can hold a handle to it -- so it records nothing.
std::mutex &waits()
{
    static std::mutex the;
    return the;
}

Joined wait(const Thr &handle)
{
    if (self != nullptr) {
        std::lock_guard<std::mutex> held(waits());
        for (ThreadHandle *at = handle.get(); at != nullptr; at = at->waiting_on)
            if (at == self)
                return Joined::WouldNeverReturn;
        self->waiting_on = handle.get();
    }
    struct Unrecord {
        ~Unrecord()
        {
            if (self == nullptr)
                return;
            std::lock_guard<std::mutex> held(waits());
            self->waiting_on = nullptr;
        }
    } unrecord;

    // `joined` IS TAKEN BEFORE THE WAIT, so of two joins racing, exactly one
    // is First whatever the timing -- Q2's "exactly", kept.
    bool again = false;
    {
        std::lock_guard<std::mutex> held(handle->lock);
        again = handle->joined;
        handle->joined = true;
    }

    if (!reap(handle)) {
        std::lock_guard<std::mutex> held(handle->lock);
        handle->joined = again;
        return Joined::NeverRan;
    }
    if (again)
        return Joined::Again;

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
    return Joined::First;
}

std::vector<errors::Diagnostic> close_all()
{
    std::vector<errors::Diagnostic> unreported;

    // UNTIL THE REGISTRY IS EMPTY, AND NOT ONE PASS -- THREAD.md D6. A thread
    // being closed can still reach a start() before its next statement
    // boundary, and that thread arrived after the swap: one pass returned, the
    // arena was destroyed, and the new thread went on walking it. Now the new
    // thread's parent is stopped, so it ends at its first statement, and the
    // next pass reaps it.
    for (;;) {
        // TAKEN OUT FROM UNDER THE LOCK FIRST, AND NOT WALKED UNDER IT. A child
        // still running may be inside `wait()` on a handle of its own, and
        // joining while holding this lock would be this function waiting for a
        // thread waiting for this lock.
        std::vector<Thr> taken;
        {
            std::lock_guard<std::mutex> held(registry().lock);
            taken.swap(registry().live);
        }
        if (taken.empty())
            break;

        for (const Thr &handle : taken)
            handle->stop.store(true, std::memory_order_relaxed);

        // THE FLAG, THEN THE ALARM CLOCK, THEN THE WAIT -- in that order,
        // because the flag is what the thread acts on and the signal only makes
        // it look.
        keep_waking(taken);

        for (const Thr &handle : taken) {
            if (!reap(handle))
                continue;

            // ASKED AFTER THE JOIN, WHERE IT IS A FACT. `Interrupted` is the
            // ending this function caused -- MILESTONES/M23.md §3.5 has the race
            // that reading `finished` beforehand had.
            std::lock_guard<std::mutex> held(handle->lock);
            if (!handle->joined && handle->ending != eval::Ending::Interrupted)
                for (const errors::Diagnostic &problem : handle->problems)
                    unreported.push_back(problem);
        }
    }
    return unreported;
}

} // namespace satellite::thread
