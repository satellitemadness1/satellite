// threads/startup_threads.cpp -- see startup_threads.hpp.

#include "startup_threads.hpp"

#include "../machine/machine_codes.hpp"

#include <chrono>
#include <cstdio>
#include <exception>
#include <string>
#include <utility>

namespace satellite004 {

StartupThreads::~StartupThreads()
{
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
    parked_.notify_all();
    for (;;) {
        work_arrived_.wait(lock, [this] { return stopping_ || !jobs_.empty(); });
        if (jobs_.empty())
            return;
        std::function<void()> job = std::move(jobs_.front());
        jobs_.pop_front();
        lock.unlock();
        job();
        lock.lock();
    }
}

signed long long int StartupThreads::start(unsigned long long int count, MachineState &state)
{
    const auto began = std::chrono::steady_clock::now();
    signed long long int code = success;
    std::string refusal;
    for (unsigned long long int started = 0; started < count; started++) {
        try {
            threads_.emplace_back(&StartupThreads::park_and_run, this);
        } catch (const std::exception &refused) {
            refusal = refused.what();
            code = thread_start_error;
            break;
        }
    }

    unsigned long long int warm_count = 0;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        parked_.wait(lock, [this] { return parked_count_ == threads_.size(); });
        warm_count = parked_count_;
    }
    const long double milliseconds =
        std::chrono::duration<long double, std::milli>(std::chrono::steady_clock::now() - began).count();

    if (code != success)
        report_error("threads.startup(refused): the machine started " + std::to_string(warm_count) + " of " +
                         std::to_string(count) + " threads (" + refusal + "); those " +
                         std::to_string(warm_count) + " are warm",
                     code);
    char time[64];
    std::snprintf(time, sizeof time, "%.3Lf", milliseconds);
    state.set("threads.startup(warm): " + std::to_string(warm_count) + " threads parked in " + time + " ms",
              success);
    return code;
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
