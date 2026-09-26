// What this machine's memory is doing: /proc/meminfo, this process's own
// resident set, swap, the DMI Memory Device record, and the watchdog that
// shuts the interpreter down when free memory runs out.
//
// One file because they all read the same two sources -- meminfo_kb() answers
// six of these and the watchdog answers from mem_available_mb() -- and a
// helper that stays static is a helper that cannot be called from anywhere it
// was not meant to be.
//
// Part of src/system_facts/, split from a 653-line system.cpp. See
// system_internal.hpp for what the pieces share and why system.cpp kept what
// it kept.

#include "system_facts/system_internal.hpp"

// For run_emergency_exit_hook(), which the watchdog owes the terminal before
// it calls _exit(2). See the note at that call site.
#include "system_facts/interrupt.hpp"

namespace satellite {

static unsigned long meminfo_kb(const char *key)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f)
        return 0;
    char line[256];
    unsigned long kb = 0;
    size_t keylen = strlen(key);
    while (fgets(line, sizeof line, f)) {
        if (strncmp(line, key, keylen) == 0 && line[keylen] == ':') {
            kb = strtoul(line + keylen + 1, nullptr, 10);
            break;
        }
    }
    fclose(f);
    return kb;
}

unsigned long mem_total_mb()
{
    return meminfo_kb("MemTotal") / 1024;
}

unsigned long mem_available_mb()
{
    return meminfo_kb("MemAvailable") / 1024;
}

unsigned long long mem_total_bytes()
{
    return static_cast<unsigned long long>(meminfo_kb("MemTotal")) * 1024ULL;
}

unsigned long long mem_available_bytes()
{
    return static_cast<unsigned long long>(meminfo_kb("MemAvailable")) * 1024ULL;
}

// Used is total minus AVAILABLE and not total minus free, which is the same
// choice mem_used_mb already made: Linux spends every spare page on cache, so
// "free" on a healthy machine is a small number that alarms people, and
// available is the figure that answers what a program can still have.
unsigned long long mem_used_bytes()
{
    const unsigned long long total = mem_total_bytes();
    const unsigned long long avail = mem_available_bytes();
    return total > avail ? total - avail : 0;
}

// /proc/self/statm's second field is the resident page count, and it is read
// fresh on every call on purpose: the question is what this program is using
// NOW, while it runs, so a cached answer would be the wrong one by definition.
// statm rather than status because it is six integers on one line -- no key
// matching, no unit parsing -- which matters when a program calls this inside
// a loop to watch itself grow.
unsigned long long process_memory_bytes()
{
    FILE *f = fopen("/proc/self/statm", "r");
    if (!f)
        return 0;
    unsigned long long total_pages = 0;
    unsigned long long resident_pages = 0;
    const int got = fscanf(f, "%llu %llu", &total_pages, &resident_pages);
    fclose(f);
    if (got != 2)
        return 0;
    const long page = sysconf(_SC_PAGESIZE);
    if (page <= 0)
        return 0;
    return resident_pages * static_cast<unsigned long long>(page);
}

unsigned long long swap_total_bytes()
{
    return static_cast<unsigned long long>(meminfo_kb("SwapTotal")) * 1024ULL;
}

unsigned long long swap_used_bytes()
{
    const unsigned long long total = swap_total_bytes();
    const unsigned long long free_ =
        static_cast<unsigned long long>(meminfo_kb("SwapFree")) * 1024ULL;
    return total > free_ ? total - free_ : 0;
}

