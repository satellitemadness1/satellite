#pragma once

// The console -- PLAN M10, and DESIGN §10.1 is the specification.
//
// "Producer threads push whole strings into a locked queue; one printer thread
// consumes. A LINE STAYS ATOMIC BECAUSE THE UNIT QUEUED IS A WHOLE STRING.
// There is a `drain()` barrier before reading input, so a prompt cannot appear
// before the output that explains it."
//
// THE PRINTER CREATES ITS OWN THREAD AND DOES NOT TAKE ONE FROM M6. The author,
// 2026-09-02: "the plan was always that the printer would create its own
// thread." Six places in the documents had said the opposite until that day and
// the correction is a category one rather than a scheduling one --
// machine_limits/pool.hpp holds it in one line: THAT POOL TAKES WORK THAT
// FINISHES. `run_over()` is a range, a split and a join; a printer waits on a
// queue for the life of the run, so a resident tenant would hold a worker
// forever and `satl --limits` would go on reporting it as available. So this
// file includes no pool.hpp, and the test for the next candidate tenant is not
// "does it want a thread" but "does the work end".
//
// THE BARRIER IS A PREFIX OF THE SHUTDOWN AND NOT A SECOND MECHANISM. The
// author, 2026-09-02: "the block is similar to a shutdown -- combine shutdown
// with the block." Four steps, in this order:
//
//     drain    wait until the printer's queue is empty
//     flush    fflush the fd -- DESIGN §10.1, because glibc buffers fully to a
//              pipe or a file and line-buffers only to a tty
//     stop     close the queue to new work
//     join     the printer thread ends
//
// `satellite.console.input` `1 5 2`-`1 5 4` takes step 1 and carries on (M14);
// `satellite.return` from `satellite.main` takes all four. WHICH IS ALSO WHY
// THE BARRIER IS NOT SPECULATIVE three milestones before anything reads input:
// the same wait is on the exit path of every program that prints.
//
// AND `display` WAITS WHEN THE WRITER IS BEHIND, WHICH REVERSES WHAT STOOD
// HERE. The paragraph this replaces said the queue was unbounded on purpose,
// that a high-water mark was "a threshold nobody chose appearing in the
// most-called path", and that "the bound is memory, the way a list's is, and
// M6's watchdog is what notices." The first two are still fair. The third was
// measured on 2026-09-17 and is not true:
//
//     experiments/energy/release.satl prints in a loop with no sleep in it.
//     Through satl-term the `satl` child grew 125 MB EVERY SECOND -- 5.9 GB in
//     48 seconds -- and satl-term itself stayed flat at 164 MB. Every one of
//     those bytes was a line already rendered and waiting for a terminal that
//     draws at 60 Hz. The same program writing to a FILE holds 4.4 MB flat,
//     because there the printer keeps up.
//
// The watchdog does not notice, and that is not a bug in the watchdog: its
// `memory_max` defaults to `Fact::MemoryTotal`, so on this machine it is armed
// at 61.9 GiB -- the whole of RAM -- and `min_free_mb` is unset by the argument
// in limits.hpp, which is a good argument. So the only thing standing between a
// print loop and the machine's memory was a ceiling the machine cannot reach
// before it is already thrashing.
//
// SO THE QUEUE IS BOUNDED, AND THE BOUND IS NOT A LIMIT ON THE LANGUAGE.
// DESIGN §7.5 forbids a constant deciding how big a thing the user may write. A
// program can still print forever and print lines of any size -- kHighWater is
// read only when the queue is NON-EMPTY, so a single line larger than it is
// queued whole and written whole, and nothing a program can say becomes
// unsayable. What the constant decides is how far ahead of the WRITER the
// program may run before it waits for it, which is a fact about a pipe and not
// about satellite. The honest cost is stated plainly: a program printing to a
// slow terminal now runs at the terminal's speed. The alternative it replaces
// is the same program dying, and taking the machine with it.
//
// IT COUNTS BYTES AND NOT LINES, because a line count bounds nothing: this
// program's lines are ~150 bytes and QUAD's are not, and a cap of N lines is a
// cap of N times whatever the widest line turns out to be.
//
// WHAT THE WATCHDOG CANNOT REACH IS STATED HERE RATHER THAN DISCOVERED LATER.
// `machine_limits/watchdog.cpp` flushes stdio before `_exit` and says why in
// capitals -- but `fflush` reaches what the printer has ALREADY WRITTEN, and
// this queue sits ABOVE stdio. Draining there is not the fix: the watchdog is a
// detached thread killing a process for taking too much memory, and waiting on
// the printer is exactly what it must never do, because a stuck printer would
// hang the one thing whose job is to not hang. So the shutdown drains and the
// emergency exit does not, and the cost is that LINES QUEUED BUT NOT YET
// WRITTEN ARE LOST WHEN THE WATCHDOG FIRES. PLAN §8's M22 entry is where that
// is revisited, because M22 is already the emergency path's only registrar.

#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace satellite::console {

// The one console. DESIGN §4 numbers it `satellite.console` `1 5` -- a path and
// not a constructor -- so there is one of these and a program cannot ask for a
// second.
//
// A FUNCTION-LOCAL STATIC AND NOT A NAMESPACE-SCOPE ONE, which is the shape
// `evaluator/dispatch.hpp`'s table already has and is here for its reason:
// PLAN §4.3's startup floor is measured every milestone and a global
// constructor is exactly the thing that moves it. `satl --version` must not pay
// for a console it never prints through, and it does not -- the object is built
// the first time somebody asks for it and the THREAD is started the first time
// something is queued.
class Console {
public:
    static Console &the();

