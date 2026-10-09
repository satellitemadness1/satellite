#pragma once
// `satl --config` -- MEASURE THE MACHINE ONCE, AND NEVER AGAIN.
// SATELLITE_ARGUMENTS C1-C8 and D7-D9.
//
// The author, 2026-09-17: *"let's just create a function that creates once on a
// config step... we can run a satl --config and it will configure the system,
// one of the things that it does is it checks how many threads it can possibly
// create on the machine."*
//
// IT IS --rebuild's SIBLING AND THE SAME ARGUMENT MAKES IT AFFORDABLE. 9.6
// seconds is unthinkable at every start-up and nothing at all once per machine.
// --rebuild composes what the PERSON asked for; this measures what the MACHINE
// can do. Neither is on a path anybody waits on twice.
//
// TWO PROPERTIES DECIDE WHETHER THIS IS SAFE, AND BOTH WERE MEASURED RATHER THAN
// ARGUED (SATELLITE_ARGUMENTS Part 4's tables):
//
//   1. EVERY THREAD PARKS ON A CONDITION VARIABLE AND NEVER SPINS. 400,000
//      PARKED threads is 9.6 seconds and 3.3 GB and the machine stutters once.
//      200,000 RUNNABLE threads against 24 CPUs is a scheduler death spiral and
//      it cost a hard reboot on 2026-09-16. The difference is entirely this.
//   2. IT STOPS AT A CEILING /proc STATES, LESS HEADROOM -- never at failure.
//      `threads-max` is SYSTEM-WIDE: a probe that runs until it cannot make one
//      more has taken the last thread on the machine, and the desktop cannot
//      make one either. Headroom is not politeness, it is the difference between
//      a stutter and a login shell that cannot fork.
//
// A THREAD'S STACK IS LAZILY COMMITTED, which is why the count is large and the
// memory is not. 50,000 threads asking 8 MiB each took 419 MB resident -- about
// 8.33 kB apiece -- and that barely moves whether the stack asked for is 256 kB
// or 8 MiB. Stack size costs ADDRESS SPACE, which this machine has 128 TB of.
// The first version of that table said stack size was the binding limit and it
// was wrong; measuring said so.

#include "config_file.hpp"
#include "../machine/machine_codes.hpp"

#include <pthread.h>
#include <sys/resource.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

namespace satellite004 {

// ONE CEILING, AND WHERE IT CAME FROM. `read` false means this machine does not
// state it -- which is not the same as zero, and must never join the minimum.
struct ThreadCeiling {
    std::string source;
    unsigned long long int threads = 0;
    bool read = false;
};

namespace machine_probe {

// The first whole number in a file, or nothing. Every /proc ceiling is one.
inline bool first_number_in(const char *path, unsigned long long int &out)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;
    unsigned long long int value = 0;
    if (!(file >> value)) return false;
    out = value;
    return true;
}

// MemAvailable, in kilobytes, out of /proc/meminfo.
inline bool memory_available_kb(unsigned long long int &out)
{
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return false;
    std::string name;
    while (file >> name) {
        if (name == "MemAvailable:") {
            unsigned long long int value = 0;
            if (!(file >> value)) return false;
            out = value;
            return true;
        }
        std::string rest;
        std::getline(file, rest);
    }
    return false;
}

// 8.33 kB of real memory per parked thread, MEASURED 2026-09-17 across a 50x
// range (8,192 / 100,000 / 400,000 threads) and perfectly linear. Kept as
// hundredths so the arithmetic stays in whole numbers.
inline constexpr unsigned long long int kResidentPerThreadHundredthsKb = 833;

// THE HEADROOM, AND IT IS THREE QUARTERS. The author's own run stopped at
// 400,000 of a 506,566 ceiling -- 79% -- on purpose. Three quarters is that
// number rounded to something a person can hold, and it leaves a quarter of
// every thread on the machine to the desktop, the shell and whatever else is
// running. Raising it is one line, and the line is here.
inline constexpr unsigned long long int kHeadroomOver = 3;
inline constexpr unsigned long long int kHeadroomUnder = 4;

} // namespace machine_probe

