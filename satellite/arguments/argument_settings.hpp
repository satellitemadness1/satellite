#pragma once
// satellite/arguments/argument_settings.hpp -- THE SETTINGS: the rows of the arguments a person
// changes, and what each one is. The author, 2026-10-06:
//
//     "arguments.item(value_to_change_to) and if it's typed ON the prompt it permanently changes
//      the value, and if it's typed in a file it temporarily changes a value, and the alias syntax
//      using = ... that does the same thing, and has the same rules as changing it permanently on
//      the console, or temporarily inside of a file"
//
// and, before it, 2026-10-04: "the entire arguments variable should be saved into config.ini", and
// "use parentheses".
//
// ONE TABLE, AND EVERYTHING THAT HAS TO AGREE ABOUT THEM READS IT: start-up reads each one back out
// of config.ini (arguments.cpp), S016 knows each one's key (structured-library.cpp), and a line that
// changes one judges its value by the row's kind (bytecode/setting_writes.hpp).
//
// A FACT IS NOT HERE, and that is the line this table draws. The machine's rows (memory.total,
// machine.cores), the command line's (argument1, length) and satl's own numbers (version, revision,
// build) say what IS: a program that wrote one would be lying to itself about the machine
// (SATELLITE_ARGUMENTS.md Part 4C), so they are read and never written, as before.
//
// NOR ARE THE WARM THREADS (the fresh reader, 2026-10-06). threads_startup_per_core and threads_max
// stay the author's rows in satellite_config.hpp: saved from the prompt, one line --
// arguments.threads_startup_per_core(100000) -- would make every start after it ask for a million
// threads, and the prompt that could undo it starts them too. (A thread stress test froze this
// machine once.) They are read, and refused as rows satl holds.
//
// THE KEY IS THE ROW'S NAME AFTER `arguments.`, AND config.ini's KEY TOO: `arguments.float.decimal`
// is `float.decimal = 10` in the file, as directory.default and the float's precisions always were.

#include <string>

namespace satellite004 {

enum class SettingKind {
    flag,     // true or false -- bare, or satellite.bool.true and .false (MS-2's D22)
    number,   // a whole number, never under the row's least
    text,     // words: a folder, a path
};

// WHEN satl READS A SETTING, which decides what a line in a file can do with it.
enum class SettingRead {
    // WHILE A PROGRAM RUNS -- or by nothing yet: a file's line changes it from that line on.
    while_it_runs,
    // AS satl STARTS, before it reads any file: the start-up lines, the warm threads, the folder the
    // prompt opens in, the display buffer. A file's line comes too late to change it for its own
    // run, so it changes only for good -- typed at the prompt.
    as_satl_starts,
    // THE PROGRAM'S SWITCH (MS-2): written at the top of its file, before anything is said about the
    // program. Inside a capsule a line is too late for it -- some of what it turns off is said before
    // satellite.main begins.
    at_the_top_of_the_file,
};

struct Setting {
    const char *key;
    SettingKind kind;
    unsigned long long int least;   // a number's smallest value
    SettingRead read;
};

// access IS ON UNLESS A PERSON SAID OTHERWISE (the author, 2026-09-18: "arguments.access will always
// be on, on this machine, as we are testing it"). Here, so the row start-up makes and the library that
// answers it when there is no row (argument_words.hpp) cannot disagree.
inline constexpr bool kAccessOnByDefault = true;

inline constexpr Setting kSettings[] = {
    // keep the last known name, type and value of everything (argument_words.hpp)
    {"access", SettingKind::flag, 0, SettingRead::while_it_runs},
    // what a program is missing is said, and the tutorial is on (SCRATCH.md/MISSING_SYNTAX.md MS-2)
    {"missing", SettingKind::flag, 0, SettingRead::at_the_top_of_the_file},
    // the start-up lines satl shows before anything else
    {"startup_display", SettingKind::flag, 0, SettingRead::as_satl_starts},
    // the converting threads' step (satellite_config.hpp) -- read by nothing yet
    {"magic", SettingKind::number, 0, SettingRead::while_it_runs},
    // the folder the prompt in satl's own console starts in
    {"directory.default", SettingKind::text, 0, SettingRead::as_satl_starts},
    // where satellite.log is written
    {"log_path", SettingKind::text, 0, SettingRead::while_it_runs},
    // read by nothing yet
    {"object_bytes_max", SettingKind::number, 0, SettingRead::while_it_runs},
    {"file_size_max_bytes", SettingKind::number, 0, SettingRead::while_it_runs},
    // the nines width every satellite.infinity() is made with, read where one is made
    {"infinity", SettingKind::number, 1, SettingRead::while_it_runs},
    // how many places a float and .nines() show
    {"infinity_display", SettingKind::number, 1, SettingRead::while_it_runs},
    // read by nothing yet (SATELLITE_INFINITY.md)
    {"infinity.counter", SettingKind::number, 1, SettingRead::while_it_runs},
    // the float's two precisions
    {"float.whole", SettingKind::number, 1, SettingRead::while_it_runs},
    {"float.decimal", SettingKind::number, 1, SettingRead::while_it_runs},
    // the displays that may wait for the console -- "it has to be changed outside of program
    // execution" (the author, 2026-09-26)
    {"display.buffer", SettingKind::number, 1, SettingRead::as_satl_starts},
    // how a console scrolls (the author, 2026-10-07: "arguments.scroll.vertical = true or false so if
    // I ship this program sometime, I can have it turned off by default", and scroll.horizontal "as
    // well"): up and down is a bar and no ceiling on what it keeps; sideways is 2048 characters of
    // width that wrap nothing. Read when a console is made, and satl's own console follows a change.
    {"scroll.vertical", SettingKind::flag, 0, SettingRead::while_it_runs},
    {"scroll.horizontal", SettingKind::flag, 0, SettingRead::while_it_runs},
};

// The setting `key` names -- "float.decimal" -- or nullptr when it names none.
inline const Setting *a_setting_named(const std::string &key)
{
    for (const Setting &setting : kSettings)
        if (key == setting.key)
            return &setting;
    return nullptr;
}

// EVERY SETTING'S NAME, for a sentence that lists them: "access, missing, startup_display ...".
inline std::string every_setting_named()
{
    std::string named;
    for (const Setting &setting : kSettings)
        named += (named.empty() ? "" : ", ") + std::string(setting.key);
    return named;
}

} // namespace satellite004
