// satellite/satellite_object/object_lock.cpp -- satellite's own lock, and the check that turns
// a wait that would never end into S728 (object_lock.hpp says why it is satellite's own).

#include "object_lock.hpp"

#include "../machine/machine_codes.hpp"
#include "../satellite_variable_thread/satellite_thread.hpp"

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <utility>

namespace satellite004 {
namespace {

// ONE MUTEX FOR EVERY LOCKED OBJECT'S BOOKKEEPING, AND ONE SIGNAL. Held for a few
// instructions at a time -- to see whether a lock is free and mark it taken -- never for a
// statement. Only a statement touching a locked object, and join(), ever reach it.
std::mutex &graph()
{
    static std::mutex one;
    return one;
}

std::condition_variable &released()
{
    static std::condition_variable one;
    return one;
}

// DOES `from` WAIT FOR `me` -- through the lock it waits for (whoever holds that), or the
// thread it joins (whatever that one waits for)? `seen` keeps a circle that does not pass
// through `me` from being walked forever. Under graph().
bool waits_for(const WaitNode *from, const WaitNode *me, std::vector<const WaitNode *> &seen)
{
    if (from == nullptr)
        return false;
    if (from == me)
        return true;
    if (std::find(seen.begin(), seen.end(), from) != seen.end())
        return false;
    seen.push_back(from);
    if (const ObjectLock *lock = from->waiting_lock) {
        if (waits_for(lock->writer, me, seen))
            return true;
        for (const WaitNode *reader : lock->readers)
            if (waits_for(reader, me, seen))
                return true;
    }
    if (const satellite_thread *thread = from->waiting_thread)
        if (waits_for(thread->node, me, seen))
            return true;
    return false;
}

// WOULD WAITING FOR `lock` NEVER END: someone holding it is, through locks and joins,
// waiting for this thread.
bool a_holder_waits_for(const ObjectLock &lock, const WaitNode *me)
{
    std::vector<const WaitNode *> seen;
    if (lock.writer != nullptr && lock.writer != me && waits_for(lock.writer, me, seen))
        return true;
    for (const WaitNode *reader : lock.readers)
        if (reader != me && waits_for(reader, me, seen))
            return true;
    return false;
}

thread_local std::vector<std::pair<const ObjectLock *, LockUse>> held_here;

} // namespace

WaitNode &this_threads_node()
{
    thread_local WaitNode mine;
    return mine;
}

signed long long int take_object_lock(ObjectLock &lock, LockUse use)
{
    WaitNode &me = this_threads_node();
    const bool writes = use == LockUse::writing;
    std::unique_lock<std::mutex> hold(graph());
    // A WRITER COUNTS ITSELF WAITING FROM THE START, so readers arriving after it wait behind it.
    if (writes)
        ++lock.writers_waiting;
    for (;;) {
        const bool free = writes ? lock.writer == nullptr && lock.readers.empty()
                                 : lock.writer == nullptr && lock.writers_waiting == 0;
        if (free)
            break;
        if (a_holder_waits_for(lock, &me)) {
            if (writes)
                --lock.writers_waiting;
            hold.unlock();
            released().notify_all();
            return wait_never_ends;
        }
        me.waiting_lock = &lock;
        released().wait(hold);
        me.waiting_lock = nullptr;
    }
    if (writes) {
        --lock.writers_waiting;
        lock.writer = &me;
    } else {
        lock.readers.push_back(&me);
    }
    return success;
}

void give_back_object_lock(ObjectLock &lock, LockUse use)
{
    const WaitNode *me = &this_threads_node();
    {
        const std::lock_guard<std::mutex> hold(graph());
        if (use == LockUse::writing) {
            if (lock.writer == me)
                lock.writer = nullptr;
        } else {
            const auto found = std::find(lock.readers.begin(), lock.readers.end(), me);
            if (found != lock.readers.end())
                lock.readers.erase(found);
        }
    }
    released().notify_all();
}

signed long long int start_waiting_for(const satellite_thread &thread)
{
    WaitNode &me = this_threads_node();
    const std::lock_guard<std::mutex> hold(graph());
    std::vector<const WaitNode *> seen;
    if (thread.node == &me || waits_for(thread.node, &me, seen))
        return wait_never_ends;
    me.waiting_thread = &thread;
    return success;
}

void done_waiting_for_a_thread()
{
    const std::lock_guard<std::mutex> hold(graph());
    this_threads_node().waiting_thread = nullptr;
}

void this_thread_runs(satellite_thread &thread)
{
    const std::lock_guard<std::mutex> hold(graph());
    thread.node = &this_threads_node();
}

void this_thread_ended(satellite_thread &thread)
{
    const std::lock_guard<std::mutex> hold(graph());
    thread.node = nullptr;
}

ObjectHold::ObjectHold(ObjectLock *lock, LockUse use)
{
    if (lock == nullptr || use == LockUse::none || !lock->on.load(std::memory_order_acquire))
        return;
    for (const auto &[each, how] : held_here)
        if (each == lock)
            return;
    code_ = take_object_lock(*lock, use);
    if (code_ != success)
        return;
    lock_ = lock;
    use_ = use;
    held_here.emplace_back(lock, use);
}

ObjectHold::~ObjectHold()
{
    if (lock_ == nullptr)
        return;
    held_here.pop_back();
    give_back_object_lock(*lock_, use_);
}

} // namespace satellite004