// D7 -- EVERY THREAD CEILING THIS SYSTEM REPORTS, each one named, so a person
// can see WHICH one bound and go raise that one.
//
// `nproc` IS DELIBERATELY NOT HERE. 24 is lower than every ceiling on every
// machine, so putting it in the set would make the minimum always `nproc` and
// the other seven sources dead code. It is a count of CPUs, not a limit on
// threads, and it is already reported on its own as arguments.machine.cores.
//
// `arguments.threads_startup` IS NOT HERE EITHER: it is how many to START, not
// how many may EXIST.
inline std::vector<ThreadCeiling> read_thread_ceilings()
{
    using namespace machine_probe;
    std::vector<ThreadCeiling> ceilings;
    unsigned long long int value = 0;

    ThreadCeiling threads_max{"/proc/sys/kernel/threads-max", 0, false};
    if (first_number_in("/proc/sys/kernel/threads-max", value)) { threads_max.threads = value; threads_max.read = true; }
    ceilings.push_back(threads_max);

    ThreadCeiling pid_max{"/proc/sys/kernel/pid_max", 0, false};
    if (first_number_in("/proc/sys/kernel/pid_max", value)) { pid_max.threads = value; pid_max.read = true; }
    ceilings.push_back(pid_max);

    // TWO MAPPINGS PER STACK -- the stack itself and its guard page -- so the
    // map count buys half as many threads as it names.
    ThreadCeiling map_count{"/proc/sys/vm/max_map_count / 2", 0, false};
    if (first_number_in("/proc/sys/vm/max_map_count", value)) { map_count.threads = value / 2; map_count.read = true; }
    ceilings.push_back(map_count);

    ThreadCeiling by_memory{"MemAvailable / 8.33 kB a thread", 0, false};
    if (memory_available_kb(value)) {
        by_memory.threads = (value * 100ULL) / kResidentPerThreadHundredthsKb;
        by_memory.read = true;
    }
    ceilings.push_back(by_memory);

    // RLIMIT_NPROC, and unlimited is NOT a ceiling of zero.
    ThreadCeiling by_rlimit{"ulimit -u (RLIMIT_NPROC)", 0, false};
    struct rlimit limit {};
    if (getrlimit(RLIMIT_NPROC, &limit) == 0 && limit.rlim_cur != RLIM_INFINITY) {
        by_rlimit.threads = static_cast<unsigned long long int>(limit.rlim_cur);
        by_rlimit.read = true;
    }
    ceilings.push_back(by_rlimit);

    // The cgroup's, which is what binds inside a container and nowhere else.
    // "max" is the word for unlimited and first_number_in reads no number from it.
    ThreadCeiling by_cgroup{"cgroup pids.max", 0, false};
    if (first_number_in("/sys/fs/cgroup/pids.max", value)) { by_cgroup.threads = value; by_cgroup.read = true; }
    ceilings.push_back(by_cgroup);

    return ceilings;
}

// D8 -- THE LOWEST OF THEM. The author, 2026-09-17: *"grab the thread_count from
// all sources, and then settle with the lowest number."* Nothing is ever
// spawned to answer this: every number above is a file read.
inline unsigned long long int lowest_thread_ceiling(const std::vector<ThreadCeiling> &ceilings,
                                                   std::string *which = nullptr)
{
    unsigned long long int lowest = 0;
    for (const ThreadCeiling &ceiling : ceilings) {
        if (ceiling.read == false || ceiling.threads == 0) continue;
        if (lowest == 0 || ceiling.threads < lowest) {
            lowest = ceiling.threads;
            if (which != nullptr) *which = ceiling.source;
        }
    }
    return lowest;
}

// The ceiling less headroom: the most this probe will ever ask for.
inline unsigned long long int probe_target(unsigned long long int lowest)
{
    using namespace machine_probe;
    return (lowest / kHeadroomUnder) * kHeadroomOver;
}

// ONE RUNG OF THE RAMP.
struct ProbeStep {
    unsigned long long int threads = 0;
    unsigned long long int nanoseconds_each = 0;
    unsigned long long int resident_bytes = 0;
    bool reached = false;   // false means the machine refused before this rung
};

struct ProbeResult {
    unsigned long long int measured = 0;      // threads alive at once, at the end
    unsigned long long int target = 0;        // what it was aiming for
    unsigned long long int lowest_ceiling = 0;
    std::string binding_source;
    std::vector<ProbeStep> steps;
    bool refused_early = false;               // the machine said no before the target
    bool capped = false;                      // the target was a cap asked for, not the machine's own
};

namespace machine_probe {

// This process's resident size, in bytes. /proc/self/statm field 2 is pages.
inline unsigned long long int resident_bytes()
{
    std::ifstream file("/proc/self/statm");
    if (!file.is_open()) return 0;
    unsigned long long int total = 0, resident = 0;
    if (!(file >> total >> resident)) return 0;
    return resident * static_cast<unsigned long long int>(sysconf(_SC_PAGESIZE));
}

// WHAT EVERY PROBED THREAD DOES: take the lock, wait to be released, let go.
// It never spins, never wakes on its own, and never touches a shared counter --
// so 400,000 of these cost the scheduler nothing at all.
struct ParkingLot {
    pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t wake = PTHREAD_COND_INITIALIZER;
    bool released = false;
};

inline void *park(void *lot_input)
{
    ParkingLot *lot = static_cast<ParkingLot *>(lot_input);
    pthread_mutex_lock(&lot->gate);
    while (lot->released == false)
        pthread_cond_wait(&lot->wake, &lot->gate);
    pthread_mutex_unlock(&lot->gate);
    return nullptr;
}

// 256 kB, which is what the 2026-09-17 table was measured at. It is ADDRESS
// SPACE and not memory -- the resident cost was 8.33 kB whatever this says --
// but a smaller number keeps max_map_count and the address space honest.
inline constexpr std::size_t kProbeStackBytes = 256 * 1024;

} // namespace machine_probe

