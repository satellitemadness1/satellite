#pragma once

// `satellite.variable.thread` and `satellite.variable.capsule` -- M23, the two
// bodies behind satellite_value/value.hpp's tenth and eleventh arms.
//
// DESIGN §10.5 IS THREE SENTENCES AND THEY ARE THE WHOLE DESIGN: "§7's frames
// are what make a capsule call safe to run on one; PLAN.md §2.2's arena is what
// makes walking the program ATOMIC-FREE rather than merely safe." Both halves
// were built years of milestones before there was a thread to prove them on --
// a local is a frame slot, reachable from one walk by construction, and the op
// arena is immutable with its one mutable part (the inline cache) already in a
// side table. So what this file adds is not a threading model. It is a handle.
//
//     satellite.variable.thread t = satellite.thread.new(capsule_test(word))
//     t.start()
//     t.join()
//
// A THREAD IS A REFERENCE TYPE AND A DEFERRED CALL IS A VALUE, WHICH IS THE ONE
// DISTINCTION THIS FILE EXISTS TO KEEP. Two names for one thread are one
// thread: `join()` through either waits for the same `pthread_t`, because the
// kernel has never heard of our slots. Two names for one deferred call are two
// values holding the same frozen `(capsule, arguments)`, and handing it to two
// threads is two runs of one capsule rather than one run seen twice. That is
// why `Thr` comes off the `const` in value.hpp and `Cap` does not.
//
// WHERE THE OS THREAD COMES FROM, AND IT IS NOT M6's POOL. The author settled
// this on 2026-09-12 and MILESTONES/M23.md §2.4 has the argument; the short
// form is that machine_limits/pool.hpp's test for a tenant -- "does the work
// end" -- is necessary and not sufficient. The half it is missing is AND DOES
// THE CALLER WAIT FOR IT. `run_over()` is a range, a split and a join: it
// returns when the last chunk lands, which is the exact opposite of what
// `start()` has to do. Underneath that is a worse problem: a pool has
// THREAD_COUNT workers, so a program starting one more thread than that, each
// waiting on the next, would wait forever -- a ceiling on the language wearing
// an optimisation's clothes. PLAN §4.5.1's tenant list is wrong about M23 and
// says so now.
//
// SO A FRESH std::thread EACH TIME, and no ceiling of satellite's own on how
// many. When the machine refuses one, S1405 reports the MACHINE's reason --
// `ulimit -u`, a cgroup pids limit, no memory -- rather than a number satellite
// chose. SCRATCH.md/NO_LIMITS.md §1.2 is the rule and this is what keeping it
// costs: about 20 us per `start()`, measured in M23.md §5.
//
// WHAT A THREAD SHARES WITH THE WALK THAT STARTED IT is machine.hpp's second
// constructor and is exactly three things: the op arena, the syntax tree, and
// `satellite.library`. Everything else -- four stacks, the inline caches, the
// dials, the search threshold, the problems -- is the child's own.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "evaluator/closure.hpp"
#include "evaluator/globals.hpp"
#include "evaluator/machine.hpp"
#include "satellite_thread/access_list.hpp"
#include "satellite_value/value.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace satellite::thread {

// A CALL THAT HAS NOT HAPPENED -- `satellite.variable.capsule` `1 6 16`.
//
// FROZEN AT CONSTRUCTION, which is what makes it a value and not a closure.
// DESIGN §13: "the handler evaluates the arguments and stores (capsule number,
// argument values) for the thread to run later." The arguments in here were
// computed on the calling thread before this object existed; there is no
// environment captured, no slot reachable, and nothing to re-evaluate. That is
// also the sentence that keeps DESIGN §12's deferral of "a bare name can be a
// value" shut: what the program wrote was a CALL.
struct Deferred {
    // WHICH COMPILED CAPSULE, as an index into Compiled::capsules(). Not a
    // PathId: words_runtime.hpp warns that "a user's PathId is valid inside one
    // run only" and a capsule's name is exactly such a path, so the index --
    // decided in the compiler's first pass, before any body was compiled -- is
    // the thing that means something for the length of a run.
    uint32_t capsule = 0;

