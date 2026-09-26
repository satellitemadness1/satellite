#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// The console as a queue with its own printer thread, so that writing output
// and pushing it through stdout are two different jobs on two different
// threads.
//
// Before this, `satellite.console.display` did `output_ += text` into a plain
// std::string member of the Evaluator, and interp.cpp copied the whole thing
// out and printed it once, after the program had already finished. Two
// properties of that are worth naming, because both are why this file exists:
//
//   1. Nothing appeared until the program exited. **Verified**: a program that
//      prints a line, spins for several seconds, then prints another line
//      emitted both lines 2 ms apart, at the end. A long-running program was
//      indistinguishable from a hung one.
//   2. Every byte the program ever displayed was held in memory at once, and
//      then copied.
//
// Both fall out of the same fact — the buffer had no consumer. This gives it
// one. A producer takes the lock only long enough to move a string into a
// vector, and never waits on stdout; the printer thread does the writing and
// frees each string the moment it has been written.
//
// The producer side is deliberately allowed to outrun the consumer. That is
// the point rather than a tolerated side effect: the tree walk should be
// limited by how fast it can walk, not by how fast a terminal can scroll.

namespace satellite {

class Console {
public:
    // Starts the printer thread. There is nothing to configure, because the
    // only decision this class makes — when to flush — is answered by the
    // queue being empty rather than by a policy.
    Console();

    // Prints everything still queued, then joins. A Console that is destroyed
    // while a program is still writing to it is a bug in the caller, not a
    // case this handles: the writer must be finished first.
    ~Console();

    Console(const Console &) = delete;
    Console &operator=(const Console &) = delete;

    // Queues one piece of output. Safe from any thread.
    //
    // The unit is whatever the caller passes, and `display` passes one whole
    // line including its newline. That is what keeps a line ATOMIC once
    // threads exist: with a shared std::string and `+=`, two threads appending
    // at once interleave mid-line and shred both. One string per element
    // cannot, whatever order the elements arrive in.
    //
    // Takes by value and moves: a caller that built the string only to display
    // it hands over the allocation instead of paying for a copy of it.
    void write(std::string text);

    // Keep at least this long between two consecutive pieces of output. Zero,
    // the default, is full speed.
    //
    // A MINIMUM GAP and not a fixed delay: a line that is already later than
    // the pace waits for nothing. Inside a program the two are the same thing,
    // because the walk queues lines far faster than any pace lets them out —
    // at the prompt they are not, and the fixed reading made a line typed
    // after five idle seconds wait another 1.5 s on a 1500 ms pace, which is
    // lag rather than spacing.
    //
    // This is what satellite.console.display(100ms) sets, and it is set on the
    // CONSOLE rather than on the evaluator because of where the waiting has to
    // happen. A pause taken by the code that called display() would stop the
    // program: the loop computing the next line would sit in a sleep, and the
    // whole point of the request is that the program keeps running while its
    // output is metered out. The printer thread is already the one thread
    // whose job is the terminal's pace, so the sleep goes there, and the
    // producer never learns that the output is being paced at all.
    //
    // Safe from any thread, and takes effect on the next line DISPLAYED rather
    // than on the next line printed. The difference is not subtle, and the
    // first version of this got it wrong: a program that paces five lines and
    // then sets the pace back to zero queues all six calls in microseconds, so
    // a printer that read the current pace as it went would find the zero
    // already in place and print the five it was supposed to space out in one
    // burst. **Verified** — that program ran in 2 ms. The pace a line is
    // written with therefore travels WITH the line (see Piece), and the
    // printer obeys the pace the program had set when it said display, not the
    // one that happens to be current when the terminal gets there.
    //
    // The cost, stated rather than discovered: the queue is the buffer the
    // pause is drawn against. A program that displays faster than the pace
    // lets lines leave holds the difference in memory, and a hot loop at 100 ms
    // a line holds all of it. That is the trade a pace asks for -- the
    // alternative is blocking the producer, which is the thing this class was
    // built not to do.
    void pace(long long nanoseconds);

    // What pace() was last given. Chiefly for tests: a printer that is asleep
    // has no other observable state.
    long long pace() const;

