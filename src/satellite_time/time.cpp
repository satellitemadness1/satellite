// The clock read and the wait. See satellite_time/time.hpp for the two-clock
// split and whose decision it is.

#include "satellite_time/time.hpp"

#include <cerrno>
#include <chrono>
#include <ctime>

namespace satellite::time {

long long now_nanoseconds()
{
    // system_clock is the Unix epoch BY THE STANDARD since C++20, not by
    // convention -- which is what lets the value mean the same thing to the
    // process that reads it back next year on another machine.
    const auto since = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(since).count();
}

SleepEnd sleep_nanoseconds(long long count, bool (*interrupted)())
{
    if (count <= 0)
        return SleepEnd::finished;

    // AN ABSOLUTE DEADLINE, NOT A REMAINING COUNT. clock_nanosleep with
    // TIMER_ABSTIME resumes after EINTR with no arithmetic and no drift --
    // the relative form would need the remainder read back and re-slept, and
    // every read-back is a place to lose a few microseconds per signal. The
    // clock is CLOCK_MONOTONIC because libstdc++'s steady_clock is exactly
    // that, and the deadline must not move when NTP steps the wall clock --
    // satellite_random/random.cpp static_asserts the same property for the
    // spin one module over.
    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    deadline.tv_sec += count / 1000000000;
    deadline.tv_nsec += count % 1000000000;
    if (deadline.tv_nsec >= 1000000000) {
        deadline.tv_nsec -= 1000000000;
        deadline.tv_sec += 1;
    }

    for (;;) {
        const int answer =
            clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, nullptr);
        if (answer == 0)
            return SleepEnd::finished;

        // EINTR is the SIGINT handler having run on this thread -- interrupt
        // .cpp installs without SA_RESTART so that a blocked wait CAN be
        // woken. Whether it was Ctrl-C is the flag's to say: a signal that
        // was not one (a debugger's, a profiler's timer) resumes the wait,
        // which the absolute deadline makes free.
        if (answer == EINTR && interrupted != nullptr && interrupted())
            return SleepEnd::interrupted;
        if (answer != EINTR)
            return SleepEnd::finished; // nothing sane left to wait on
    }
}

} // namespace satellite::time
