#pragma once

// The thread pool -- PLAN §4.5.1.2, which is the author's decision and is the
// specification rather than advice.
//
//     "satl almost must start the THREAD_COUNT in satellite_config.ini because
//      we almost have to assume the user will call parallel_for, so that
//      requires a warm pool no matter what."
//
// SO IT STARTS AT STARTUP, ALWAYS, AND NOT ON A SIZE TRIGGER. §4.5.1.1 measured
// a lazy pool and an eager one and proposed choosing between them by the size of
// the source; §4.5.1.2 threw that away, and the reason is the one that matters:
// a size trigger predicts PARSE cost, the pool's real tenant is the RUNNING
// PROGRAM, and ten lines can run a million-iteration loop. It is also the kind of
// hidden threshold DESIGN §1.1 refuses.
//
// WHAT IT COSTS A PROGRAM THAT NEVER THREADS, measured on this machine
// 2026-08-28 and quoted from §4.5.1.2 because it is the argument for the
// decision: +21 us at 100 satellite-rooted lines, +47 us at 500, +182 us at
// 1,000 -- against satl's whole 1.75 ms startup, so 1-3% of it.
//
// TWO IMPLEMENTATION REQUIREMENTS, BOTH MEASURED, AND BOTH ARE BUILT HERE.
//
//   1. THE MAIN THREAD MUST NOT BUILD THE POOL ITSELF. start() spawns ONE
//      thread and that thread builds the other THREAD_COUNT-1: ~20 us on the
//      main thread instead of the ~590 us it costs to build 23 yourself.
//
//   2. BELOW ~170 UNITS THE WORK STAYS ON THE CALLER'S THREAD. §4.5.1.2 calls
//      this "a rule the pool's owner has to enforce rather than a suggestion",
//      and the number is the second-batch crossover from §4.5.1.1: waking 24
//      parked threads costs ~47 us, so at 80 lines the pooled arm is still
//      0.48x -- it takes twice as long as doing the work. would_thread() is
//      that rule and run_over() obeys it.
//
// `parallel_for` DOES NOT EXIST, AND NAMING THAT IS THE HONEST FORM OF THE
// DEPENDENCY. PLAN M6 says so in as many words: the decision above rests on a
// construct that is in no numbering, no document and no milestone. The
// language's whole parallelism surface today is `satellite.thread.new` `1 23 1`
// and `satellite.variable.thread.start()` / `.join()` `1 6 13 1`-`1 6 13 2`,
// all three BUILT at M23 and none of them through here.
// The pool is still right without it for the reason §4.5.1 gives on its own
// terms -- amortisation across a run, the second tenant onward -- and run_over()
// is the C++ shape that construct will be built on rather than the construct.
//
// AND M23 IS NOT A TENANT AFTER ALL, WHICH THE RULE ABOVE IS THE REASON FOR AND
// DOES NOT CATCH. "Does the work end" is necessary and not sufficient: a
// satellite thread's capsule finishes, so it passes -- and the half the test is
// missing is AND DOES THE CALLER WAIT FOR IT. run_over() returns when the last
// chunk lands; `my_thread.start()` has to return before the work starts. The two
// are opposites, and underneath that is the objection that settles it: a pool has
// THREAD_COUNT workers, so a program starting one more thread than that, each
// joining the next, would wait forever -- a ceiling on the language wearing an
// optimisation's clothes. M23 makes a fresh std::thread per start() instead, at
// about 28 us created-run-joined. PLAN §4.5.1 and §8's M23 entry carry the same
// correction; MILESTONES/M23.md §2.4 is the argument. THE TEST FOR THE NEXT
// CANDIDATE IS NOW TWO QUESTIONS.
//
// SO THE ONLY CALLER OF run_over() AT M6 IS tests/limits_test, and that is said
// out loud rather than left to be noticed, the way 040-sources.mk had to say it
// about satellite_random. The pool's tenants are M25's second file, M22's prompt
// parsed repeatedly, parse-time interning and whatever `parallel_for` becomes;
// none of them exists. (This list named M23's threads until 2026-09-12 -- see the
// correction above, which is where the reason is.) A batch runner nothing has
// ever run is a batch runner that does not work, so the suite runs real batches
// through it.
//
// THIS LIST NAMED "M10's PRINTER THREAD" FIRST UNTIL 2026-09-02, AND THE RULE
// THAT REPLACES IT IS ONE LINE: THIS POOL TAKES WORK THAT FINISHES.
//
// run_over() is a range, a split and a join -- it returns when the last chunk
// lands. The console's two threads exist precisely because they do NOT finish.
// The printer waits on a queue for the life of the run (the author, 2026-09-02:
// the plan was always that the printer would create its own thread), and DESIGN
// §10.1's reader "blocks on stdin", which is worse than a mis-count: a worker
// sitting in read() never comes back for a chunk, so run_over() waits forever.
// Neither was ever a scheduling question and neither is a tenant.
//
// THE TEST TO APPLY TO THE NEXT CANDIDATE is not "does it want a thread" but
// "does the work end". A thread that outlives every batch cannot be lent by
// something that counts what it has lent out -- wanted() and parked() would go
// on counting it and `satl --limits` would go on reporting it, which is §4.5.2's
// whole job done wrong. PLAN §4.5.1 has the correction and MILESTONES/M6.md §8
// has what it costs the argument for building this at all.

#include <cstddef>
#include <functional>

namespace satellite::limits::pool {

// The floor, in units of work. §4.5.1.1's second-batch crossover.
inline constexpr size_t kFloor = 170;

// Spawn one thread, which builds the other `threads - 1`. Called once, from
// limits::begin(). A second call does nothing.
void start(unsigned threads);

// How many threads the pool was asked for -- THREAD_COUNT, or the machine's
// answer when no file said.
unsigned wanted();

// How many exist and are waiting for work RIGHT NOW.
//
// A SAMPLE AND NOT A GUARANTEE, and that is the interesting part rather than a
// weakness in it. The pool takes ~590 us to finish building, and satl's whole
// run of `--words` is ~766 us, so this answers a number that is still climbing
// -- which is exactly §4.5.1.1's finding ("by the time the pool is ready, a
// single thread has already walked ~2,150 lines") visible from the outside.
// `satl --limits` and `satl --words` both print it and both say when it was
// taken.
unsigned parked();

// Whether `units` of work is worth waking the pool for. Requirement 2 above.
bool would_thread(size_t units);

// Run `body(from, to)` over [0, units), split across the pool when that is
// worth it and run on this thread when it is not. Returns how many threads
// actually ran a piece of it, which is 1 when the floor refused the work.
//
// THE CALLER TAKES A SHARE AND KEEPS TAKING SHARES, which is what makes this
// free of the deadlock the obvious version has: if the pool is still being
// built and no worker is awake to answer, the caller does every chunk itself
// and waits for nothing. There is no state in which run_over() can block on a
// thread that does not exist yet.
//
// ONE BATCH AT A TIME. Two threads calling this at once are serialised. Nothing
// nests batches today and nothing should: a body that called run_over() would
// wait for a pool whose threads are all inside the body.
//
// `body` MUST NOT THROW. It is run on a detached thread with no handler above
// it, so an escaping exception is std::terminate. Nothing in this tree throws.
unsigned run_over(size_t units, const std::function<void(size_t, size_t)> &body);

} // namespace satellite::limits::pool