// C2/C3/C4 -- THE PROBE. Ramp in doubling steps, every thread parked, and stop
// at the target rather than at a refusal. Threads stay alive across rungs, so
// `resident_bytes` is the real cost of holding that many at once.
inline ProbeResult probe_threads(unsigned long long int target,
                                 unsigned long long int start_at = 1024)
{
    using namespace machine_probe;
    ProbeResult result;
    result.target = target;
    if (target == 0) return result;

    ParkingLot lot;
    std::vector<pthread_t> parked;
    parked.reserve(static_cast<std::size_t>(target < 1000000 ? target : 1000000));

    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setstacksize(&attributes, kProbeStackBytes);

    const auto began = std::chrono::steady_clock::now();
    for (unsigned long long int rung = start_at; ; rung *= 2) {
        const unsigned long long int want = rung < target ? rung : target;
        ProbeStep step;
        step.threads = want;
        bool refused = false;
        while (parked.size() < want) {
            pthread_t thread{};
            if (pthread_create(&thread, &attributes, &park, &lot) != 0) {
                refused = true;
                result.refused_early = true;
                break;
            }
            parked.push_back(thread);
        }
        const auto now = std::chrono::steady_clock::now();
        const unsigned long long int so_far =
            static_cast<unsigned long long int>(std::chrono::duration_cast<std::chrono::nanoseconds>(now - began).count());
        step.threads = parked.size();
        step.reached = (refused == false);
        step.nanoseconds_each = parked.empty() ? 0 : so_far / parked.size();
        step.resident_bytes = resident_bytes();
        result.steps.push_back(step);
        if (refused == true || want >= target) break;
    }
    result.measured = parked.size();

    // LET EVERY ONE OF THEM GO, then wait for all of them. A thread left parked
    // is a thread still holding its stack, and the next thing satl does would be
    // measuring around them.
    pthread_mutex_lock(&lot.gate);
    lot.released = true;
    pthread_cond_broadcast(&lot.wake);
    pthread_mutex_unlock(&lot.gate);
    for (pthread_t thread : parked)
        pthread_join(thread, nullptr);
    pthread_attr_destroy(&attributes);
    return result;
}

// C5 -- WHERE THE ANSWER LIVES. Its own file and not config.ini, because
// config.ini is the PERSON'S and this is the MACHINE'S: a person edits one and
// has no business editing the other, and copying config.ini to a new machine
// should not copy a thread count measured somewhere else.
inline std::string machine_conf_path()
{
    const std::string where = config_file::folder();
    if (where.empty()) return std::string();   // HOME is unset: a real state, and not a path
    return where + "/machine.conf";
}

inline bool write_machine_conf(const ProbeResult &result)
{
    const std::string path = machine_conf_path();
    if (path.empty()) return false;
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) return false;
    const std::time_t when = std::time(nullptr);
    char stamp[32] = {0};
    std::tm broken {};
    if (localtime_r(&when, &broken) != nullptr)
        std::strftime(stamp, sizeof stamp, "%Y-%m-%d %H:%M:%S", &broken);
    file << "# satellite 004 -- what THIS machine measured. Written by satl --config.\n"
         << "#\n"
         << "# Not config.ini: that file is yours and this one is the machine's. Copying\n"
         << "# it to another machine copies a measurement taken somewhere else.\n"
         << "#\n"
         << "# Delete it and satl answers from the /proc ceilings instead, and never probes.\n"
         << "\n"
         << (result.capped
                 ? "# CAPPED RUN. `satl --config <most>` was asked for a number, so threads_measured\n"
                   "# below is that cap and NOT what this machine allows -- the probe stopped where it\n"
                   "# was told to, not where the machine did. Run `satl --config` with no number for\n"
                   "# the machine's own answer.\n"
                 : "")
         << "capped = " << (result.capped ? "true" : "false") << "\n"
         << "threads_measured = " << result.measured << "\n"
         << "threads_ceiling = " << result.lowest_ceiling << "\n"
         << "ceiling_source = " << result.binding_source << "\n"
         << "measured_on = " << stamp << "\n";
    return file.good();
}

// C6 -- the measured count, or 0 when no probe has ever run here.
inline unsigned long long int read_machine_conf_threads()
{
    const std::string path = machine_conf_path();
    if (path.empty()) return 0;
    std::ifstream file(path);
    if (!file.is_open()) return 0;
    std::string line;
    while (std::getline(file, line)) {
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) continue;
        std::string key = line.substr(0, equals);
        while (!key.empty() && (key.back() == ' ' || key.back() == '\t')) key.pop_back();
        if (key != "threads_measured") continue;
        return std::strtoull(line.c_str() + equals + 1, nullptr, 10);
    }
    return 0;
}

// C6/C7 -- WHAT arguments.machine.threads ANSWERS, AND IT NEVER SPAWNS ANYTHING.
// The measurement when there is one; otherwise the lowest ceiling, which is a
// number /proc states for free. A person who wants the measured answer runs
// `satl --config` once; a person who does not gets a true answer anyway.
inline unsigned long long int threads_this_machine_allows()
{
    const unsigned long long int measured = read_machine_conf_threads();
    if (measured != 0) return measured;
    return lowest_thread_ceiling(read_thread_ceilings());
}

} // namespace satellite004
