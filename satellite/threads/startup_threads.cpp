// threads/startup_threads.cpp -- see startup_threads.hpp.

#include "startup_threads.hpp"

#include "../machine/machine_codes.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <exception>
#include <string>
#include <utility>

namespace satellite004 {

namespace {
std::atomic<unsigned long long int> threads_up{0};
std::atomic<unsigned long long int> threads_busy{0};
} // namespace

unsigned long long int pool_threads_up() { return threads_up.load(std::memory_order_relaxed); }
unsigned long long int pool_threads_busy() { return threads_busy.load(std::memory_order_relaxed); }

StartupThreads::~StartupThreads()
{
    // THE STARTER IS JOINED FIRST, AND FORGETTING IT WAS A REAL CRASH. main can
    // return before it ever waits -- `index.load` refusing with no libraries is
    // the path check.sh walks -- and destroying a joinable std::thread calls
    // std::terminate. That turned a clean refusal of 5 into SIGABRT, 134.
    //
    // It is joined BEFORE `stopping_` is set, because it is still filling
    // threads_ and waiting for them to park: stopping them underneath it would
    // have it waiting on a count that no longer moves.
    if (starter_.joinable())
        starter_.join();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    work_arrived_.notify_all();
    for (std::thread &thread : threads_)
        thread.join();
}

void StartupThreads::park_and_run()
{
    std::unique_lock<std::mutex> lock(mutex_);
    parked_count_++;
    threads_up.fetch_add(1, std::memory_order_relaxed);
    parked_.notify_all();
    for (;;) {
        work_arrived_.wait(lock, [this] { return stopping_ || !jobs_.empty(); });
        if (jobs_.empty()) {
            threads_up.fetch_sub(1, std::memory_order_relaxed);
            return;
        }
        std::function<void()> job = std::move(jobs_.front());
        jobs_.pop_front();
        lock.unlock();
        threads_busy.fetch_add(1, std::memory_order_relaxed);
        job();
        threads_busy.fetch_sub(1, std::memory_order_relaxed);
        lock.lock();
    }
}

// The thread-starting work, with nothing said about it. Every number it finds is
// kept for wait_until_warm to report on the caller's thread.
void StartupThreads::start_quietly(unsigned long long int count)
{
    const auto began = std::chrono::steady_clock::now();
    asked_for_ = count;
    start_code_ = success;
    for (unsigned long long int started = 0; started < count; started++) {
        try {
            threads_.emplace_back(&StartupThreads::park_and_run, this);
        } catch (const std::exception &refused) {
            refusal_ = refused.what();
            start_code_ = thread_start_error;
            break;
        }
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        parked_.wait(lock, [this] { return parked_count_ == threads_.size(); });
        warm_count_ = parked_count_;
    }
    milliseconds_ =
        std::chrono::duration<long double, std::milli>(std::chrono::steady_clock::now() - began).count();
}

void StartupThreads::start_in_background(unsigned long long int count)
{
    // ONE THREAD, and it is the only one main starts. It starts the rest.
    starter_ = std::thread(&StartupThreads::start_quietly, this, count);
}

signed long long int StartupThreads::wait_until_warm(MachineState &state)
{
    if (starter_.joinable())
        starter_.join();

    if (start_code_ != success)
        report_error("threads.startup(refused): the machine started " + std::to_string(warm_count_) + " of " +
                         std::to_string(asked_for_) + " threads (" + refusal_ + "); those " +
                         std::to_string(warm_count_) + " are warm",
                     start_code_);
    char time[64];
    std::snprintf(time, sizeof time, "%.3Lf", milliseconds_);
    state.set("threads.startup(warm): " + std::to_string(warm_count_) + " threads parked in " + time + " ms",
              success);
    return start_code_;
}

// The straight-through form: start them and wait, on this thread. Kept because a
// caller with nothing else to do should not have to know about the topology.
signed long long int StartupThreads::start(unsigned long long int count, MachineState &state)
{
    start_quietly(count);
    return wait_until_warm(state);
}

void StartupThreads::submit(std::function<void()> job)
{
    // No warm thread (threads_startup 0, threads_max 0, or the machine refused
    // them all): the job runs here, on the calling thread, never silently not at all.
    if (threads_.empty()) {
        job();
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        jobs_.push_back(std::move(job));
    }
    work_arrived_.notify_one();
}

unsigned long long int StartupThreads::warm() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return parked_count_;
}

} // namespace satellite004
