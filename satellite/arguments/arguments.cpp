#include "arguments.hpp"
#include "argument_settings.hpp"
#include "cpu_facts.hpp"

#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"
#include "../config/config_file.hpp"
#include "../config/satellite_config.hpp"
#include "../../satellite-numbers/machine_facts.hpp"

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

void Arguments::add_list(const std::string &name, std::vector<std::string> items)
{
    add(name, ArgumentKind::list).items = std::move(items);
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
    entry.count = bytes;   // the exact figure, for the arguments variable (main_arguments.cpp)
}

const Argument *Arguments::find(const std::string &name) const
{
    const auto found = where_.find(name);
    return found == where_.end() ? nullptr : &entries_[found->second];
}

bool Arguments::flag(const std::string &name) const
{
    // UNDER THE LOCK a line takes to change a row (set_for_this_run): arguments.missing is read where a
    // MISSING code is raised, which can be while a program runs.
    const std::lock_guard<std::mutex> held(changing_);
    const Argument *entry = find(name);
    return entry != nullptr && entry->kind == ArgumentKind::flag && entry->flag;
}

bool Arguments::the_program_sets(const std::string &name, bool value)
{
    // IN PLACE, AND NOT THROUGH add(): add() takes a second value for a config row to be gather()
    // replacing it, which this is not -- the row is the author's, and the program is answering it.
    Argument row;
    row.name = name;
    row.kind = ArgumentKind::flag;
    row.flag = value;
    return set_for_this_run(row);
}

bool Arguments::set_for_this_run(const Argument &row)
{
    const std::lock_guard<std::mutex> held(changing_);
    const auto found = where_.find(row.name);
    if (found == where_.end() || entries_[found->second].kind != row.kind)
        return false;
    entries_[found->second] = row;
    return true;
}

bool Arguments::copy_of(const std::string &name, Argument &out) const
{
    const std::lock_guard<std::mutex> held(changing_);
    const auto found = where_.find(name);
    if (found == where_.end())
        return false;
    out = entries_[found->second];
    return true;
}

std::vector<Argument> Arguments::every_row_now() const
{
    const std::lock_guard<std::mutex> held(changing_);
    std::vector<Argument> rows;
    for (const Argument *row : every_row_arguments_first())
        rows.push_back(*row);
    return rows;
}

const satellite_number &Arguments::number(const std::string &name) const
{
    static const satellite_number zero;
    const Argument *entry = find(name);
    return entry != nullptr && entry->kind == ArgumentKind::number ? entry->number : zero;
}

