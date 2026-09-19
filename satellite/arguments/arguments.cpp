#include "arguments.hpp"

#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"
#include "../config/satellite_config.hpp"

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
        if (found->second < facts_start_ && overwritten_.empty())
            overwritten_ = name; // a config row that gather() would replace
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

void Arguments::add_number(const std::string &name, satellite_number value)
{
    add(name, ArgumentKind::number).number = std::move(value);
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

const satellite_number &Arguments::number(const std::string &name) const
{
    static const satellite_number zero;
    const Argument *entry = find(name);
    return entry != nullptr && entry->kind == ArgumentKind::number ? entry->number : zero;
}

std::string Arguments::text(const std::string &name) const
{
    const Argument *entry = find(name);
    return entry != nullptr && entry->kind == ArgumentKind::text ? entry->text : std::string();
}

// The author's rows (satellite_config.hpp): {name, number, flag, is_flag}. A row
// whose is_flag is true is a flag holding `flag`; any other row is a number
// holding `number`.
signed long long int Arguments::gather_config()
{
    const auto refuse = [](const std::string &why) {
        return report_error("satellite_config.hpp: " + why, config_value_not_understood);
    };

    for (const satellite_argument_row &row : return_arguments_vector()) {
        bool named = row.name.rfind("arguments.", 0) == 0 && row.name.size() > 10 && row.name.back() != '.' &&
                     row.name.find("..") == std::string::npos;
        for (const char character : row.name)
            if (!((character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
                  (character >= '0' && character <= '9') || character == '_' || character == '.'))
                named = false;
        if (!named)
            return refuse("\"" + row.name +
                          "\" is not a name arguments can hold (arguments. then words of a-z A-Z 0-9 _ joined by .)");
        if (find(row.name) != nullptr)
            return refuse(row.name + " is written twice");
        // REFUSED BY NAME, NOT BY WHAT THIS RUN FILLED IN (review of M0.5): a row
        // named arguments.argument_3 was a config value on a run given two words
        // and a refusal on a run given three.
        if (filled_in_by_satl(row.name))
            return refuse(row.name + " is filled in by satl itself (the command line or the machine), so it cannot be a row");
        if (row.is_flag) {
            add_flag(row.name, row.flag);
            continue;
        }
        // THE DIGITS BECOME A satellite_number HERE, and nowhere earlier: the row
        // kept them as text, so a number of any length arrives whole.
        satellite_number value;
        std::size_t bad_offset = 0;
        if (satellite_number::from_text(row.number.digits, value, bad_offset) != success)
            return refuse(row.name + " is written \"" + row.number.digits +
                          "\", which is not a whole number (digits only, in quotes when there are many)");
        add_number(row.name, std::move(value));
    }

    // Shown every time satl starts unless the config says false (the author, 2026-09-15).
    const Argument *startup_display = find("arguments.startup_display");
    if (startup_display == nullptr)
        add_flag("arguments.startup_display", true);
    else if (startup_display->kind != ArgumentKind::flag)
        return refuse("arguments.startup_display is a bool row (true, true or false, true)");

    // The three numbers the title lines show, and the start-up threads.
    for (const char *name : {"arguments.version", "arguments.revision", "arguments.build",
                             "arguments.threads_startup"}) {
        const Argument *entry = find(name);
        if (entry == nullptr || entry->kind != ArgumentKind::number)
            return refuse(std::string(name) + " needs a number row");
        if (entry->number.negative())
            return refuse(std::string(name) + " cannot be negative (" + entry->number.to_text() + ")");
    }
    const Argument *threads_max = find("arguments.threads_max");
    if (threads_max != nullptr && (threads_max->kind != ArgumentKind::number || threads_max->number.negative()))
        return refuse("arguments.threads_max is a number row that is not negative");

    // INFINITY'S TWO DIGIT COUNTS (the author, 2026-09-16): 128 held (4096 until
    // 2026-09-18), 32 shown, "both digits configurable". A missing row takes the
    // author's default, as arguments.startup_display does; a row that is there must
    // be a count of at least one digit, because a multiplier with no digits cannot
    // hold x2.
    for (const auto &[name, fallback] : {std::pair<const char *, unsigned long long int>{"arguments.infinity", 128},
                                         std::pair<const char *, unsigned long long int>{"arguments.infinity_display", 32}}) {
        const Argument *entry = find(name);
        if (entry == nullptr) {
            add_number(name, satellite_number(fallback));
            continue;
        }
        if (entry->kind != ArgumentKind::number || entry->number.negative() || entry->number.is_zero())
            return refuse(std::string(name) + " is a number row of at least 1 digit");
    }

    // THE INFINITY COUNTER (the author, 2026-09-18): how many calculations one
    // infinity-family object may take without reaching the next type before satl
    // prints the SATELLITE INFINITY WARNING and counts again. The same rule as the two
    // above: a missing row takes the author's 999,999,999, and a row that is there
    // counts at least one calculation.
    const Argument *counter = find("arguments.infinity.counter");
    if (counter == nullptr)
        add_number("arguments.infinity.counter", satellite_number(999999999ull));
    else if (counter->kind != ArgumentKind::number || counter->number.negative() || counter->number.is_zero())
        return refuse("arguments.infinity.counter is a number row of at least 1 calculation");
    return success;
}

signed long long int Arguments::gather(const CommandLine &command_line)
{
    // Everything added from here on is satl's own: the command line and the
    // machine. A config row with one of those names is refused, never replaced
    // behind the author's back (review 2026-09-15).
    facts_start_ = entries_.size();

    // THE COMMAND LINE (PLAN M0.5), with 003's names: the file is
    // arguments.program, every word after it is arguments.argument_1 onwards in
    // the order it was typed, and arguments.length counts them all, the program
    // included -- so `satl prog.satl a b` is 3. arguments.file is the same file
    // under the name the interpreter reads it by.
    add_flag("arguments.debug_mode", command_line.debug);
    add_text("arguments.file", command_line.file);
    add_text("arguments.program", command_line.file);
    for (std::size_t i = 0; i < command_line.words.size(); i++)
        add_text("arguments.argument_" + std::to_string(i + 1), command_line.words[i]);
    add_count("arguments.length", command_line.words.size() + 1);

    // THE DIRECTORY SATL STARTED IN, read once. M0.6's satellite.directory.change
    // never moves it (PLAN M0.5). Any length: getcwd(nullptr, 0) sizes its own
    // buffer. Empty when the machine cannot say -- a directory deleted under the
    // shell -- and the program still runs.
    if (char *directory = getcwd(nullptr, 0)) {
        add_text("arguments.session.directory", directory);
        std::free(directory);
    } else {
        add_text("arguments.session.directory", "");
    }

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
    if (!overwritten_.empty())
        return report_error("satellite_config.hpp: " + overwritten_ +
                                " is filled in by satl itself (the command line or the machine), so it cannot be a row",
                            config_value_not_understood);
    return success;
}

bool filled_in_by_satl(const std::string &name)
{
    // Every name gather() can add, including the ones it adds only when the
    // machine answers (statvfs, uname). arguments_cases.cpp checks that a real
    // gather adds nothing outside this list.
    static const char *const names[] = {
        "arguments.debug_mode", "arguments.file", "arguments.program", "arguments.length",
        "arguments.session.directory", "arguments.machine.threads", "arguments.machine.cores",
        "arguments.machine.page_size", "arguments.memory.total", "arguments.disk.total", "arguments.disk.free",
        "arguments.username", "arguments.system.hostname", "arguments.system.kernel", "arguments.system.kernel_version"};
    for (const char *filled : names)
        if (name == filled)
            return true;
    const std::string numbered = "arguments.argument_";
    return name.size() > numbered.size() && name.compare(0, numbered.size(), numbered) == 0 &&
           name.find_first_not_of("0123456789", numbered.size()) == std::string::npos;
}

std::string describe(const Argument &argument)
{
    switch (argument.kind) {
    case ArgumentKind::text:
        return argument.text;
    case ArgumentKind::count:
        return std::to_string(argument.count);
    case ArgumentKind::number:
        return argument.number.to_text();
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
