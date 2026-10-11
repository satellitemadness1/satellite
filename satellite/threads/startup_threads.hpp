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
#include <string>
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

    // THE AUTHOR'S TOPOLOGY, 2026-09-16: "we are going to have the main thread,
    // after it starts 1 thread, that 1 thread starts 256 threads". Main asks and
    // is free the same instant; the one thread it started does the 256.
    //
    // WHY IT IS WORTH A THREAD TO START THREADS. Parking 256 costs about 12 ms,
    // and main has real work waiting -- the number index and the function table,
    // which need no thread at all. Overlapping the two is the first place this
    // interpreter does two things at once.
    //
    // IT REPORTS NOTHING. MachineState is not thread-safe and main writes to it
    // throughout, so the starter records what happened and wait_until_warm says
    // it on the CALLING thread. A pool that raced the state it reported through
    // would be a poor advertisement for the parallel machine.
    void start_in_background(unsigned long long int count);

    // Waits for start_in_background, then reports on THIS thread and answers the
    // code it would have answered. Safe to call when nothing was started.
    signed long long int wait_until_warm(MachineState &state);

    // Hands a job to one parked thread; with no warm thread, runs it here.
    void submit(std::function<void()> job);

    // How many threads have started and parked at least once.
    unsigned long long int warm() const;

private:
    void park_and_run();
    void start_quietly(unsigned long long int count);   // no state: see start_in_background

    mutable std::mutex mutex_;
    std::condition_variable work_arrived_;
    std::condition_variable parked_;
    std::deque<std::function<void()>> jobs_;
    std::vector<std::thread> threads_;
    unsigned long long int parked_count_ = 0;
    bool stopping_ = false;

    // What the background starter found, read only after it is joined.
    std::thread starter_;
    unsigned long long int asked_for_ = 0;
    unsigned long long int warm_count_ = 0;
    signed long long int start_code_ = 0;
    std::string refusal_;
    long double milliseconds_ = 0;
};

// WHAT THE STATUS BAR ACROSS satl'S OWN CONSOLE COUNTS (console_status.cpp): the
// pool's threads that are up, and those running a job this instant. One pool a
// process, so two counters for the process; a job moves the second once each way,
// relaxed, and the desk reads them twice a second.
unsigned long long int pool_threads_up();
unsigned long long int pool_threads_busy();

} // namespace satellite004
