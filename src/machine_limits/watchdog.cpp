// The memory watchdog. See machine_limits/watchdog.hpp.

#include "machine_limits/watchdog.hpp"

#include "machine_limits/limits.hpp"
#include "programs/opening.hpp"
#include "system_facts/facts.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include <unistd.h>

namespace satellite::limits {

namespace {

// How often it looks. M6's done-when asks for "the process gone within a second
// of the threshold being crossed", which is this number and is the whole of
// what it promises: a run that crosses the line just after a wake-up lives for
// one more second, by construction.
constexpr std::chrono::seconds kEverySecond{1};

// Say it and go.
//
// ONE LINE IN PLAIN WORDS AND NO CODE, which errors.def's S08xx block note
// argues at length: a code is for looking up and a span is for something inside
// a file, and a process that has run out of the memory it was allowed has
// neither. Rendering `satl: error S08xx:` over this would dress a shutdown up
// as a diagnostic about a file that is fine.
//
// fflush BEFORE _exit, BECAUSE _exit FLUSHES NOTHING. That is the price of not
// running static destructors from a detached thread, and it is one call.
//
// AND FROM M10 IT DOES NOT REACH EVERYTHING, WHICH IS STATED HERE RATHER THAN
// DISCOVERED. `satellite_console/console.hpp` puts a QUEUE above stdio: a line
// a program displayed is a string waiting for the printer thread, and `fflush`
// reaches only what the printer has already written. So LINES QUEUED AND NOT
// YET WRITTEN ARE LOST WHEN THIS FIRES.
//
// DRAINING HERE IS NOT THE FIX AND MUST NOT BE ADDED. This is a detached thread
// killing a process for taking too much memory; waiting on the printer is
// exactly what it may never do, because a stuck printer would hang the one
// thing whose job is to not hang. It is `_exit` over `exit` again -- the same
// argument, one layer up, and reached from the same direction. PLAN §8's M22
// entry is where the cost is revisited, because M22 is already the emergency
// path's only registrar.
[[noreturn]] void stop(const std::string &because)
{
    std::fprintf(stderr, "\nsatl: stopping -- %s\n", because.c_str());
    std::fflush(nullptr);
    _exit(EXIT_LIMIT);
}

void watch()
{
    for (;;) {
        std::this_thread::sleep_for(kEverySecond);

        const Held &now = held();

        // THE READ FAILING IS NOT THE SAME AS USING NOTHING, and v1 made the
        // same call for its own check: a /proc that will not answer gives 0,
        // and killing a healthy process because a file could not be opened is
        // the worst thing a watchdog can do.
        // THE CEILING IS ASKED FOR ONCE PER WAKE-UP AND NOT HELD, because
        // Setting::value() is a read and the row it reads may be the machine's
        // own total. That is one /proc/meminfo open a second on a thread whose
        // whole job is to open /proc once a second, and it is the reading a
        // watchdog should be doing: system_facts/facts.hpp's rule is that
        // nothing is cached, "so a cached answer would be the wrong one by
        // definition".
        const unsigned long long ceiling = now.memory_max.value();
        const unsigned long long using_now = facts::process_memory_bytes();
        if (using_now != 0 && using_now > ceiling)
            stop("this run is using " + human_bytes(using_now) +
                 " and MEMORY_MAX is " + human_bytes(ceiling) +
                 " (" + std::string(origin_text(now.memory_max.origin)) + ")");

        const Dial &floor = now.dial(DialId::MinFreeMb);
        if (!floor.set)
            continue;
        const unsigned long available = facts::mem_available_mb();
        if (available != 0 && available < floor.value)
            stop("this machine has " + std::to_string(available) +
                 " MB of memory available and min_free_mb asks for " +
                 std::to_string(floor.value));
    }
}

} // namespace

void start_watchdog()
{
    std::thread(watch).detach();
}

int hold_for_the_watchdog()
{
    const Held &now = held();
    std::printf("satl: watching. MEMORY_MAX is %s, from %s.\n",
                human_bytes(now.memory_max.value()).c_str(),
                std::string(origin_text(now.memory_max.origin)).c_str());
    if (const Dial &floor = now.dial(DialId::MinFreeMb); floor.set)
        std::printf("      min_free_mb is %llu, from %s.\n", floor.value,
                    std::string(origin_text(floor.origin)).c_str());
    else
        std::printf("      min_free_mb is unset, so the machine's free memory "
                    "is not watched.\n");
    std::printf("      This run is using %s. Interrupt to stop.\n",
                human_bytes(facts::process_memory_bytes()).c_str());
    std::fflush(stdout);

    // A SLEEP AND NOT A SPIN, and not a join either: the watchdog thread is
    // detached, so there is nothing to join, and a loop that burned a core
    // while waiting to be told the machine is short of memory would be its own
    // joke. It leaves through _exit inside stop(), or through the terminal's
    // interrupt, which is the ordinary way a person ends this.
    for (;;)
        std::this_thread::sleep_for(kEverySecond);

    return EXIT_FINE;
}

} // namespace satellite::limits