    std::vector<Value> arguments;

    // WHAT THE PROGRAM WROTE, for a sentence to quote. A refusal that says
    // `capsule_test` is worth the string; the alternative is walking the ast
    // back from the capsule index inside a diagnostic, which is work done on
    // the failure path to save bytes on the working one.
    std::string name;

    // value.hpp's Burial -- THREAD.md D19. The arguments may be a list nested
    // as deep as any other.
    ~Deferred()
    {
        Burial burial;
        for (Value &argument : arguments)
            burial.add(argument);
    }
};

// A THREAD -- `satellite.variable.thread` `1 6 13`.
//
// WHICH FIELDS ARE GUARDED BY WHAT -- REWRITTEN AT THREAD.md T1. The M23 note
// here said `joined`, `answer` and `problems` needed no lock because only the
// parent joins, and that "a second join() from a second thread is a program
// that has already lost its own race". THREAD.md D4, D5 and D9 were that
// sentence being wrong: a handle is a value, values are passed to threads, and
// two threads joining one thread hung for ever inside std::thread::join(). So:
//
//   started   atomic, and start() takes it with `exchange` -- D4: two threads
//             starting one handle both passed a load() and the second
//             assignment to a joinable std::thread was std::terminate.
//   finished  atomic, for the renderer, which reads it without the lock.
//   stop      atomic, read at every statement boundary of this thread AND of
//             every thread it started (D8's parent chain).
//
//   everything below `lock` is written and read under it. `ended` is what
//   waiters wait on; `claimed` makes std::thread::join() happen exactly once
//   whoever asks (D5, D9); `reaped` is what every other waiter waits on,
//   which is also what makes pthread_kill safe: it is only sent while the
//   thread has not ended, and a thread that has not ended has not been joined.
struct ThreadHandle : std::enable_shared_from_this<ThreadHandle> {
    Cap body;

    // THE WORLD THE CHILD WALKS. Pointers and not copies: the arena and the ast
    // are immutable for the length of the run and shared by every walk, which
    // is PLAN §2.2's whole argument for the arena arriving before threads did.
    const eval::Compiled *program = nullptr;
    const Ast *ast = nullptr;
    std::shared_ptr<eval::Globals> globals;
    eval::Policy policy;

    // THE PROCESS'S OWN INTERRUPT HOOK -- Ctrl-C -- and never a thread's.
    // THREAD.md D8: a thread started by a thread copied its parent's policy,
    // whose hook was the parent's `stopped_or_interrupted`, and the child then
    // asked itself at every statement boundary for ever. The root is decided
    // once, at `new`, by interrupt_root() below.
    bool (*root)() = nullptr;

    // THE THREAD WHOSE WALK CALLED start(), or null for the program's own
    // walk. Its `stop` stops this one too, so closing a thread closes what it
    // started -- and a thread started while the run is closing stops at its
    // first statement, which is what makes close_all() final (D6).
    std::shared_ptr<ThreadHandle> parent;

    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};
    std::atomic<bool> stop{false};

    std::mutex lock;
    std::condition_variable changed;

    std::thread worker;
    bool ended = false;          // the body returned; answer/problems are set
    bool failed = false;         // the machine would not make the thread
    bool claimed = false;        // somebody is inside worker.join()
    bool reaped = false;         // worker.join() has returned
    bool joined = false;         // a PROGRAM has called join() on this thread

    // THIS THREAD'S RECORD IN THE WAIT GRAPH -- THREAD.md T2. Its walk uses
    // it (Machine::wait_as), so a joiner can point at it and the deadlock
    // check can follow a join into whatever this thread is waiting for.
    ThreadWait wait_record;

    Value answer;
    std::vector<errors::Diagnostic> problems;

    // HOW THE WALK ENDED, AND IT IS WHAT close_all() READS TO DECIDE WHETHER A
    // REFUSAL IS WORTH REPORTING. Stopping is not failing: a thread the run
    // closed ends Interrupted and was doing nothing wrong, while one that
    // refused on its own ends Refused and has a sentence somebody needs.
    // MILESTONES/M23.md §3.5.
    eval::Ending ending = eval::Ending::Finished;

    // TWO THINGS NESTED DEEP ENOUGH TO CRASH ON THE WAY OUT -- THREAD.md D19.
    // The answer is buried like any value. The PARENT chain is unlinked in a
    // loop: a thread that starts its successor and ends, a million times over,
    // leaves a million handles each holding the one before, and letting the
    // last go freed them one C++ frame per generation.
    ~ThreadHandle()
    {
        {
            Burial burial;
            burial.add(answer);
        }
        std::shared_ptr<ThreadHandle> up = std::move(parent);
        while (up && up.use_count() == 1) {
            std::shared_ptr<ThreadHandle> next = std::move(up->parent);
            up = std::move(next);
        }
    }
};

