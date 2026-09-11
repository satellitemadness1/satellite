// /proc/meminfo, and this process's own resident set. See
// system_facts/facts.hpp.
//
// PORTED FROM THE FIRST SATELLITE'S memory_facts.cpp, which was 236 lines and
// answered eleven questions. Six arrive here. What stayed behind is swap
// (swap_total_bytes, swap_used_bytes) and the SMBIOS type 17 read behind
// mem_frequency_mhz() and mem_width_bits() -- forty lines of DMI offsets whose
// own header records that /sys/firmware/dmi/entries is mode 0400 root, "so a run
// as anybody else gets 0". Neither is a LIMIT: satl does not hold itself to the
// swap in a machine or to how fast its memory is clocked, and both belong to
// satellite.system's twenty-eight paths at M20, which PLAN M6 names as the other
// side of this milestone's seam.
//
// ONE FILE BECAUSE THEY READ ONE SOURCE. meminfo_kb() answers five of the six
// below, and a helper that stays static is a helper nothing outside this file
// can reach for.

#include "system_facts/facts.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>

namespace satellite::facts {

namespace {

// One `Key:  12345 kB` row of /proc/meminfo, in kB, or 0.
//
// THE COLON IS CHECKED AND IT IS NOT PEDANTRY. `Mem` is a prefix of `MemTotal`,
// `MemFree` and `MemAvailable`, and a prefix match on the first of those would
// answer whichever row the kernel happens to print first -- which is a wrong
// number rather than no number, and the worst shape a fact can have.
unsigned long meminfo_kb(const char *key)
{
    std::FILE *file = std::fopen("/proc/meminfo", "r");
    if (!file)
        return 0;

    char line[256];
    unsigned long kb = 0;
    const size_t length = std::strlen(key);
    while (std::fgets(line, sizeof line, file)) {
        if (std::strncmp(line, key, length) == 0 && line[length] == ':') {
            kb = std::strtoul(line + length + 1, nullptr, 10);
            break;
        }
    }
    std::fclose(file);
    return kb;
}

unsigned long long kb_to_bytes(unsigned long kb)
{
    return static_cast<unsigned long long>(kb) * 1024ULL;
}

} // namespace

unsigned long long mem_total_bytes()
{
    return kb_to_bytes(meminfo_kb("MemTotal"));
}

unsigned long long mem_available_bytes()
{
    return kb_to_bytes(meminfo_kb("MemAvailable"));
}

// ONE SUBTRACTION IN ONE PLACE, and v1 had it in two -- mem_used_mb() rounded to
// megabytes first and mem_used_bytes() did not, so the two could disagree by up
// to a megabyte about the same instant. The megabyte answers below are all
// derived from the byte ones for that reason.
unsigned long long mem_used_bytes()
{
    const unsigned long long total = mem_total_bytes();
    const unsigned long long available = mem_available_bytes();
    return total > available ? total - available : 0;
}

unsigned long mem_total_mb()
{
    return static_cast<unsigned long>(mem_total_bytes() / (1024ULL * 1024ULL));
}

unsigned long mem_available_mb()
{
    return static_cast<unsigned long>(mem_available_bytes() / (1024ULL * 1024ULL));
}

unsigned long mem_used_mb()
{
    return static_cast<unsigned long>(mem_used_bytes() / (1024ULL * 1024ULL));
}

// --- swap -------------------------------------------------------------------
//
// THE HEADER OF THIS FILE PROMISED THESE TO M20 AND THIS IS M20 COLLECTING.
// Nothing about them is new: they read the same file through the same helper as
// everything above, which is why they land here rather than in a file of their
// own -- "one file because they read one source" is the rule this obeys.
unsigned long long swap_total_bytes()
{
    return kb_to_bytes(meminfo_kb("SwapTotal"));
}

unsigned long long swap_free_bytes()
{
    return kb_to_bytes(meminfo_kb("SwapFree"));
}

// TOTAL MINUS FREE, and NOT total minus available as main memory is.
// /proc/meminfo has no SwapAvailable to read, because swap holds no reclaimable
// cache -- so free IS what is available here and the two readings coincide.
// Saying it in code rather than reusing mem_used_bytes()' shape is what keeps
// the difference visible.
unsigned long long swap_used_bytes()
{
    const unsigned long long total = swap_total_bytes();
    const unsigned long long free_bytes = swap_free_bytes();
    return total > free_bytes ? total - free_bytes : 0;
}

// A MACHINE WITH NO SWAP ANSWERS 0 FOR ALL THREE AND THAT IS TRUTHFUL, not a
// failed read: SwapTotal is present and zero on such a machine, and
// meminfo_kb() answers 0 for a key that is absent as well. The two cases are
// indistinguishable here and do not need distinguishing -- "there is no swap"
// and "this kernel will not say how much swap there is" both mean a program
// gets nothing from it.

// /proc/self/statm field 2 is the resident page count.
//
// statm RATHER THAN status, WHICH IS v1'S CHOICE AND ITS REASON: statm is six
// integers on one line -- no key matching, no unit parsing -- and that matters
// when something calls this in a loop to watch a process grow. The watchdog
// calls it once a second for the life of the run.
unsigned long long process_memory_bytes()
{
    std::FILE *file = std::fopen("/proc/self/statm", "r");
    if (!file)
        return 0;

    unsigned long long total_pages = 0;
    unsigned long long resident_pages = 0;
    const int got = std::fscanf(file, "%llu %llu", &total_pages, &resident_pages);
    std::fclose(file);
    if (got != 2)
        return 0;

    const long page = sysconf(_SC_PAGESIZE);
    if (page <= 0)
        return 0;
    return resident_pages * static_cast<unsigned long long>(page);
}

} // namespace satellite::facts