// SMBIOS type 17, the Memory Device record, straight out of the binary table.
// The offsets are the specification's own and are written here as the
// specification writes them:
//
//   0x08  Total Width   bits INCLUDING error-correction -- 72 on ECC, 64 not
//   0x0A  Data Width    bits carrying data alone -- 64 on both
//   0x0C  Size          0 means the slot is empty
//   0x15  Speed         MT/s; 0 or 0xFFFF mean the firmware would not say
//
// The first POPULATED slot answers for the machine. A board with mismatched
// sticks would have more than one answer and this returns the first; reporting
// per-slot would need a list, and a list of one number per slot is a different
// feature from "what is this machine's memory".
static bool dmi_memory_device(unsigned long *speed_mhz, unsigned *width_bits)
{
    DIR *dir = opendir("/sys/firmware/dmi/entries");
    if (!dir)
        return false;
    bool found = false;
    for (;;) {
        errno = 0;
        const struct dirent *entry = readdir(dir);
        if (!entry)
            break;
        // "17-0", "17-1": the type, a dash, the instance.
        if (strncmp(entry->d_name, "17-", 3) != 0)
            continue;

        std::string path = std::string("/sys/firmware/dmi/entries/") +
                           entry->d_name + "/raw";
        FILE *f = fopen(path.c_str(), "rb");
        if (!f)
            continue;               // root-only, which is the usual answer
        unsigned char raw[64] = {0};
        const size_t got = fread(raw, 1, sizeof raw, f);
        fclose(f);
        if (got < 0x17)
            continue;

        const unsigned size = static_cast<unsigned>(raw[0x0C]) |
                              (static_cast<unsigned>(raw[0x0D]) << 8);
        if (size == 0)
            continue;               // an empty slot has nothing to report

        const unsigned total_width = static_cast<unsigned>(raw[0x08]) |
                                     (static_cast<unsigned>(raw[0x09]) << 8);
        const unsigned data_width = static_cast<unsigned>(raw[0x0A]) |
                                    (static_cast<unsigned>(raw[0x0B]) << 8);
        const unsigned speed = static_cast<unsigned>(raw[0x15]) |
                               (static_cast<unsigned>(raw[0x16]) << 8);

        if (width_bits) {
            unsigned bits = total_width;
            if (bits == 0 || bits == 0xFFFF)
                bits = data_width;
            *width_bits = (bits == 0xFFFF) ? 0 : bits;
        }
        if (speed_mhz)
            *speed_mhz = (speed == 0xFFFF) ? 0 : speed;
        found = true;
        break;
    }
    closedir(dir);
    return found;
}

unsigned long mem_frequency_mhz()
{
    unsigned long speed = 0;
    dmi_memory_device(&speed, nullptr);
    return speed;
}

unsigned mem_width_bits()
{
    unsigned bits = 0;
    dmi_memory_device(nullptr, &bits);
    return bits;
}

unsigned long mem_used_mb()
{
    unsigned long total = meminfo_kb("MemTotal");
    unsigned long avail = meminfo_kb("MemAvailable");
    return (total > avail ? total - avail : 0) / 1024;
}

void start_memory_watchdog()
{
    std::thread([] {
        for (;;) {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // §8.1 named this site: it warned that keeping both a double and
            // a Number would make the watchdog "quietly ignore any user
            // retuning of min_free_mb", because the retuned value would land
            // in the alternative this read does not look at. There is only one
            // numeric alternative now, so there is nothing to miss.
            long long min_mb = 4096;
            if (auto v = Library::instance().get("system", "min_free_mb"))
                if (const Number *n = std::get_if<Number>(&*v)) {
                    long long set = 0;
                    // Rounded UP, so a fractional threshold errs toward
                    // shutting down early rather than one megabyte too late.
                    if (n->ceil().to_integer(set) && set >= 0)
                        min_mb = set;
                }

            unsigned long avail = mem_available_mb();
            if (avail == 0)
                continue; // /proc/meminfo unreadable; don't kill on that
            if (static_cast<long long>(avail) < min_mb) {
                // Before the message and before the exit: _exit(2) runs no
                // destructor and no atexit handler, so a terminal the line
                // editor put into raw mode would stay that way after the
                // process is gone -- no echo, no line editing, and nothing on
                // screen to say why. See interrupt.hpp.
                run_emergency_exit_hook();
                fprintf(stderr,
                        "\nsatellite: available memory %lu MB fell below "
                        "%lld MB — shutting down\n",
                        avail, min_mb);
                fflush(nullptr);
                _exit(2);
            }
        }
    }).detach();
}

} // namespace satellite
