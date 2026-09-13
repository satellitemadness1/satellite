#pragma once

// THE ACCESS LIST -- THREAD.md T2, the author's answer to Q1 (2026-09-13).
//
// In the author's words: every thread keeps "an access list", and "when
// something attempts to access a variable, more than one thread, it waits while
// it is accessed once at a time, like a lock, but it's an access list kept per
// thread". What goes on a list is a shared thing -- a spacesuit object, or
// `satellite.library` -- and it stays there for ONE METHOD CALL on the object,
// or ONE STATEMENT for the globals. So `received = received + 1` then
// `pieces.append(e)` inside a method are one step to every other thread
// (D11, D12), and `satellite.library.n = satellite.library.n + 1` is one step
// too (D10).
//
// THREE PIECES:
//
//   Access      one per shared thing. `owner` is the thread record holding it,
//               or null; `depth` counts re-entry by that owner (a method
//               calling another method of the same object).
//   ThreadWait  one per thread. What it is waiting for right now -- an Access,
//               or the thread it is joining -- which is the edge the deadlock
//               check walks.
//   the graph   one process-wide mutex, taken only when a thread has to WAIT
//               and when an owner lets go of something another thread waits
//               for. Taking a free Access is one compare-and-swap and no lock;
//               a single-threaded program never gets here at all.
//
// A WAIT THAT WOULD NEVER END IS REFUSED, NEVER SLEPT ON. Before a thread waits
// it follows the edges from the holder: holder -> what the holder waits for ->
// its holder -> ... If that leads back to the thread asking, every thread on the
// path is waiting for the next and none can move, so the ask is refused (S1408,
// or S1407 when the last edge is a join). THREAD.md §1: nothing hangs.
//
// WHY THE WALK IS EXACT AND NOT A GUESS. Every edge it reads is written under
// the graph mutex: `waiting_for` and `joining` are, and an Access somebody is
// waiting for has `waiters > 0`, and its owner lets go of it only under the
// mutex (release() below). The walk holds the mutex, so no edge it follows can
// change under it -- a cycle it finds exists, and one it does not find does not.

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace satellite::thread {

struct ThreadWait;

struct Access {
    std::atomic<ThreadWait *> owner{nullptr};
    // WRITTEN ONLY BY THE OWNER, and atomic anyway: the next owner writes it
    // after its compare-and-swap on `owner`, which orders the two writes, but
    // ThreadSanitizer does not treat an atomic as ordering OTHER memory, and a
    // T2 that is not clean under it is not done (THREAD.md §1).
    std::atomic<uint32_t> depth{0};
    std::atomic<uint32_t> waiters{0}; // threads registered to wait, under graph
    std::condition_variable changed;  // waited on with the graph mutex
};

struct ThreadWait {
    Access *waiting_for = nullptr;       // under graph
    ThreadWait *joining = nullptr;       // under graph
};

inline std::mutex &graph()
{
    static std::mutex the;
    return the;
}

// WHETHER `me` WAITING ON SOMETHING `from` HOLDS OR IS would never end. Call
// with the graph mutex held.
inline bool leads_back(const ThreadWait *from, const ThreadWait *me)
{
    for (const ThreadWait *at = from; at != nullptr;) {
        if (at == me)
            return true;
        if (at->waiting_for != nullptr)
            at = at->waiting_for->owner.load(std::memory_order_acquire);
        else
            at = at->joining;
    }
    return false;
}

// TAKE `thing` FOR `me`. False means the wait would never end and nothing was
// taken; the caller refuses.
inline bool acquire(Access &thing, ThreadWait *me)
{
    if (thing.owner.load(std::memory_order_acquire) == me) {
        thing.depth.fetch_add(1, std::memory_order_relaxed);
        return true;
    }
    ThreadWait *free = nullptr;
    if (thing.owner.compare_exchange_strong(free, me, std::memory_order_acquire)) {
        thing.depth.store(1, std::memory_order_relaxed);
        return true;
    }

    std::unique_lock<std::mutex> held(graph());
    thing.waiters.fetch_add(1, std::memory_order_acquire);
    me->waiting_for = &thing;
    for (;;) {
        free = nullptr;
        if (thing.owner.compare_exchange_strong(free, me,
                                                std::memory_order_acquire))
            break;
        if (leads_back(thing.owner.load(std::memory_order_acquire), me)) {
            me->waiting_for = nullptr;
            thing.waiters.fetch_sub(1, std::memory_order_acquire);
            return false;
        }
        thing.changed.wait(held);
    }
    me->waiting_for = nullptr;
    thing.waiters.fetch_sub(1, std::memory_order_acquire);
    thing.depth.store(1, std::memory_order_relaxed);
    return true;
}

// LET GO OF ONE LEVEL OF `thing`, which `me` holds.
inline void release(Access &thing)
{
    if (thing.depth.fetch_sub(1, std::memory_order_relaxed) > 1)
        return;
    if (thing.waiters.load(std::memory_order_acquire) == 0) {
        thing.owner.store(nullptr, std::memory_order_acquire);
        // A THREAD MAY HAVE REGISTERED BETWEEN THE LOAD AND THE STORE. It
        // holds the graph mutex from registering until it sleeps, so taking
        // the mutex here waits until it is asleep, and the wake reaches it.
        if (thing.waiters.load(std::memory_order_acquire) == 0)
            return;
        std::lock_guard<std::mutex> held(graph());
        thing.changed.notify_all();
        return;
    }
    std::lock_guard<std::mutex> held(graph());
    thing.owner.store(nullptr, std::memory_order_acquire);
    thing.changed.notify_all();
}

} // namespace satellite::thread
