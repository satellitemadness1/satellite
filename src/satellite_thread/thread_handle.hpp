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
#include "satellite_value/value.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
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
};

// A THREAD -- `satellite.variable.thread` `1 6 13`.
//
// WHICH FIELDS ARE ATOMIC AND WHY, which satellite_file/file_handle.hpp asked
// for by name when it said "M23 is when it first gets EXERCISED, not when it
// gets written".
//
//   started_   read by the parent to refuse a second start(), written once by
//              the parent before the child exists. Atomic so that a handle
//              shared between two threads -- `t` passed to another capsule
//              running on a third thread -- cannot see a torn answer.
//   finished_  written by the CHILD as its last act, read by anyone. This is
//              the only field with a writer that is not the parent.
//   stop_      written by close_all() on the main thread, read by the child at
//              every statement boundary through the Policy's interrupted hook.
//
// `answer` AND `problems` ARE NOT ATOMIC AND DO NOT NEED TO BE, because the
// only thing that reads them is a join(), and std::thread::join() is a
// synchronisation point: everything the child wrote before it ended happens-
// before everything the joiner does after. That is the same argument
// file_handle.hpp makes for its buffer and is the reason a mutex here would be
// a mutex around a fence.
struct ThreadHandle {
    Cap body;

    // THE WORLD THE CHILD WALKS. Pointers and not copies: the arena and the ast
    // are immutable for the length of the run and shared by every walk, which
    // is PLAN §2.2's whole argument for the arena arriving before threads did.
    const eval::Compiled *program = nullptr;
    const Ast *ast = nullptr;
    std::shared_ptr<eval::Globals> globals;
    eval::Policy policy;

    std::thread worker;

    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};
    std::atomic<bool> stop{false};

    // JOINED IS THE PARENT'S ALONE and is deliberately not atomic. Only the
    // walk that calls join() writes it, and a second join() from a second
    // thread is a program that has already lost its own race -- S1404 catches
    // the ordinary mistake, which is a join() inside a loop.
    bool joined = false;

    Value answer;
    std::vector<errors::Diagnostic> problems;

    // HOW THE WALK ENDED, AND IT IS WHAT close_all() READS TO DECIDE WHETHER A
    // REFUSAL IS WORTH REPORTING. Stopping is not failing: a thread the run
    // closed ends Interrupted and was doing nothing wrong, while one that
    // refused on its own ends Refused and has a sentence somebody needs.
    //
    // AND IT REPLACED A `finished` PRE-CHECK THAT WAS A RACE. The first version
    // asked whether the thread had finished BEFORE raising `stop`, and a thread
    // that was one microsecond from refusing was recorded as "we stopped it"
    // and had its diagnostic thrown away. The ending is the same question asked
    // after the join, where the answer is a fact rather than a sample.
    // MILESTONES/M23.md §3.5.
    eval::Ending ending = eval::Ending::Finished;
};

// WHAT `start()` DOES, AND THE ONE PLACE AN OS THREAD IS MADE. Answers false
// when the machine would not make one, with the machine's own reason in
// `why` -- S1405's {1}.
bool launch(const Thr &handle, std::string &why);

// WHAT `join()` DOES. Waits, then hands back what the capsule returned. The
// caller has already checked that there is something to wait for.
void wait(const Thr &handle);

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
