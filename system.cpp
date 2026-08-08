#include "system.hpp"
#include "library.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

#include <pwd.h>
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
