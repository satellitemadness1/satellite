#include "system_facts/system.hpp"
#include "satellite_library/library.hpp"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include <dirent.h>

#include <pthread.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Tier 3 of library_path(), baked from the Makefile's `prefix`. The fallback
// is here so that a build with no -D at all -- the test binaries compile these
// sources directly, and so does anyone reaching for a bare clang++ -- still
// compiles and still resolves somewhere defensible.
#ifndef SATELLITE_LIB_DIR
#define SATELLITE_LIB_DIR "/usr/local/share/satellite/lib"
#endif

namespace satellite {

std::string username()
{
    if (const passwd *pw = getpwuid(getuid()); pw && pw->pw_name)
        return pw->pw_name;
    if (const char *env = getenv("USER"))
        return env;
    return "unknown";
}

std::string home_dir()
{
    if (const char *env = getenv("HOME"))
        return env;
    if (const passwd *pw = getpwuid(getuid()); pw && pw->pw_dir)
        return pw->pw_dir;
    return "/";
}

std::string cwd()
{
    char buf[4096];
    if (getcwd(buf, sizeof buf))
        return buf;
    return "?";
}

unsigned hardware_threads()
{
    unsigned n = std::thread::hardware_concurrency();
    return n ? n : 1;
}

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

bool thread_stack_bytes(unsigned long long *used, unsigned long long *total)
{
    pthread_attr_t attr;
    if (pthread_getattr_np(pthread_self(), &attr) != 0)
        return false;
    void *base = nullptr;
    size_t size = 0;
    const int got = pthread_attr_getstack(&attr, &base, &size);
    pthread_attr_destroy(&attr);
    if (got != 0 || !base || size == 0)
        return false;

    // The stack grows DOWN on every platform this runs on, so what is in use
    // is the distance from the top of the region to where we are standing. A
    // local's address is the closest thing to a stack pointer that is legal to
    // ask for in C++, and it is a frame or two low -- which is a handful of
    // bytes against a figure reported in megabytes.
    char here = 0;
    const char *top = static_cast<char *>(base) + size;
    const unsigned long long depth =
        static_cast<unsigned long long>(top - &here);
    if (used)
        *used = depth > size ? size : depth;
    if (total)
        *total = static_cast<unsigned long long>(size);
    return true;
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

unsigned long long stack_limit_bytes()
{
    struct rlimit limit;
    if (getrlimit(RLIMIT_STACK, &limit) != 0)
        return STACK_LIMIT_UNKNOWN;
    if (limit.rlim_cur == RLIM_INFINITY)
        return STACK_LIMIT_UNKNOWN;
    return static_cast<unsigned long long>(limit.rlim_cur);
}

unsigned long mem_used_mb()
{
    unsigned long total = meminfo_kb("MemTotal");
    unsigned long avail = meminfo_kb("MemAvailable");
    return (total > avail ? total - avail : 0) / 1024;
}

static bool is_directory(const std::string &path)
{
    struct stat info;
    return !path.empty() && stat(path.c_str(), &info) == 0 &&
           S_ISDIR(info.st_mode);
}

// The install root the running binary sits under: two components up from
// /proc/self/exe, so /usr/local/bin/satl answers /usr/local.
//
// Two components rather than a match on the literal "/bin/satl", because
// satl-term is the other binary in that directory and it must reach the
// same answer.
static std::string exe_prefix()
{
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof buf);
    // A readlink that exactly fills the buffer may have been truncated, and
    // there is no way to tell which -- so it is refused. A truncated path is
    // worse than no path: it would name a directory nobody meant.
    if (n <= 0 || static_cast<size_t>(n) >= sizeof buf)
        return {};

    std::string path(buf, static_cast<size_t>(n));
    for (int up = 0; up < 2; ++up) {
        size_t slash = path.rfind('/');
        if (slash == std::string::npos)
            return {};
        path.resize(slash);
    }
    return path;
}

std::string library_path(LibraryPathSource *source)
{
    auto answer = [source](LibraryPathSource found, std::string path) {
        if (source)
            *source = found;
        return path;
    };

    if (const char *env = getenv("SATELLITE_PATH"); env && *env)
        if (is_directory(env))
            return answer(LibraryPathSource::Environment, env);

    if (std::string prefix = exe_prefix(); !prefix.empty()) {
        std::string relative = prefix + "/share/satellite/lib";
        if (is_directory(relative))
            return answer(LibraryPathSource::Relative, std::move(relative));
    }

    if (is_directory(SATELLITE_LIB_DIR))
        return answer(LibraryPathSource::Compiled, SATELLITE_LIB_DIR);

    return answer(LibraryPathSource::None, {});
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