std::string Arguments::text(const std::string &name) const
{
    const std::lock_guard<std::mutex> held(changing_);
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
        // A ROW OF WORDS (2026-09-22), and the one kind config.ini may set for
        // this machine, under its name without `arguments.`. Read here, once,
        // with the row; an empty value keeps the row's own, because "set to
        // nothing" is not a folder or a word anybody meant.
        if (row.is_text) {
            std::string value = row.text, said;
            if (config_file::read_value(row.name.substr(10), said) && !said.empty())
                value = said;
            add_text(row.name, value);
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

    // Shown every time satl starts unless the config says false (the author, 2026-09-15); and
    // what a program is missing is said unless the program says false (MS-2, 2026-10-03). A
    // missing row takes true, the author's default for both.
    for (const char *name : {"arguments.startup_display", "arguments.missing"}) {
        const Argument *entry = find(name);
        if (entry == nullptr)
            add_flag(name, true);
        else if (entry->kind != ArgumentKind::flag)
            return refuse(std::string(name) + " is a bool row (true, true or false, true)");
    }

    // The three numbers the title lines show, and how many start-up threads a core gets.
    for (const char *name : {"arguments.version", "arguments.revision", "arguments.build",
                             "arguments.threads_startup_per_core"}) {
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
    // be a count of at least one digit. arguments.infinity is the nines width every
    // satellite.infinity() is made with (INF-2) and the most places a count may have
    // (INF-3): with none, .nines() would have no place for a nine and `inf * 50%` none
    // for its 5 (SATELLITE_INFINITY.md Part 9).
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

    // THE FLOAT'S TWO PRECISIONS (the author, 2026-09-22: "arguments.float.whole(4096)
    // and arguments.float.decimal(4096) ... I think we should have the different
    // values thing"). The same rule as the infinity's two: a missing row takes its
    // default, and a row that is there is a count of at least one digit.
    //
    // AND ONE MACHINE MAY SET EITHER, in ~/.satl/config.ini by its name without
    // `arguments.` -- `float.decimal = 10` -- the way directory.default is set. A
    // precision is a thing a person tunes for the machine they run on, which is what
    // that file is for. What it says must be a count too, and is REFUSED rather than
    // quietly replaced by the default when it is not: a person who asked for 10
    // places and silently got 128 would not know to look.
    for (const auto &[name, fallback] : {std::pair<const char *, unsigned long long int>{"arguments.float.whole", 4096},
                                         std::pair<const char *, unsigned long long int>{"arguments.float.decimal", 128}}) {
        const Argument *entry = find(name);
        if (entry == nullptr)
            add_number(name, satellite_number(fallback));
        else if (entry->kind != ArgumentKind::number || entry->number.negative() || entry->number.is_zero())
            return refuse(std::string(name) + " is a number row of at least 1 digit");
        std::string said;
        const std::string key = std::string(name).substr(10);
        if (config_file::read_value(key, said) && !said.empty()) {
            satellite_number value;
            std::size_t bad_offset = 0;
            if (satellite_number::from_text(said, value, bad_offset) != success || value.negative() || value.is_zero())
                return report_error(config_file::path() + ": " + key + " = " + said +
                                        " is not a precision -- write a whole number of at least 1 digit, "
                                        "such as " + key + " = " + std::to_string(fallback),
                                    config_value_not_understood);
            add_number(name, std::move(value));
        }
    }

    // THE INFINITY COUNTER (the author, 2026-09-18): how many calculations one
    // infinity-family object may take without reaching the next type before satl
    // prints the SATELLITE INFINITY WARNING and counts again. The same rule as the two
    // above: a missing row takes the author's 999,999,999, and a row that is there
    // counts at least one calculation.
    // THE DISPLAY BUFFER (the author, 2026-09-26): 131,072 displays may wait for the console. A
    // missing row takes that; a row, or config.ini's `display.buffer = ...` for this machine, is a
    // count of at least 1, and one that is not is refused rather than quietly replaced -- the
    // float precisions' rule, above.
    {
        const Argument *buffer = find("arguments.display.buffer");
        if (buffer == nullptr)
            add_number("arguments.display.buffer", satellite_number(131072ull));
        else if (buffer->kind != ArgumentKind::number || buffer->number.negative() || buffer->number.is_zero())
            return refuse("arguments.display.buffer is a number row of at least 1 display");
        std::string said;
        if (config_file::read_value("display.buffer", said) && !said.empty()) {
            satellite_number value;
            std::size_t bad_offset = 0;
            if (satellite_number::from_text(said, value, bad_offset) != success || value.negative() || value.is_zero())
                return report_error(config_file::path() + ": display.buffer = " + said +
                                        " is not a count of displays -- write a whole number of at least 1, "
                                        "such as display.buffer = 131072",
                                    config_value_not_understood);
            add_number("arguments.display.buffer", std::move(value));
        }
    }

    const Argument *counter = find("arguments.infinity.counter");
    if (counter == nullptr)
        add_number("arguments.infinity.counter", satellite_number(999999999ull));
    else if (counter->kind != ArgumentKind::number || counter->number.negative() || counter->number.is_zero())
        return refuse("arguments.infinity.counter is a number row of at least 1 calculation");

    // EVERY OTHER SETTING, READ BACK OUT OF config.ini (the author, 2026-10-06: a setting typed at the
    // prompt "permanently changes the value"; 2026-10-04: "the entire arguments variable should be
    // saved into config.ini"), each under its name without `arguments.` (argument_settings.hpp). The
    // two text rows, the float's precisions and the display buffer are read above in their own words;
    // this is the rest, by the same rule: a number that is not one, or is under its row's least, is
    // refused rather than quietly replaced by the default -- a person who saved 10 and silently got
    // 128 would not know to look -- and a true/false row reads anything but `false` as true, as
    // config_file::read_flag always has.
    //
    // access WAS A LIBRARY'S ONLY (argument_words.hpp), read out of config.ini wherever it was asked
    // for. It is a row of the run's now, read here once with the rest, so that a file's line can
    // change it for that run alone.
    for (const Setting &setting : kSettings) {
        const std::string key = setting.key;
        const std::string name = "arguments." + key;
        if (setting.kind == SettingKind::flag && find(name) == nullptr)
            // scroll.horizontal is the one flag OFF unless written (2026-10-07): wide lines wrap
            // and the console grows downward, which is what "scroll vertically" asked for.
            add_flag(name, key == "access" ? kAccessOnByDefault : key != "scroll.horizontal");
        std::string said;
        if (!config_file::read_value(key, said) || said.empty())
            continue;
        if (setting.kind == SettingKind::flag) {
            add_flag(name, said != "false");
            continue;
        }
        const Argument *row = find(name);
        if (setting.kind == SettingKind::text || key == "float.whole" || key == "float.decimal" ||
            key == "display.buffer" || row == nullptr || row->kind != ArgumentKind::number)
            continue;   // read above -- or a row this satl's satellite_config.hpp does not have
        satellite_number value;
        std::size_t bad_offset = 0;
        if (satellite_number::from_text(said, value, bad_offset) != success || value.negative() ||
            satellite_number::compare(value, satellite_number(setting.least)) < 0)
            return report_error(config_file::path() + ": " + key + " = " + said + " is not what arguments." + key +
                                    " holds -- write a whole number" +
                                    (setting.least > 0 ? " of at least " + std::to_string(setting.least) : "") +
                                    ", such as " + key + " = " + row->number.to_text(),
                                config_value_not_understood);
        add_number(name, std::move(value));
    }
    return success;
}

signed long long int Arguments::gather(const CommandLine &command_line)
{
    // Everything added from here on is satl's own: the command line and the
    // machine. A config row with one of those names is refused, never replaced
    // behind the author's back (review 2026-09-15).
    facts_start_ = entries_.size();

    // THE COMMAND LINE, AS IT WAS TYPED (MS-1, the author 2026-10-03: "Let's just put argv as
    // argument1, then argument2 will be filename.satl, then argument3 will be --help, so it's
    // kept in order"). argument1 is satl itself, as the system started it; argument2 the file;
    // argument3 onwards every word after the file, in order. NO UNDERSCORE, and arg3, args3
    // and arguments3 are the same row (the_argument_row_spelled). arguments.length counts every
    // one of them, satl included -- `satl prog.satl a b` is 4, where it was 3 before argv was
    // counted. satl's own words BEFORE the file (--debug, --console) are satl's, and in none of
    // them. arguments.program and arguments.file are the file again, under the names the
    // interpreter reads it by.
    add_flag("arguments.debug_mode", command_line.debug);
    add_text("arguments.file", command_line.file);
    add_text("arguments.program", command_line.file);
    std::vector<std::string> typed;
    typed.push_back(command_line.satl.empty() ? std::string("satl") : command_line.satl);
    // THE FILE WHENEVER ONE IS RUN, even one spelled "" -- asked by whether a file is RUN and
    // not by whether its name is empty, or `satl "" a b` would number a as argument2 (a fresh
    // reader, 2026-10-03). The prompt and the opening lines run no file, and have argument1 only.
    if (command_line.command == Command::run)
        typed.push_back(command_line.file);
    typed.insert(typed.end(), command_line.words.begin(), command_line.words.end());
    for (std::size_t i = 0; i < typed.size(); i++)
        add_text("arguments.argument" + std::to_string(i + 1), typed[i]);
    add_count("arguments.length", typed.size());

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
    // The arguments words' own reader, which answers the count the first start kept in config.ini
    // and reads /proc/cpuinfo only when none was kept (machine_facts.hpp).
    const unsigned long long int cores = machine_facts::cores_of_this_machine();
    add_count("arguments.machine.threads", thread_count);
    add_count("arguments.machine.cores", cores > 0 ? cores : thread_count);
    // HOW MANY WARM THREADS START: THOSE CORES TIMES arguments.threads_startup_per_core (the
    // author, 2026-10-03: "how many cores the machine has x2 ... so 24 threads on this
    // machine"), and a fixed 1024 before that. A NUMBER row, not a count, because run_satl and
    // the prompt read it with number(), which answers 0 for a count -- and 0 warm threads would
    // quietly convert every line on main.
    add_number("arguments.threads_startup", satellite_number(cores > 0 ? cores : thread_count) *
                                                number("arguments.threads_startup_per_core"));
    // WHAT THE PROCESSOR CAN RUN (cpu_facts.hpp): 003's build name, and every instruction
    // set the processor and the kernel both allow, in one list.
    add_text("arguments.cpu.architecture", cpu_architecture());
    add_list("arguments.cpu.features", cpu_features());

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

namespace {

// THE NUMBER AFTER ONE OF THE FOUR SPELLINGS of a command-line row, without its leading
// zeros -- "3" for arg3, args03, argument3 and arguments3, "0" for arg0 -- or "" when the key
// is none of them. DIGITS AND NOT A COUNT, so arguments99999999999999999999 is read without
// overflowing anything. LONGEST FIRST: arguments3 also begins with argument and with arg.
std::string number_after_a_spelling(const std::string &key)
{
    for (const char *spelling : {"arguments", "argument", "args", "arg"}) {
        const std::size_t length = std::strlen(spelling);
        if (key.size() <= length || key.compare(0, length, spelling) != 0)
            continue;
        if (key.find_first_not_of("0123456789", length) != std::string::npos)
            return std::string();
        const std::size_t first = key.find_first_not_of('0', length);
        return first == std::string::npos ? std::string("0") : key.substr(first);
    }
    return std::string();
}

// argument_ AND DIGITS, the spelling until 2026-10-03.
bool an_old_argument_spelling(const std::string &key)
{
    static const std::string old = "argument_";
    return key.size() > old.size() && key.compare(0, old.size(), old) == 0 &&
           key.find_first_not_of("0123456789", old.size()) == std::string::npos;
}

// DIGITS PLUS TWO, as digits -- a row number of any length.
std::string plus_two(std::string digits)
{
    int carry = 2;
    for (std::size_t at = digits.size(); at-- > 0 && carry != 0;) {
        const int sum = (digits[at] - '0') + carry;
        digits[at] = static_cast<char>('0' + sum % 10);
        carry = sum / 10;
    }
    if (carry != 0)
        digits.insert(digits.begin(), static_cast<char>('0' + carry));
    return digits;
}

} // namespace

std::string the_argument_row_spelled(const std::string &key)
{
    const std::string number = number_after_a_spelling(key);
    return number.empty() || number == "0" ? key : "argument" + number;
}

bool names_argument_zero(const std::string &key)
{
    return number_after_a_spelling(key) == "0";
}

std::string the_row_argument_underscore_is_now(const std::string &key)
{
    if (!an_old_argument_spelling(key))
        return std::string();
    const std::size_t first = key.find_first_not_of('0', std::strlen("argument_"));
    if (first == std::string::npos)
        return std::string();   // argument_0 was never a row, and is nothing now either
    return "argument" + plus_two(key.substr(first));
}

bool filled_in_by_satl(const std::string &name)
{
    // Every name gather() can add, including the ones it adds only when the
    // machine answers (statvfs, uname). arguments_cases.cpp checks that a real
    // gather adds nothing outside this list.
    static const char *const names[] = {
        "arguments.debug_mode", "arguments.file", "arguments.program", "arguments.length",
        "arguments.session.directory", "arguments.machine.threads", "arguments.machine.cores",
        "arguments.threads_startup", "arguments.cpu.architecture", "arguments.cpu.features",
        "arguments.machine.page_size", "arguments.memory.total", "arguments.disk.total", "arguments.disk.free",
        "arguments.username", "arguments.system.hostname", "arguments.system.kernel", "arguments.system.kernel_version"};
    for (const char *filled : names)
        if (name == filled)
            return true;
    // THE COMMAND LINE'S ROWS, UNDER EVERY SPELLING (MS-1) -- argument3, arg3, args3,
    // arguments3, arg0 -- and the old argument_3: none of them is a name a config row or a
    // program may take, whichever of them this run happened to be given.
    const std::string prefix = "arguments.";
    if (name.size() <= prefix.size() || name.compare(0, prefix.size(), prefix) != 0)
        return false;
    const std::string key = name.substr(prefix.size());
    return !number_after_a_spelling(key).empty() || an_old_argument_spelling(key);
}

std::vector<const Argument *> Arguments::every_row_arguments_first() const
{
    // gather() ADDS THE TYPED ROWS IN ORDER, one after another, so the first pass keeps the
    // order they were typed in; the second takes every row that is neither them nor length.
    // A config row can never be named like one (filled_in_by_satl), so nothing else is skipped.
    const auto typed = [](const std::string &name) {
        static const std::string numbered = "arguments.argument";
        return name.size() > numbered.size() && name.compare(0, numbered.size(), numbered) == 0 &&
               name.find_first_not_of("0123456789", numbered.size()) == std::string::npos;
    };
    std::vector<const Argument *> rows;
    rows.reserve(entries_.size());
    for (const Argument &row : entries_)
        if (typed(row.name))
            rows.push_back(&row);
    if (const Argument *length = find("arguments.length"))
        rows.push_back(length);
    for (const Argument &row : entries_)
        if (!typed(row.name) && row.name != "arguments.length")
            rows.push_back(&row);
    return rows;
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
    case ArgumentKind::list: {
        std::string words;
        for (const std::string &item : argument.items)
            words += (words.empty() ? "" : ", ") + item;
        return words;
    }
    }
    return "";
}

} // namespace satellite004
