// The pool: that it builds, that it parks, that the floor is enforced, and that
// a batch computes the right answer.
//
// THIS SUITE IS THE POOL'S ONLY CALLER, AND THAT IS SAID OUT LOUD RATHER THAN
// LEFT TO BE NOTICED. PLAN M6's own line is that `parallel_for` does not exist
// -- it is in no numbering, no document and no milestone -- so the construct
// the pool was built for has not been designed. Its tenants are M10's printer
// thread, M22's prompt and M23's threads. A batch runner nothing has ever run
// is a batch runner that does not work, which is the same argument
// reporter_test makes for rendering a call stack no pass produces.
//
// AND IT IS THE ONE SECTION THAT WAITS. The pool takes ~590 us to finish
// building (PLAN §4.5.1.1) and every satl command finishes before it does --
// `satl --words` reproducibly ends with 3 of 24 parked -- so a test that
// asserted "24 parked" without waiting would be asserting a race. What it waits
// for is a CONDITION and not a duration, which is what keeps it honest on a
// slower machine.
//
// (3 and not the 4 this said until 2026-08-31, and the count went UP for
// `satl --limits` rather than down: 14 to 19 where it was reproducibly 0. The
// pool now starts EARLIER, because limits.cpp no longer spends 0.42 ms reading
// the machine before it gets to pool::start(). None of it changes the reason
// this section waits.)

#include "limits_test.hpp"

#include "machine_limits/pool.hpp"
#include "system_facts/facts.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <thread>
#include <vector>

namespace limits_test {

namespace pool = satellite::limits::pool;

namespace {

// Wait until every thread the pool was asked for is parked, or give up.
//
// A CONDITION AND NOT A SLEEP. A fixed wait long enough for this machine is
// either flaky on a loaded one or slow on every run of a fast one; polling a
// condition is neither. The ceiling is generous -- 5 s against a measured
// 590 us -- because what it is protecting against is a HANG, not slowness.
bool parks_fully()
{
    for (int i = 0; i < 5000; i++) {
        if (pool::parked() == pool::wanted())
            return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
}

} // namespace

void section_pool()
{
    // The pool is not started by limits::begin() here -- that is satl's
    // startup and this binary is not satl. Starting it directly is also what
    // makes the size a choice this test can make.
    const unsigned threads = 8;
    pool::start(threads);

    check(pool::wanted() == threads, "the pool was asked for what it was told");

    // --- A BATCH BEFORE THE POOL EXISTS -----------------------------------
    //
    // RUN FIRST, WITH NO WAIT IN FRONT OF IT, and that ordering is the check.
    // This is satl's actual situation and not a corner: §4.5.1.1 measured the
    // pool at ~590 us to build and every command in this tree finishes before
    // it does, so the FIRST caller of run_over() will always be racing a pool
    // that is part built. The obvious batch runner -- split, wake, do one
    // chunk, wait -- is correct only if somebody else is awake to take the
    // rest; run_over()'s caller keeps taking until there are none left, so it
    // is correct with zero workers awake.
    //
    // A wait moved in front of this would make it a different test, and the
    // suite would stop covering the only case that happens today. Said here
    // because that wait is exactly what somebody adds to make a flaky-looking
    // test settle down.
    std::atomic<size_t> early{0};
    pool::run_over(pool::kFloor * 4, [&](size_t from, size_t to) {
        check(from <= to, "no chunk runs backwards, even before the pool fills");
        early += to - from;
    });
    check(early == pool::kFloor * 4,
          "a batch handed to a pool that is still being built still gets every "
          "unit of its work done, and does not wait for a thread that does not "
          "exist yet");

    check(parks_fully(),
          "and every one of them is built and parked -- the main thread spawns "
          "ONE thread which builds the rest (PLAN §4.5.1.2), so a pool that "
          "never fills is a builder that died rather than a pool that is slow");

    // --- the floor --------------------------------------------------------
    //
    // §4.5.1.2 CALLS THIS "a rule the pool's owner has to enforce rather than a
    // suggestion", and the number is §4.5.1.1's second-batch crossover: waking
    // 24 parked threads costs ~47 us, so at 80 units the pooled arm is still
    // 0.48x -- it takes twice as long as doing the work.
    check(!pool::would_thread(0), "no work is not worth waking anybody for");
    check(!pool::would_thread(1), "and neither is one unit");
    check(!pool::would_thread(pool::kFloor - 1),
          "nor one unit below the floor");
    check(pool::would_thread(pool::kFloor),
          "the floor itself is worth it, and the boundary is where an "
          "off-by-one lives");
    check(pool::would_thread(pool::kFloor * 100), "and anything above it is");

    // --- a batch, below the floor -----------------------------------------

    std::atomic<size_t> touched{0};
    unsigned ran = pool::run_over(10, [&](size_t from, size_t to) {
        touched += to - from;
    });
    check(ran == 1,
          "below the floor the work runs on ONE thread -- the caller's -- and "
          "the pool is not touched at all, so a small batch never contends "
          "with a pool that is still being built");
    check(touched == 10, "and every unit of it is still done");

    // --- a batch, above the floor -----------------------------------------
    //
    // EVERY UNIT EXACTLY ONCE IS THE WHOLE CONTRACT, and it is what a chunking
    // bug breaks in the two directions a count alone cannot tell apart: a unit
    // done twice and a unit not done at all sum to the same total. So this
    // marks each one.
    const size_t units = 100000;
    std::vector<unsigned char> marks(units, 0);
    ran = pool::run_over(units, [&](size_t from, size_t to) {
        for (size_t i = from; i < to; i++)
            marks[i]++;
    });

    size_t once = 0;
    size_t wrong = 0;
    for (const unsigned char mark : marks) {
        if (mark == 1)
            once++;
        else
            wrong++;
    }
    check(once == units && wrong == 0,
          "every unit of a threaded batch is done EXACTLY once -- a unit done "
          "twice and a unit missed sum to the same total, so a count alone "
          "cannot see either");
    check(ran > 1,
          "and more than one thread did a piece of it, which is the claim the "
          "floor exists to make true only above it");
    check(ran <= threads + 1,
          "and no more than the pool plus the caller, who takes a share");

    check(parks_fully(),
          "and they all park again afterwards, which is what makes the SECOND "
          "batch the cheap one -- PLAN §4.5.1's whole argument for a pool is "
          "amortisation across a run, not a cheaper first parse");

    // --- a batch of one unit above the floor -------------------------------
    //
    // The chunk is what is rounded up and the count is derived back from it,
    // so no chunk can begin past the end. Written the other way round -- one
    // chunk per thread, rounded up -- 10 units over 9 threads gives 9 chunks of
    // 2 and the last four begin at or past unit 10.
    std::atomic<size_t> small{0};
    pool::run_over(pool::kFloor, [&](size_t from, size_t to) {
        check(from <= to, "no chunk ever runs backwards");
        small += to - from;
    });
    check(small == pool::kFloor,
          "a batch barely over the floor splits into chunks that cover it "
          "exactly, with none beginning past the last unit");
}

} // namespace limits_test
