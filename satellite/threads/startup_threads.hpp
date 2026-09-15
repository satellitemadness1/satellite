#pragma once
// threads/startup_threads.hpp -- the threads started before the program runs.
//
// (the author, 2026-09-15) "Let's make sure that 256 threads are warm, and get
// the number 256 from return_arguments_vector() inside of satellite_config.hpp."
//
// arguments.threads_startup threads start before satellite.main runs, and each
// PARKS on one condition variable. Handing them a batch later costs a recall
// (about 12,486 ns, DESIGN §13), never a thread start (about 28,254 ns).
// start() returns only once every thread has reported that it is parked, so
// "warm" is checked rather than assumed.
//
// Nothing hands them work yet: that is PLAN M7, the runtime that runs a .satb.
// submit() is the door M7 will use. A job that is queued when the program ends
// still runs before its thread stops.
//
// A CEILING, NEVER AN INSTRUCTION (DESIGN §1). The count is never more than
// arguments.threads_max. If the machine refuses a thread, the threads that did
// start stay warm, and the refusal is reported with thread_start_error (21).

#include "../machine/machine_state.hpp"

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace satellite004 {

class StartupThreads {
public:
    StartupThreads() = default;
    StartupThreads(const StartupThreads &) = delete;
    StartupThreads &operator=(const StartupThreads &) = delete;

    // Stops every thread once the queued jobs have run, and joins them all.
    ~StartupThreads();

    // Starts `count` threads and waits until every one is parked. Answers
    // success, or thread_start_error when the machine started fewer.
    signed long long int start(unsigned long long int count, MachineState &state);

    // Hands a job to one parked thread; with no warm thread, runs it here.
    void submit(std::function<void()> job);

    // How many threads have started and parked at least once.
    unsigned long long int warm() const;

private:
    void park_and_run();

    mutable std::mutex mutex_;
    std::condition_variable work_arrived_;
    std::condition_variable parked_;
    std::deque<std::function<void()>> jobs_;
    std::vector<std::thread> threads_;
    unsigned long long int parked_count_ = 0;
    bool stopping_ = false;
};

} // namespace satellite004