// THE HOOK A NEW THREAD'S `root` IS: the process's, whichever walk asks.
// `policy` is the asking walk's own.
bool (*interrupt_root(const eval::Policy &policy))();

// WHAT `start()` DOES, AND THE ONE PLACE AN OS THREAD IS MADE. The caller has
// already taken `started`. Answers false when the machine would not make one,
// with the machine's own reason in `why` -- S1405's {1}.
bool launch(const Thr &handle, std::string &why);

// HOW A join() CAME OUT.
enum class Joined {
    First,     // this was the join -- the program's first
    Again,     // somebody had already joined it: S1404, the same answer
    NeverRan,  // start() was asked and the machine refused: S1403
    WouldNeverReturn,  // the thread waits, through joins, for the asker: S1407
};

// WHAT `join()` DOES. Waits until the thread has ended and been reaped --
// exactly one std::thread::join() across every caller -- then says which join
// this was. `answer` and `problems` are final once it returns.
Joined wait(const Thr &handle, ThreadWait *me);

// EVERY THREAD THIS RUN STARTED AND NOBODY JOINED, CLOSED.
//
// THE AUTHOR'S DECISION OF 2026-09-12: "whenever the program reaches
// `satellite.return(satellite)`, close everything." So this is not a wait. It
// raises `stop` on every live handle -- which the child sees at its next
// statement boundary, through the same hook M11's Ctrl-C uses -- and then joins
// each one, so nothing is still walking an arena that is about to be destroyed.
//
// STOPPING IS NOT FAILING, and the return value is where that shows. A thread
// this function stopped ends Interrupted and its diagnostics are discarded.
// What comes back is the problems of threads that REFUSED on their own and were
// never joined by anybody -- errors a program made that would otherwise be
// silent, which is what DESIGN §1.1 will not have.
//
// EVERY ENTRY POINT THAT RUNS A PROGRAM MUST CALL THIS, AND THAT IS A RULE
// ABOUT MEMORY BEFORE IT IS A RULE ABOUT ERRORS. A thread walks `Compiled` and
// `Ast` by pointer, and both are locals of whatever built the program; a run
// that returns while a thread is still walking them destroys the arena under
// it. It is system_facts/interrupt.hpp's rule one module over -- "call it from
// every entry point that can run a program" -- and the three sites are
// programs/run_command.cpp, programs/evaluate_commands.cpp and
// satellite_prompt/session.cpp's two, plus tests/eval_test's `call`.
//
// FOUND BY A SEGFAULT AND NOT BY READING. A fixture that started a thread and
// then refused -- `t.start()` twice -- returned without closing anything, and
// the next thing to touch the freed arena was the thread. MILESTONES/M23.md
// §3.6.
//
// AND IT IS CALLED ONCE PER RUN AND NOT ONCE PER PROCESS, because M22's prompt
// runs many programs in one process and a thread from the last one must not be
// waiting in this one's registry -- nor walking the line before's arena.
std::vector<errors::Diagnostic> close_all();

} // namespace satellite::thread
