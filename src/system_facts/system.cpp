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
#include <sys/utsname.h>

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

// The build facts the arguments object reports. Every one has a fallback for
// the same reason SATELLITE_LIB_DIR does: the test binaries compile these
// sources directly with no -D at all, and so does anyone reaching for a bare
// clang++. `unrecorded` is a truthful answer and an empty string is not.
#ifndef SATELLITE_BUILD_CXX
#define SATELLITE_BUILD_CXX "unrecorded"
#endif
#ifndef SATELLITE_BUILD_FLAGS
#define SATELLITE_BUILD_FLAGS "unrecorded"
#endif
#ifndef SATELLITE_BUILD_MAKE
#define SATELLITE_BUILD_MAKE "unrecorded"
#endif
#ifndef SATELLITE_BUILT
#define SATELLITE_BUILT "unrecorded"
#endif
#ifndef SATELLITE_VERSION
#define SATELLITE_VERSION "002"
#endif
#ifndef SATELLITE_REVISION
#define SATELLITE_REVISION "01"
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


// ---------------------------------------------------------------------------
// The arguments object — see system.hpp for what it promises.
// ---------------------------------------------------------------------------

namespace {

// The two words that stand in for a fact this machine will not give up. They
// are distinct on purpose: `unrecorded` means the build never passed it,
// `unavailable` means the running system declined to answer. Debugging a
// report that says one is a different job from debugging one that says the
// other.
constexpr const char *UNRECORDED  = "unrecorded";
constexpr const char *UNAVAILABLE = "unavailable";

// $NAME, or "" if unset or empty. Empty rather than the fallback word, because
// every caller here wants to try a second source before giving up.
std::string env_or_empty(const char *name)
{
    const char *v = getenv(name);
    return (v && *v) ? std::string(v) : std::string();
}

// One VAR=value out of /etc/os-release, with the surrounding quotes taken off.
//
// Parsed here rather than shelled out to, because a program that forks
// `sh -c ". /etc/os-release"` to learn its own distribution is a program that
// fails inside a container with no shell. The format is one KEY=VALUE per
// line, optionally quoted, and that is all this reads.
std::string os_release(const char *key)
{
    FILE *f = fopen("/etc/os-release", "re");
    if (!f)
        return {};

    std::string want = std::string(key) + "=";
    char line[1024];
    std::string found;
    while (fgets(line, sizeof line, f)) {
        std::string text(line);
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
            text.pop_back();
        if (text.compare(0, want.size(), want) != 0)
            continue;
        found = text.substr(want.size());
        if (found.size() >= 2 && (found.front() == '"' || found.front() == '\'') &&
            found.back() == found.front())
            found = found.substr(1, found.size() - 2);
        break;
    }
    fclose(f);
    return found;
}

// The C++ standard the interpreter was compiled to, as a year rather than as
// __cplusplus's raw 202002L, because the year is what anyone reading it knows.
const char *cxx_standard_name()
{
#if __cplusplus >= 202302L
    return "C++23";
#elif __cplusplus >= 202002L
    return "C++20";
#elif __cplusplus >= 201703L
    return "C++17";
#else
    return "pre-C++17";
#endif
}

// Which standard library, and which release of it. Both libstdc++ and libc++
// are supported targets — the Makefile builds with clang against libstdc++ —
// so neither may be assumed.
std::string standard_library_name()
{
#if defined(_GLIBCXX_RELEASE)
    return "libstdc++ " + std::to_string(_GLIBCXX_RELEASE);
#elif defined(_LIBCPP_VERSION)
    return "libc++ " + std::to_string(_LIBCPP_VERSION);
#else
    return UNRECORDED;
#endif
}

std::string c_library_name()
{
#if defined(__GLIBC__)
    return "glibc " + std::to_string(__GLIBC__) + "." +
           std::to_string(__GLIBC_MINOR__);
#else
    return UNRECORDED;
#endif
}

// Append one entry, keeping `index` in step. Called only while the body is
// still being built and before make_arguments freezes it.
//
// A blank value becomes the fallback word here, in ONE place, rather than at
// each of the thirty call sites — which is what stops an entry from quietly
// being "" on a machine where the fact was missing.
void add(Arguments &args, std::string name, std::string value,
         const char *missing = UNAVAILABLE)
{
    if (value.empty())
        value = missing;
    args.index[name] = args.entries.size();
    args.entries.push_back(
        ArgumentEntry{std::move(name),
                      std::make_shared<const Value>(make_string(encode_raw(value)))});
}

} // namespace

