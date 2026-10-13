// satellite/satellite_object/object_lock.cpp -- satellite's own lock, and the check that turns
// a wait that would never end into S728 (object_lock.hpp says why it is satellite's own).

#include "object_lock.hpp"

#include "../machine/console_lock.hpp"
#include "../machine/machine_codes.hpp"
#include "../satellite_variable_thread/satellite_thread.hpp"

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>
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
        if (waits_for(lock->holder.load(std::memory_order_relaxed), me, seen))
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
    const WaitNode *own = lock.holder.load(std::memory_order_relaxed);
    if (own != nullptr && own != me && waits_for(own, me, seen))
        return true;
    for (const WaitNode *reader : lock.readers)
        if (reader != me && waits_for(reader, me, seen))
            return true;
    return false;
}

thread_local std::vector<std::pair<ObjectLock *, LockUse>> held_here;

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
    // A WRITER COUNTS ITSELF WAITING FROM THE START, so readers arriving after it wait behind it
    // -- taken back on every way out, even an allocation that fails in the walk below.
    struct Counted {
        ObjectLock &lock;
        bool on;
        ~Counted() { if (on) --lock.writers_waiting; }
    } counted{lock, writes};
    if (writes)
        ++lock.writers_waiting;
    for (;;) {
        // A READER GOES AHEAD OF A QUEUED WRITER WHEN WAITING WOULD CLOSE A CIRCLE and no
        // writer HOLDS it -- safe, because readers exclude nobody but a writer. Without it, a
        // line reading while it joins a thread that also reads was S728 whenever a third
        // thread queued a write between them (the third review, 2026-09-24).
        const bool free = writes ? lock.writer == nullptr && lock.readers.empty()
                                 : lock.writer == nullptr &&
                                       (lock.writers_waiting == 0 || a_holder_waits_for(lock, &me));
        if (free)
            break;
        // THE COUNT IS TAKEN BACK (Counted, above) WHILE THE MUTEX IS STILL HELD: it is
        // destroyed before `hold`, being declared after it.
        if (a_holder_waits_for(lock, &me)) {
            released().notify_all();
            return wait_never_ends;
        }
        me.waiting_lock = &lock;
        released().wait(hold);
        me.waiting_lock = nullptr;
    }
    if (writes) {
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

// SATL'S OWN HOLD (object_lock.hpp): THE FLIP when nobody holds it -- one compare-and-swap of `holder` -- and the
// mutex, the circle check and a sleep only when somebody does. Every flip is sequentially consistent, as is `asleep`:
// a thread letting go either sees one asleep and wakes it, or the sleeper's next flip sees the lock free.
namespace {

signed long long int take_own_hold(ObjectLock &lock)
{
    WaitNode &me = this_threads_node();
    const WaitNode *nobody = nullptr;
    if (lock.holder.compare_exchange_strong(nobody, &me, std::memory_order_seq_cst))
        return success;
    std::unique_lock<std::mutex> hold(graph());
    for (;;) {
        lock.asleep.fetch_add(1, std::memory_order_seq_cst);
        nobody = nullptr;
        if (lock.holder.compare_exchange_strong(nobody, &me, std::memory_order_seq_cst)) {
            lock.asleep.fetch_sub(1, std::memory_order_seq_cst);
            return success;
        }
        if (a_holder_waits_for(lock, &me)) {
            lock.asleep.fetch_sub(1, std::memory_order_seq_cst);
            released().notify_all();
            return wait_never_ends;
        }
        me.waiting_lock = &lock;
        released().wait(hold);
        me.waiting_lock = nullptr;
        lock.asleep.fetch_sub(1, std::memory_order_seq_cst);
    }
}

void give_back_own_hold(ObjectLock &lock)
{
    // ONLY THIS THREAD'S: a hold let go of for a join that then ended in S728 is given back again by its
    // ObjectHold on the way out, when another thread may hold it.
    const WaitNode *me = &this_threads_node();
    if (!lock.holder.compare_exchange_strong(me, nullptr, std::memory_order_seq_cst))
        return;
    if (lock.asleep.load(std::memory_order_seq_cst) != 0) {
        const std::lock_guard<std::mutex> hold(graph());
        released().notify_all();
    }
}

signed long long int take_either(ObjectLock &lock, LockUse use)
{
    return use == LockUse::own ? take_own_hold(lock) : take_object_lock(lock, use);
}

void give_back_either(ObjectLock &lock, LockUse use)
{
    if (use == LockUse::own)
        give_back_own_hold(lock);
    else
        give_back_object_lock(lock, use);
}

// EVERY HOLD THIS THREAD'S LINE HAD, TAKEN BACK after a wait it let them go for (a join, a sleep, a circle broken).
// Should taking one back close a circle -- another thread holds it and waits for one taken back already -- what was
// taken is given back, that one is waited for with nothing held (which ends: no circle passes through a thread
// holding nothing), and every one is taken again in its order. The other thread goes on meanwhile.
void take_back_recovering()
{
    for (;;) {
        std::size_t taken = 0;
        signed long long int code = success;
        for (; taken < held_here.size(); ++taken) {
            code = take_either(*held_here[taken].first, held_here[taken].second);
            if (code != success)
                break;
        }
        if (code == success)
            return;
        for (std::size_t k = taken; k-- > 0;)
            give_back_either(*held_here[k].first, held_here[k].second);
        if (take_either(*held_here[taken].first, held_here[taken].second) == success)
            give_back_either(*held_here[taken].first, held_here[taken].second);
        else
            std::this_thread::yield();
    }
}

} // namespace

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

void let_go_while_waiting()
{
    for (auto each = held_here.rbegin(); each != held_here.rend(); ++each)
        give_back_either(*each->first, each->second);
}

signed long long int take_back_after_waiting()
{
    take_back_recovering();     // a circle met taking them back is broken there, never refused (object_lock.hpp)
    return success;
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
    if (lock == nullptr || use == LockUse::none)
        return;
    // THE AUTHOR'S LOCK WHEN IT IS ON; SATL'S OWN HOLD WHEN IT IS OFF AND A THREAD HAS STARTED (object_lock.hpp);
    // nothing before the first start(), where no other thread can be touching it.
    if (!lock->on.load(std::memory_order_acquire)) {
        if (!a_thread_was_started().load(std::memory_order_acquire))
            return;
        use = LockUse::own;
    }
    for (const auto &[each, how] : held_here)
        if (each == lock)
            return;
    // NOTED BEFORE IT IS TAKEN, so an allocation failing here cannot leave a lock taken and
    // never given back (the third review).
    held_here.emplace_back(lock, use);
    code_ = take_either(*lock, use);
    // A CIRCLE IS BROKEN HERE, NOT REFUSED (object_lock.hpp): this line's earlier holds are given back, this one is
    // waited for with nothing held, and they are taken back with this one's giving back, below.
    if (code_ == wait_never_ends && held_here.size() > 1) {
        held_here.pop_back();
        let_go_while_waiting();
        held_here.emplace_back(lock, use);
        code_ = take_either(*lock, use);
        if (code_ != success) {
            held_here.pop_back();
            take_back_recovering();
            return;
        }
        let_go_ = true;
    }
    if (code_ != success) {
        held_here.pop_back();
        return;
    }
    lock_ = lock;
    use_ = use;
}

ObjectHold::~ObjectHold()
{
    if (lock_ == nullptr)
        return;
    held_here.pop_back();
    give_back_either(*lock_, use_);
    if (let_go_)
        take_back_recovering();
}

} // namespace satellite004
