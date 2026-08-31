// How many threads and how many cores this run may use. See
// system_facts/facts.hpp.
//
// PORTED FROM THE FIRST SATELLITE'S host_facts.cpp, which was 44 lines and four
// functions. One of them arrives -- hardware_threads() -- and it arrives
// CHANGED; physical_cores() is new, because nothing in v1 ever needed a core
// count. username(), home_dir() and cwd() stayed behind with M9, and facts.hpp
// says why.
//
// THE CHANGE IS THE AFFINITY MASK, AND IT IS THE WHOLE OF WHAT THIS FILE ADDS.
// v1 answered std::thread::hardware_concurrency(), which on glibc is
// sysconf(_SC_NPROCESSORS_ONLN) -- the machine's online CPUs, no matter what
// this process is allowed to run on. PLAN §4.5.1.2 makes the pool start at
// startup ALWAYS and size itself from this number, so under `taskset -c 0-3`
// that rule would spawn 24 threads onto 4 CPUs, on every run, for the life of
// the process. sched_getaffinity is the question actually being asked: not what
// the machine has, but what this run may use.
//
// AND THAT IS A THIRD READING OF ONE NUMBER, WHICH IS WORTH NAMING BEFORE M20
// FINDS IT. PLAN §4.5.4 already separates a SETTING (THREAD_COUNT -- what satl
// may use) from a FACT (satellite.library.main.arguments.machine.threads -- what
// the machine has). The affinity mask is a third: what the OS will let this
// process use, which is a fact about the RUN. This file answers the third, and
// it is the right fallback for the setting precisely because it is the smallest
// honest number. When M20 builds the language's `threads` it wants the machine's
// total instead, and that is sysconf(_SC_NPROCESSORS_ONLN) -- a second reader,
// four lines, named here so it is a decision rather than a discovery.

#include "system_facts/facts.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <sched.h>
#include <unistd.h>

namespace satellite::facts {

namespace {

// Every CPU index this process may run on, or empty when the kernel will not
// say.
//
// cpu_set_t IS FIXED AT 1024 CPUS and this does not grow it with CPU_ALLOC.
// sched_getaffinity answers EINVAL on a machine with more, which lands in the
// empty return and the sysconf fallback above the call -- a correct answer by a
// slower route, on a machine nothing here will ever run on. CPU_ALLOC would be
// two allocations and a size loop to make that path exact; it is written down
// instead.
std::vector<unsigned> allowed_cpus()
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    if (sched_getaffinity(0, sizeof mask, &mask) != 0)
        return {};

    std::vector<unsigned> cpus;
    for (unsigned i = 0; i < CPU_SETSIZE; i++)
        if (CPU_ISSET(i, &mask))
            cpus.push_back(i);
    return cpus;
}

// One integer out of a /sys topology file, or false.
bool sysfs_number(const std::string &path, long &into)
{
    std::FILE *file = std::fopen(path.c_str(), "r");
    if (!file)
        return false;
    long value = 0;
    const int got = std::fscanf(file, "%ld", &value);
    std::fclose(file);
    if (got != 1)
        return false;
    into = value;
    return true;
}

} // namespace

unsigned hardware_threads()
{
    const std::vector<unsigned> cpus = allowed_cpus();
    if (!cpus.empty())
        return static_cast<unsigned>(cpus.size());

    const long online = sysconf(_SC_NPROCESSORS_ONLN);
    return online > 0 ? static_cast<unsigned>(online) : 1u;
}

// Distinct (physical_package_id, core_id) pairs over the CPUs this run may use.
//
// THE PAIR IS WHAT THE KERNEL ITSELF MEANS BY A CORE, which is why this counts
// rather than estimates. On this machine, 2026-08-30: 24 CPUs in the mask,
// 12 distinct pairs, and PLAN §4.5's `lscpu` reading of "24 hardware threads,
// 12 physical cores" is reproduced from a different source.
//
// NEVER threads / 2. That is right on this Xeon and wrong on every machine
// without SMT, wrong again on a POWER machine with four or eight threads to a
// core, and PLAN §4.5.1.2 already refuses the same arithmetic in the other
// direction -- "not cores x 2, which is this CPU's SMT ratio and not a rule."
// The ratio is not a rule in either direction.
//
// A MASKED /sys ANSWERS hardware_threads(), which overstates the cores on an
// SMT machine and is the honest failure available: it can never claim MORE
// cores than there are threads, and a caller that oversubscribes by two is in a
// place it can survive. A container with /sys hidden is the ordinary case for
// this, not a corner.
unsigned physical_cores()
{
    const std::vector<unsigned> cpus = allowed_cpus();
    if (cpus.empty())
        return hardware_threads();

    std::vector<uint64_t> pairs;
    pairs.reserve(cpus.size());
    for (const unsigned cpu : cpus) {
        const std::string base =
            "/sys/devices/system/cpu/cpu" + std::to_string(cpu) + "/topology/";
        long package = 0;
        long core = 0;
        if (!sysfs_number(base + "physical_package_id", package))
            continue;
        if (!sysfs_number(base + "core_id", core))
            continue;
        pairs.push_back((static_cast<uint64_t>(package) << 32) |
                        static_cast<uint32_t>(core));
    }

    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
    return pairs.empty() ? hardware_threads()
                         : static_cast<unsigned>(pairs.size());
}

} // namespace satellite::facts