Arguments arguments_for(const List &command_line)
{
    Arguments args;

    // --- the command line, first and in order -------------------------------
    // argv[0] is the script (interp.hpp), so it gets the name a shell would
    // call it: the program. Everything after it is numbered from 1, matching
    // the index the program reads it at, so argument_1 IS args[1].
    for (size_t i = 0; i < command_line.size(); i++) {
        std::string name = i == 0 ? std::string("program")
                                  : "argument_" + std::to_string(i);
        args.index[name] = args.entries.size();
        // The caller's handle, not a copy: these Values already exist and are
        // immutable, which is the whole reason a ValuePtr may be shared.
        args.entries.push_back(ArgumentEntry{std::move(name), command_line[i]});
    }
    args.command_line_count = args.entries.size();

    // Deliberately AFTER command_line_count is fixed, so it is a named entry
    // and not a positional one — it describes the command line, it is not part
    // of it, and counting itself would be wrong by one.
    add(args, "argument_count", std::to_string(command_line.size()));

    // --- where the program is -----------------------------------------------
    {
        char buf[4096];
        add(args, "current_directory",
            getcwd(buf, sizeof buf) ? std::string(buf) : std::string());
    }

    std::string home = env_or_empty("HOME");
    std::string user = env_or_empty("USER");
    if (home.empty() || user.empty()) {
        // $HOME and $USER are both forgeable and both absent under a bare
        // `env -i`, so the password database is the second source. It is the
        // authority for the name; $USER is merely what the shell was told.
        if (const struct passwd *pw = getpwuid(getuid())) {
            if (home.empty() && pw->pw_dir)
                home = pw->pw_dir;
            if (user.empty() && pw->pw_name)
                user = pw->pw_name;
        }
    }
    add(args, "home_directory", home);
    add(args, "username", user);

    {
        char buf[256];
        add(args, "hostname",
            gethostname(buf, sizeof buf) == 0 ? std::string(buf) : std::string());
    }

    // --- the operating system, one fact per entry ---------------------------
    // Separate entries rather than one "os info" string, because a program
    // that wants the architecture should not have to parse a sentence.
    {
        struct utsname u;
        bool ok = uname(&u) == 0;
        add(args, "operating_system", ok ? u.sysname : "");
        add(args, "kernel_release",   ok ? u.release : "");
        add(args, "kernel_version",   ok ? u.version : "");
        add(args, "architecture",     ok ? u.machine : "");
    }
    add(args, "distribution",         os_release("PRETTY_NAME"));
    add(args, "distribution_id",      os_release("ID"));
    add(args, "distribution_version", os_release("VERSION_ID"));

    // --- what built the interpreter -----------------------------------------
    // Baked in at compile time, so these describe the binary that is running
    // and not whatever compiler happens to be installed now. That distinction
    // is the reason they are worth carrying at all.
    add(args, "cxx_compiler",         SATELLITE_BUILD_CXX,  UNRECORDED);
    add(args, "cxx_compiler_version",
#ifdef __VERSION__
        __VERSION__,
#else
        "",
#endif
        UNRECORDED);
    add(args, "cxx_standard",         cxx_standard_name(),  UNRECORDED);
    add(args, "cxx_flags",            SATELLITE_BUILD_FLAGS, UNRECORDED);
    add(args, "make_version",         SATELLITE_BUILD_MAKE, UNRECORDED);
    add(args, "standard_library",     standard_library_name(), UNRECORDED);
    add(args, "c_library",            c_library_name(),     UNRECORDED);
    add(args, "built",                SATELLITE_BUILT,      UNRECORDED);

    // --- the interpreter itself ---------------------------------------------
    {
        char buf[4096];
        ssize_t n = readlink("/proc/self/exe", buf, sizeof buf);
        // The same truncation refusal exe_prefix() makes above: a path that
        // exactly fills the buffer may have been cut, and a truncated path
        // names a file nobody meant.
        add(args, "interpreter",
            (n > 0 && static_cast<size_t>(n) < sizeof buf)
                ? std::string(buf, static_cast<size_t>(n))
                : std::string());
    }
    add(args, "interpreter_version",
        std::string(SATELLITE_VERSION) + " revision " + SATELLITE_REVISION);

    {
        LibraryPathSource source = LibraryPathSource::None;
        std::string path = library_path(&source);
        add(args, "library_path", path);
        // Which of the three tiers answered — the same fact `--where` prints,
        // because "it found a library" and "it found the one you installed"
        // are different answers and only one of them is reassuring.
        const char *how = "none";
        switch (source) {
        case LibraryPathSource::Environment: how = "SATELLITE_PATH"; break;
        case LibraryPathSource::Relative:    how = "alongside the binary"; break;
        case LibraryPathSource::Compiled:    how = "compiled in"; break;
        case LibraryPathSource::None:        how = "none"; break;
        }
        add(args, "library_path_source", how);
    }

    // --- the process --------------------------------------------------------
    add(args, "process_id",        std::to_string(getpid()));
    add(args, "parent_process_id", std::to_string(getppid()));
    {
        unsigned n = std::thread::hardware_concurrency();
        add(args, "thread_count", n ? std::to_string(n) : std::string());
    }
    {
        long page = sysconf(_SC_PAGESIZE);
        add(args, "page_size", page > 0 ? std::to_string(page) : std::string());
    }
    add(args, "pointer_bits", std::to_string(sizeof(void *) * 8));
    // Answered from the compiler's own macro where there is one, and by
    // inspecting a value where there is not, so this is right on a target the
    // macro does not cover rather than merely usually right.
    {
        const uint16_t probe = 1;
        bool little = *reinterpret_cast<const unsigned char *>(&probe) == 1;
        add(args, "byte_order", little ? "little" : "big");
    }

    // --- the environment it was started in ----------------------------------
    // Four named variables and no mechanism for a fifth. Reading the
    // environment in general is satellite.system.environment(name)'s job and
    // it does not exist yet; these three are here because they describe the
    // session rather than the program's own configuration.
    add(args, "shell",    env_or_empty("SHELL"));
    add(args, "terminal", env_or_empty("TERM"));
    add(args, "language", env_or_empty("LANG"));

    return args;
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
