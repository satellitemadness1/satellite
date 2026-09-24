#pragma once
// satellite/satellite_object/object_lock.hpp -- THE AUTHOR'S LOCK ON ONE OBJECT (THREADS.md
// T2, 2026-09-23), and satellite's own lock underneath it (2026-09-24).
//
// The author, 2026-09-23: *"we were locking objects, at the users discretion, so if the user
// decides to put a lock on something, that doesn't necessarily turn the lock on, it only
// turns the lock on when something goes to write to that object"*, and *"we were leaving
// the locks off, entirely, unless the user turns the lock that's on, on that object, then
// it only locks if that object is being written to"*.
//
// SO EVERY OBJECT CARRIES ONE, OFF:
//
//     my_object.lock()      turns it ON -- and locks nothing yet
//     my_object.unlock()    turns it OFF again
//
// While it is on, a statement that WRITES the object holds it for that statement, so
// `count = count + 1` from ten threads loses nothing (the plain-bool race lost about half
// its writes, time_test/locks). A statement that only READS it waits only while a write is
// happening or waiting, and never for another read. While it is off -- always, for an
// object nobody locks -- nothing is taken, and the cost is one relaxed load per statement
// of a capsule running on an object.
//
// SATELLITE'S OWN LOCK, NOT std::shared_mutex (the author, 2026-09-24: "do we have to build
// our own lock or something ... How do you build a mutex anyways?"). A lock that makes
// `count = count + 1` exact must make a second writer wait -- and a statement holding it
// that WAITS, for a thread that needs the same lock (`total = w.join()`), waits forever.
// No lock can make that pattern impossible; this one CATCHES it. It knows which threads
// hold it, every thread says what it is waiting for -- a lock, or another thread's join --
// and before any thread sleeps it follows that chain: when the chain comes back to the
// thread about to sleep, the statement stops with S728 WAIT_NEVER_ENDS and says which line,
// rather than the program freezing. 003 caught the same circles as S1407 and S1408.
//
// ALL OF IT UNDER ONE SMALL MUTEX (object_lock.cpp), taken only by statements that touch a
// locked object and by join(): a lock nobody turned on never reaches it.
//
// A WAITING WRITER GOES FIRST: new readers wait behind it, so a steady stream of reading
// statements cannot starve a write (the second review's worry about glibc's rwlock).
//
// ONE THREAD NEVER WAITS FOR ITSELF: a statement already holding the lock that calls a
// capsule of the same object runs that capsule's statements without taking it again.

#include <atomic>
#include <vector>

namespace satellite004 {

class satellite_thread;
struct WaitNode;

struct ObjectLock {
    std::atomic<bool> on{false};
    // Held under object_lock.cpp's one mutex, never read or written elsewhere.
    const WaitNode *writer = nullptr;
    std::vector<const WaitNode *> readers;
    unsigned int writers_waiting = 0;
};

// WHAT ONE OS THREAD IS WAITING FOR, read by every other thread's check: a lock, or a
// program thread it join()s. Under object_lock.cpp's mutex.
struct WaitNode {
    const ObjectLock *waiting_lock = nullptr;
    const satellite_thread *waiting_thread = nullptr;
};

enum class LockUse { none, reading, writing };

// This OS thread's node.
WaitNode &this_threads_node();

// TAKE IT, or answer wait_never_ends when waiting would close a circle -- and then take
// nothing. success when taken.
signed long long int take_object_lock(ObjectLock &lock, LockUse use);
void give_back_object_lock(ObjectLock &lock, LockUse use);

// BEFORE join() WAITS for `thread`: wait_never_ends when that thread is, through locks and
// joins, waiting for this one; otherwise success, and this thread is marked as waiting for
// it until done_waiting_for_a_thread().
signed long long int start_waiting_for(const satellite_thread &thread);
void done_waiting_for_a_thread();

// A PROGRAM THREAD'S OWN NODE, set by the thread when its body starts and cleared when it
// ends, so a join() on it can see what it waits for.
void this_thread_runs(satellite_thread &thread);
void this_thread_ended(satellite_thread &thread);

// HELD FOR ONE STATEMENT (or one condition, one part of a for, one method on a file).
// Takes nothing when there is no lock, when it is off, when nothing is written or read, or
// when this thread already holds it. code() is wait_never_ends when taking it would have
// waited forever -- and then nothing is held.
class ObjectHold {
public:
    ObjectHold(ObjectLock *lock, LockUse use);
    ~ObjectHold();
    ObjectHold(const ObjectHold &) = delete;
    ObjectHold &operator=(const ObjectHold &) = delete;
    signed long long int code() const { return code_; }

private:
    ObjectLock *lock_ = nullptr;
    LockUse use_ = LockUse::none;
    signed long long int code_ = 0;
};

} // namespace satellite004
