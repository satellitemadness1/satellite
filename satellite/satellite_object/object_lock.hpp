#pragma once
// satellite/satellite_object/object_lock.hpp -- THE AUTHOR'S LOCK ON ONE OBJECT (THREADS.md
// T2, 2026-09-23).
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
// `count = count + 1` from ten threads loses nothing -- the plain-bool race lost about
// half its writes (time_test/locks). A statement that only READS it waits only while a
// write is happening, and never for another read: without that, a thread reading a list
// while another appends to it can crash satl, not just see an old value. While it is off
// -- which is always, for an object nobody locks -- nothing is taken at all, and the cost
// is one relaxed load per statement of a capsule running on an object.
//
// ONE THREAD NEVER WAITS FOR ITSELF: a statement already holding the lock that calls a
// capsule of the same object runs that capsule's statements without taking it again. The
// thread's holds are kept in `held` below for exactly that.
//
// NOT CAUGHT YET: two threads each holding one object's lock and waiting for the other's
// -- a statement on A that calls into B, while another on B calls into A -- wait forever.
// 003 caught that circle as S1408; THREADS.md lists it as open.

#include <atomic>
#include <shared_mutex>
#include <vector>

namespace satellite004 {

struct ObjectLock {
    std::atomic<bool> on{false};
    std::shared_mutex mutex;
};

enum class LockUse { none, reading, writing };

// HELD FOR ONE STATEMENT (or one condition, or one method call on a file). Takes nothing
// when there is no lock, when it is off, when nothing is written or read, or when this
// thread already holds it.
class ObjectHold {
public:
    ObjectHold(ObjectLock *lock, LockUse use)
    {
        if (lock == nullptr || use == LockUse::none || !lock->on.load(std::memory_order_acquire))
            return;
        for (const ObjectLock *each : held())
            if (each == lock)
                return;
        if (use == LockUse::writing)
            lock->mutex.lock();
        else
            lock->mutex.lock_shared();
        lock_ = lock;
        use_ = use;
        held().push_back(lock);
    }
    ~ObjectHold()
    {
        if (lock_ == nullptr)
            return;
        held().pop_back();
        if (use_ == LockUse::writing)
            lock_->mutex.unlock();
        else
            lock_->mutex.unlock_shared();
    }
    ObjectHold(const ObjectHold &) = delete;
    ObjectHold &operator=(const ObjectHold &) = delete;

private:
    static std::vector<const ObjectLock *> &held()
    {
        thread_local std::vector<const ObjectLock *> mine;
        return mine;
    }
    ObjectLock *lock_ = nullptr;
    LockUse use_ = LockUse::none;
};

} // namespace satellite004
