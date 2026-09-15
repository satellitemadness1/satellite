// The thread pool. See machine_limits/pool.hpp for the measurements and the
// decision they justify.
//
// THE POOL IS ALLOCATED ONCE AND NEVER DESTROYED, ON PURPOSE, and this is the
// one thing in the file that looks like a leak and is not.
//
// A pool whose destructor joined its threads would put that join on the exit
// path of EVERY run -- `satl --version` included -- to tidy up threads the
// kernel is about to reclaim anyway; and a STATIC pool destroyed at exit is the
// classic exit-time crash, because static destructors run while detached
// threads are still awake and one of them touches the object as it is being
// torn down. Leaking it makes both impossible: there is nothing to destroy, so
// there is no window. The threads are detached for the same reason, and it is
// the shape v1's watchdog already had.
//
// AND THE PROCESS MAY EXIT WHILE THE BUILDER IS STILL BUILDING, which is not a
// corner but the ordinary case: satl --version finishes in ~0.6 ms and the pool
// takes ~0.59 ms to finish spawning. exit() from the main thread tears down the
// whole thread group, so a thread inside pthread_create at that instant simply
// stops existing -- nothing here is left half-written, because nothing here is
// read after main returns.

#include "machine_limits/pool.hpp"

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>

namespace satellite::limits::pool {

namespace {

struct Pool {
    std::mutex lock;
    std::condition_variable wake;    // workers wait here for a batch
    std::condition_variable done;    // the caller waits here for the last chunk
    std::mutex one_batch;            // pool.hpp: one batch at a time

    unsigned asked = 0;              // THREAD_COUNT
    unsigned waiting = 0;            // parked right now

    // The batch. `generation` is what a worker compares against its own count
    // to tell a new batch from a spurious wake-up -- a flag would be ambiguous
    // for a worker that slept through a whole batch and woke into the next.
    unsigned long long generation = 0;
    const std::function<void(size_t, size_t)> *body = nullptr;
    size_t units = 0;
    size_t chunk = 0;
    unsigned chunks = 0;
    unsigned taken = 0;
    unsigned finished = 0;

    // How many THREADS took at least one chunk of this batch. Counted at TAKE
    // time and under the lock, not at finish time: a worker that registered
    // itself only after its last chunk could still be unregistered when the
    // caller wakes on `finished == chunks`, and run_over() would then under-
    // report the very thing it returns.
    unsigned runners = 0;
};

Pool *the_pool = nullptr;

// Do chunk `which`, then say so. The lock is HELD on entry and on return, and
// dropped for the body -- which is the whole point of a chunk.
void run_chunk(Pool &pool, std::unique_lock<std::mutex> &held, unsigned which)
{
    const size_t from = static_cast<size_t>(which) * pool.chunk;
    size_t to = from + pool.chunk;
    if (to > pool.units)
        to = pool.units;
    const std::function<void(size_t, size_t)> *body = pool.body;

    held.unlock();
    (*body)(from, to);
    held.lock();

    if (++pool.finished == pool.chunks)
        pool.done.notify_all();
}

void worker()
{
    Pool &pool = *the_pool;
    unsigned long long mine = 0;
    std::unique_lock<std::mutex> held(pool.lock);
    for (;;) {
        pool.waiting++;
        pool.wake.wait(held, [&] { return pool.generation != mine; });
        pool.waiting--;
        mine = pool.generation;

        bool ran = false;
        while (pool.taken < pool.chunks) {
            if (!ran) {
                pool.runners++;
                ran = true;
            }
            run_chunk(pool, held, pool.taken++);
        }
    }
}

// The one thread the main thread spawns. It builds the rest and then becomes
// one of them, so THREAD_COUNT threads exist and not THREAD_COUNT + 1.
//
// A THREAD THAT CANNOT BE CREATED STOPS THE BUILD AND DOES NOT STOP THE
// PROCESS. std::thread's constructor THROWS std::system_error when
// pthread_create fails -- `ulimit -u`, a cgroup pids limit, a machine out of
// memory -- and this runs on a detached thread with no handler above it, so
// letting it escape is std::terminate: satl killed at startup, with no message,
// because it could not make its twentieth thread. A pool that could not be
// fully built is a SMALLER pool, and run_over()'s caller-keeps-taking loop is
// what makes a smaller one correct rather than merely slower.
void build()
{
    try {
        for (unsigned i = 1; i < the_pool->asked; i++)
            std::thread(worker).detach();
    } catch (...) {
    }
    worker();
}

} // namespace

void start(unsigned threads)
{
    if (the_pool)
        return;
    the_pool = new Pool;          // deliberately never deleted; see the header
    the_pool->asked = threads < 1 ? 1 : threads;
    std::thread(build).detach();
}

unsigned wanted()
{
    return the_pool ? the_pool->asked : 0;
}

unsigned parked()
{
    if (!the_pool)
        return 0;
    std::lock_guard<std::mutex> held(the_pool->lock);
    return the_pool->waiting;
}

bool would_thread(size_t units)
{
    return the_pool && the_pool->asked > 1 && units >= kFloor;
}

unsigned run_over(size_t units, const std::function<void(size_t, size_t)> &body)
{
    if (units == 0)
        return 1;

    // REQUIREMENT 2, AND IT IS CHECKED BEFORE THE LOCK IS TAKEN. Below the
    // floor the pool is not merely unused -- it is not touched at all, so a
    // small batch costs one comparison and never contends with a pool that is
    // still being built.
    if (!would_thread(units)) {
        body(0, units);
        return 1;
    }

    Pool &pool = *the_pool;
    std::lock_guard<std::mutex> only_one(pool.one_batch);

    // ONE CHUNK PER THREAD PLUS ONE FOR THE CALLER, and the CHUNK is what is
    // rounded up rather than the count -- so the last chunk is short and no
    // chunk starts past the end. Deriving the count back from the size is what
    // makes that true for every `units`: at 10 units over 25 threads the chunk
    // is 1 and there are 10 chunks, where dividing the other way would have
    // produced 25 chunks of 1 and fifteen that begin past the last unit.
    //
    // Splitting finer would balance an uneven body better and would cost a lock
    // per chunk; every tenant this is being built for walks a flat array, where
    // the pieces are the same size by construction.
    const unsigned share = pool.asked + 1;
    const size_t chunk = (units + share - 1) / share;
    const unsigned chunks = static_cast<unsigned>((units + chunk - 1) / chunk);

    std::unique_lock<std::mutex> held(pool.lock);
    pool.body = &body;
    pool.units = units;
    pool.chunk = chunk;
    pool.chunks = chunks;
    pool.taken = 0;
    pool.finished = 0;
    pool.runners = 0;
    pool.generation++;
    pool.wake.notify_all();

    // The caller keeps taking chunks until there are none left, which is what
    // makes this safe against a pool that is not built yet -- see pool.hpp.
    // With no worker awake it does all of them and waits for nothing.
    bool ran = false;
    while (pool.taken < pool.chunks) {
        if (!ran) {
            pool.runners++;
            ran = true;
        }
        run_chunk(pool, held, pool.taken++);
    }
    pool.done.wait(held, [&] { return pool.finished == pool.chunks; });
    return pool.runners;
}

} // namespace satellite::limits::pool
