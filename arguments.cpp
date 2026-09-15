#include "arguments.hpp"

#include "machine_codes.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <utility>

#include <pwd.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>

namespace satellite004 {

namespace {

// Physical cores: the distinct (physical id, core id) pairs in /proc/cpuinfo.
// 0 when the file does not say, and the caller falls back to threads.
unsigned long long int physical_cores()
{
    std::FILE *cpuinfo = std::fopen("/proc/cpuinfo", "r");
    if (cpuinfo == nullptr)
        return 0;
    std::set<std::pair<long, long>> cores;
    long physical = -1;
    char line[512];
    while (std::fgets(line, sizeof line, cpuinfo) != nullptr) {
        const char *colon = std::strchr(line, ':');
        if (colon == nullptr)
            continue;
        if (std::strncmp(line, "physical id", 11) == 0)
            physical = std::strtol(colon + 1, nullptr, 10);
        else if (std::strncmp(line, "core id", 7) == 0)
            cores.insert({physical, std::strtol(colon + 1, nullptr, 10)});
    }
    std::fclose(cpuinfo);
    return cores.size();
}

} // namespace

Argument &Arguments::add(const std::string &name, ArgumentKind kind)
{
    const auto found = where_.find(name);
    if (found != where_.end()) {
        Argument &existing = entries_[found->second];
        existing = Argument{};
        existing.name = name;
        existing.kind = kind;
        return existing;
    }
    where_[name] = entries_.size();
    entries_.push_back(Argument{});
    entries_.back().name = name;
    entries_.back().kind = kind;
    return entries_.back();
}

void Arguments::add_text(const std::string &name, const std::string &value)
{
    add(name, ArgumentKind::text).text = value;
}

void Arguments::add_count(const std::string &name, unsigned long long int value)
{
    add(name, ArgumentKind::count).count = value;
}

void Arguments::add_flag(const std::string &name, bool value)
{
    add(name, ArgumentKind::flag).flag = value;
}

// The largest unit the amount reaches at least 1 of, up to terabytes.
void Arguments::add_bytes(const std::string &name, unsigned long long int bytes)
{
    static const char *const units[] = {"bytes", "kilobytes", "megabytes", "gigabytes", "terabytes"};
    long double value = static_cast<long double>(bytes);
    int unit = 0;
    while (value >= 1024.0L && unit < 4) {
        value /= 1024.0L;
        unit++;
    }
    Argument &entry = add(name, ArgumentKind::size);
    entry.size = value;
    entry.unit = units[unit];
}

const Argument *Arguments::find(const std::string &name) const
{
    const auto found = where_.find(name);
    return found == where_.end() ? nullptr : &entries_[found->second];
}

bool Arguments::flag(const std::string &name) const
{
    const Argument *entry = find(name);
    return entry != nullptr && entry->kind == ArgumentKind::flag && entry->flag;
}

std::string Arguments::text(const std::string &name) const
{
    const Argument *entry = find(name);
    return entry != nullptr && entry->kind == ArgumentKind::text ? entry->text : std::string();
}

signed long long int Arguments::gather(int argc, char **argv)
{
    // The command line: --debug, and the .satl file to run.
    bool debug_mode = false;
    std::string file;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--debug") == 0)
            debug_mode = true;
        else if (file.empty())
            file = argv[i];
    }
    add_flag("arguments.debug_mode", debug_mode);
    add_text("arguments.file", file);

    // The machine.
    const long threads = sysconf(_SC_NPROCESSORS_ONLN);
    const unsigned long long int thread_count = threads > 0 ? static_cast<unsigned long long int>(threads) : 1;
    const unsigned long long int cores = physical_cores();
    add_count("arguments.machine.threads", thread_count);
    add_count("arguments.machine.cores", cores > 0 ? cores : thread_count);

    const long page = sysconf(_SC_PAGESIZE);
    const long pages = sysconf(_SC_PHYS_PAGES);
    if (page > 0)
        add_bytes("arguments.machine.page_size", static_cast<unsigned long long int>(page));
    if (page > 0 && pages > 0)
        add_bytes("arguments.memory.total",
                  static_cast<unsigned long long int>(pages) * static_cast<unsigned long long int>(page));

    struct statvfs disk;
    if (statvfs("/", &disk) == 0) {
        add_bytes("arguments.disk.total", static_cast<unsigned long long int>(disk.f_blocks) * disk.f_frsize);
        add_bytes("arguments.disk.free", static_cast<unsigned long long int>(disk.f_bavail) * disk.f_frsize);
    }

    const passwd *user = getpwuid(geteuid());
    const char *name = user != nullptr ? user->pw_name : std::getenv("USER");
    add_text("arguments.username", name != nullptr ? name : "");

    struct utsname system;
    if (uname(&system) == 0) {
        add_text("arguments.system.hostname", system.nodename);
        add_text("arguments.system.kernel", system.sysname);
        add_text("arguments.system.kernel_version", system.release);
    }
    return success;
}

std::string describe(const Argument &argument)
{
    switch (argument.kind) {
    case ArgumentKind::text:
        return argument.text;
    case ArgumentKind::count:
        return std::to_string(argument.count);
    case ArgumentKind::flag:
        return argument.flag ? "true" : "false";
    case ArgumentKind::size: {
        char digits[64];
        std::snprintf(digits, sizeof digits, "%.18Lg", argument.size);
        return std::string(digits) + " " + argument.unit;
    }
    }
    return "";
}

} // namespace satellite004