    // START THE PRINTER. Idempotent, and it is the same call `display` makes
    // for itself -- one mechanism with two callers rather than a lazy path and
    // an eager one that can disagree. `satl`'s run arm calls it so that the
    // cost is paid where it can be attributed, which is PLAN §9's rule.
    void start();

    // `satellite.console.display` `1 5 1` -- a whole line. The newline is
    // appended here rather than by the caller, because DESIGN §10.1's atomicity
    // is a property of THE UNIT QUEUED: a line and its terminator queued
    // separately are two units and another thread's line can land between them.
    void display(std::string line);

    // `display`'s UN-NEWLINED FORM, which PLAN §8's M10 entry calls this
    // milestone's and says "lives under `1 5 1`".
    //
    // NO PROGRAM CAN REACH IT YET, AND THAT IS THE GRAMMAR RATHER THAN AN
    // OMISSION. v1 spelled it `display(text, end="")` -- PLAN §8's M14 entry
    // cites `named_arg_misuse()` and `display_with_end()` by name -- and
    // DESIGN §6's grammar has no named arguments at all: `args := expression {
    // "," expression }`. So the capability is built here, where the queue is,
    // and the SPELLING is left to the milestone that gets the first caller.
    // M14's `satellite.console.input(prompt)` `1 5 3` is that caller: DESIGN
    // §10.1 justifies the flush by "a prompt written with no trailing newline",
    // and a prompt is exactly a write with no terminator followed by a drain.
    void write(std::string text);

    // WHETHER THE LAST THING PRINTED LEFT THE CURSOR MID-LINE, and forget it --
    // M30. `display("a", end="")` at the prompt printed `a` and the prompt's
    // redraw, which starts with `\r ESC[K`, erased it. The prompt asks this
    // after drain() and starts a fresh row when the answer is yes.
    bool take_mid_line();

    // STEP 1 OF THE SHUTDOWN, ON ITS OWN. Returns when everything queued before
    // the call has been written AND flushed -- see the note in console.cpp on
    // why the flush happens under the lock, which is what makes that promise
    // true rather than nearly true.
    void drain();

    // ALL FOUR STEPS -- AND A FIFTH SINCE M14: the reader is woken through
    // its pipe and joined, so a thread parked on stdin never outlives the
    // run. Safe to call twice, because an error path and a success path both
    // take it and neither should have to know which ran first.
    void shutdown();

    // THE TERMINAL'S FACTS -- `satellite.console.width` `1 5 6` and `.height`
    // `1 5 7`, and the 14 lines of v1's 1,449 that come across (M14):
    // `terminal_columns()`'s ioctl with its 80-column fallback, plus the
    // height v1 never had (`ws_row` appears zero times in it), falling back
    // to 24. ASKED FRESH EVERY TIME, never sampled -- a terminal resizes
    // during a run, which is what makes these facts and not configuration
    // (PLAN M14's argument against `arguments.machine.*`), and M14's
    // done-when clause 7 resizes a pty between two asks to prove it. M22's
    // prompt consumes these from here rather than reimplementing them.
    int width() const;
    int height() const;

    // What is waiting, for a test to assert on. Not for the language.
    size_t waiting() const;
    bool printing() const;

private:
    Console() = default;
    ~Console();

    Console(const Console &) = delete;
    Console &operator=(const Console &) = delete;

    // start(), with the lock already held. The one place the thread is made.
    void start_held();

    void push(std::string text);
    void printer();

    mutable std::mutex mutex_;

    // TWO CONDITION VARIABLES AND NOT ONE, which is the hazard v1's
    // `console_output/console.cpp` documents and PLAN §8's M14 entry says to
    // read before writing the reader: a waiting drain and a sleeping printer
    // must never be woken for each other's reason. `arrived_` says there is
    // work; `emptied_` says there is none left.
    std::condition_variable arrived_;
    std::condition_variable emptied_;

    // A THIRD, AND THE HEADER'S OWN RULE IS WHY IT IS NOT A REUSE OF `emptied_`:
    // a waiting drain and a producer waiting for room are woken for different
    // reasons and hold different predicates. `emptied_` says there is no work
    // left anywhere, which is what `drain()` needs; `roomed_` says the printer
    // has TAKEN a batch, which is what a blocked producer needs and which is
    // true the instant of the swap, long before the writing is done.
    std::condition_variable roomed_;

    // SWAPPED OUT WHOLE RATHER THAN POPPED FROM THE FRONT -- v1's note again.
    // The printer takes the vector, releases the lock and writes; a producer
    // that arrives meanwhile fills a fresh one and never waits on a syscall.
    std::vector<std::string> queue_;

    // WHAT `queue_` IS HOLDING, IN BYTES. Kept alongside rather than summed on
    // demand, because push() consults it on every call and a fold over the
    // vector would put an O(n) walk in the most-called path in the language --
    // which is the objection the paragraph at the top of this file makes to
    // hidden costs, applied to the cure rather than the disease.
    size_t queued_bytes_ = 0;

    // HOW FAR AHEAD OF THE WRITER A PROGRAM MAY RUN. Not a limit on what can be
    // printed -- see the header -- and deliberately generous: a file sink
    // drains far faster than this fills, so backpressure never engages there
    // and the measured 4.4 MB stays 4.4 MB. Peak queue memory is at most twice
    // this: one batch in flight while the next fills.
    static constexpr size_t kHighWater = 4u * 1024u * 1024u;

    std::thread printer_;
    bool started_ = false;
    bool closed_ = false;

    // A BATCH IS OUT OF THE QUEUE AND NOT YET WRITTEN, which is the state an
    // empty queue cannot express and which a drain that ignored it would return
    // in the middle of.
    bool writing_ = false;
    bool mid_line_ = false;  // take_mid_line(); guarded by mutex_
};

} // namespace satellite::console
