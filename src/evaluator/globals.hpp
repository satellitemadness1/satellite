#pragma once

// `satellite.library` -- the one thing two walks share, M23.
//
// DESIGN §7.2 IS THE WHOLE ARGUMENT AND IT WAS WRITTEN FOR THIS MILESTONE
// WITHOUT NAMING IT. The section is about frames, and the reason frames exist
// is that the first satellite kept every local in a global registry and "eight
// threads running a capsule with NO RECURSION AND NO SHARED STATE produced 1585
// wrong results out of 1600". The fix is in two halves and only one of them was
// buildable before there were threads:
//
//     "The global registry needs ZERO LOGIC CHANGES. It is not demoted; it is
//      pointed at the data it was designed for. Permanent identity, CROSS-
//      THREAD SHARING, lock-free reads and atomic read-modify-write are exactly
//      what globals need and exactly the opposite of what locals need."
//
// So a local is a frame slot, reachable from one thread by construction -- the
// `std::vector<Value> slots` in §7.2's own code block says "no mutex, no
// atomic: reachable from one thread" -- and a global is THIS, reachable from
// all of them. Until M23 there was no second thread and Machine held the
// globals in a plain vector of its own; a thread's Machine would then have got
// its own empty copy, and `satellite.library.counter` would have been two
// counters that never met. That is not a race, it is worse: a program that
// silently does nothing, which is the shape DESIGN §1.1 refuses hardest.
//
// LOCK-FREE READS ARE WHAT §7.2 ASKS FOR AND ARE NOT WHAT THIS DOES, AND THE
// REASON IS satellite_value/value.hpp's FIRST LINE. A `Value` is a 40-byte
// variant whose widest arm is a 32-byte `Number` and whose handle arms carry a
// refcount; there is no machine this tree targets on which reading or writing
// one is a single atomic operation. §7.2's sentence was written about v1's
// registry of `SatValue`s and is a description of what globals WANT rather than
// a measurement of what they cost. So the mutex is here, and what is lock-free
// is deciding whether to take it.
//
// THE SINGLE-THREADED PROGRAM PAYS ONE RELAXED LOAD AND THAT IS THE WHOLE COST.
// `shared_` is false until a second walk exists, which happens exactly once per
// run and only in a program that says `start()`. Every program written before
// M23 -- every example in this tree, every fixture, every line typed at M22's
// prompt -- reads a global through one relaxed compare and a copy it was making
// anyway. MILESTONES/M23.md §5 has the measurement.
//
// AND THE FLIP IS SAFE BECAUSE OF WHEN IT HAPPENS, which is worth stating
// rather than trusting. `share()` runs on the parent thread inside
// `satellite.variable.thread.start()`, BEFORE std::thread's constructor
// returns -- so the store is sequenced before anything the new thread does, and
// std::thread's constructor is a synchronisation point that C++ requires to
// publish it. There is no window in which one walk thinks the globals are
// private while another is already reading them, because until that constructor
// returns there is no other walk.
//
// A HANDLE AND NOT A SINGLETON, for dispatch.hpp's opposite reason. The handler
// TABLE is process-wide and says so, because every row it holds is a property
// of the build; these are a property of a RUN, and M22's prompt runs many
// programs in one process. A static here would let one program read the
// globals of the one before it.

#include "satellite_thread/access_list.hpp"
#include "satellite_value/value.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>

namespace satellite::eval {

class Globals {
public:
    explicit Globals(uint32_t count) : slots_(count) {}

    // ONE MORE WALK CAN SEE THESE FROM NOW ON. Called on the parent thread
    // before the child exists -- see the note above about why that ordering is
    // the whole of the safety argument. It never goes back: a program that has
    // started a thread has had one, and a run is short.
    void share() { shared_.store(true, std::memory_order_relaxed); }

    bool shared() const { return shared_.load(std::memory_order_relaxed); }

    // A COPY AND NOT A REFERENCE, which is the one shape change M23 makes to
    // the machine's accessors. Handing back a `const Value &` would hand back a
    // reference into a vector another thread may be writing, and the lock would
    // protect the read of the pointer and not the use of it -- the classic
    // shape where a mutex is present and buys nothing. Every caller was already
    // copying: `op_global` pushes the value onto the value stack.
    Value read(uint32_t index) const
    {
        if (!shared())
            return slots_[index];
        std::lock_guard<std::recursive_mutex> held(lock_);
        return slots_[index];
    }

    void write(uint32_t index, Value value)
    {
        if (!shared()) {
            slots_[index] = std::move(value);
            return;
        }
        std::lock_guard<std::recursive_mutex> held(lock_);
        slots_[index] = std::move(value);
    }

    uint32_t count() const { return static_cast<uint32_t>(slots_.size()); }

    // ONE HOLD ACROSS SEVERAL READS AND WRITES -- THREAD.md D3. A mutating
    // method on a global takes the value out, runs, and writes it back, and
    // those were three separate grabs of the lock above: another thread read
    // `nothing` in the gap and one of two appends was lost. The returned lock
    // keeps every other thread's read() and write() waiting until it goes out
    // of scope, and read()/write() inside it re-enter, which is why the mutex
    // is recursive. Empty, and free, while the globals are not shared.
    std::unique_lock<std::recursive_mutex> hold() const
    {
        if (!shared())
            return {};
        return std::unique_lock<std::recursive_mutex>(lock_);
    }

    // `satellite.library` ON A THREAD'S ACCESS LIST -- THREAD.md T2. One for
    // all the globals, held for a statement: Machine::touch_globals().
    thread::Access access;

private:
    mutable std::recursive_mutex lock_;
    std::atomic<bool> shared_{false};
    std::vector<Value> slots_;
};

} // namespace satellite::eval