    // Stop obeying the pace stamped on lines that are ALREADY QUEUED, and wake
    // a printer that is mid-pause so it notices immediately.
    //
    // This exists for exactly one caller: a run that was interrupted. A pace
    // is a presentation choice made by a program that is still running, and an
    // interrupted one is not -- so a Ctrl-C on a program that queued ten
    // thousand lines at 100 ms a line has to be able to stop the seventeen
    // minutes of drain it would otherwise owe. pace(0) cannot do it, because
    // the pace travels WITH each queued line (see pace() above) and setting a
    // new one changes nothing about the lines already stamped.
    //
    // The output itself is NOT discarded. It was produced, so it is owed to
    // the reader; only the spacing is dropped.
    void abandon_pace();

    // Obey the stamped paces again. The prompt's Console outlives every
    // program it runs, so an interrupted run must not leave the next one
    // unable to pace itself.
    void resume_pace();

    // Blocks until everything queued so far has been written AND flushed.
    //
    // The run has to be able to say "I am done" without destroying the
    // Console, because interp.cpp still has an error report to print after the
    // program's own output and the two must not interleave.
    //
    // A pace is honoured here and in the destructor rather than dropped at the
    // end: a program that displays five lines and exits would otherwise show
    // all five at once, which is the exact case the pace was set for. So the
    // run outlives the walk by whatever the queue still owes.
    void drain();

private:
    void run();

    // Whether the printer should stop obeying a pace: abandon_pace() was
    // called, or a Ctrl-C has arrived. See the note at the wait in run().
    bool pace_abandoned() const;

    // One queued write: the text, and the pace that was in effect when the
    // producer handed it over.
    //
    // Eight bytes per queued line more than a bare std::string, and that is the
    // price of the paragraph on pace() above: the pace has to be sampled where
    // the ordering is defined, which is the producer's side, and then carried.
    // The strings themselves are unchanged and are still freed one at a time as
    // they are written.
    struct Piece {
        std::string text;
        long long pace_ns = 0;
    };

    // A std::vector<Piece>, one element per write, exactly as asked.
    //
    // The printer does NOT erase the front element per line. `erase(begin())`
    // shifts every remaining element on every line, which makes printing a
    // program's whole output quadratic in the number of lines — 2,000 lines of
    // forge narration is two million moves for nothing. Instead the printer
    // SWAPS the whole vector out under the lock and empties its private copy,
    // freeing each string the instant it has been written. Same data
    // structure, same "gone from memory once printed" guarantee, and the
    // producers get the lock back immediately instead of queueing behind a
    // write to a terminal.
    std::vector<Piece> pending_;

    // The pace new writes are stamped with. Atomic and NOT guarded by mutex_:
    // pace() is called from the walking thread and write() reads it there too,
    // and neither should have to queue behind a printer that is mid-terminal.
    std::atomic<long long> pace_ns_{0};

    std::mutex mutex_;

    // The pace sleep, and the two things that end it early. A mutex of its own
    // rather than mutex_, so that a printer waiting out a 1500 ms pause is
    // never holding the queue's lock while it does -- which is the property
    // this whole class is built around: a producer must never wait on a
    // terminal.
    std::mutex pace_mutex_;
    std::condition_variable pace_woken_;
    std::atomic<bool> unpaced_{false};

    // Wakes the printer when work arrives, or when it is time to stop. An
    // empty queue WAITS here rather than spinning: a program can go a long
    // time between two displays, and a poll loop would burn a core doing
    // nothing for all of it.
    std::condition_variable arrived_;

    // Wakes drain() when the printer has caught up. Separate from arrived_ so
    // that a waiting drain and a sleeping printer are never woken for each
    // other's reason.
    std::condition_variable emptied_;

    // Guarded by mutex_. `printing_` covers the window where the queue is
    // already empty but the batch swapped out of it has not been written yet —
    // without it, drain() would return early and let an error report overtake
    // the output it is supposed to follow.
    bool closing_ = false;
    bool printing_ = false;

    // Last, so every member above it is constructed before the thread that
    // touches them starts.
    std::thread printer_;
};

} // namespace satellite
