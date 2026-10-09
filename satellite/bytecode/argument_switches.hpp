#pragma once
// satellite/bytecode/argument_switches.hpp -- A FILE'S OWN SWITCHES (SCRATCH.md/MISSING_SYNTAX.md
// MS-2): a line at the top of a program's file, outside every capsule, that gives one of the run's
// arguments its value before anything is said about the program or runs in it.
//
//     satellite.include(satellite)
//     arguments.missing(false)
//
// (the author, 2026-10-03) "the missing syntax ... can optionally be turned off with
// arguments.missing = false, which by default is set to true, and gives you the tutorial by
// default", and "arguments.missing = false right where a satellite word would go,
// satellite.something.something". So it stands where `satellite.library.span = 25` stands, it is
// read by the same scan (capsule_scopes.cpp), and it has two spellings: `arguments.missing`, and its
// full name, `satellite.library.arguments.missing`.
//
// AND SINCE 2026-10-06, EVERY SETTING, AND TWO WAYS TO WRITE IT (the author: "arguments.item(value_to_
// change_to) ... and if it's typed in a file it temporarily changes a value, and the alias syntax using
// = ... does the same thing"): `arguments.missing(false)` and `arguments.missing = false` are one line,
// and so are `arguments.float.decimal(10)` and `arguments.float.decimal = 10` -- each this run's value,
// set before anything runs, and gone when the run ends (setting_writes.hpp has the whole table).
//
// THE RULES, AND WHOSE EACH ONE IS:
//
//   - A SETTING, AND NOTHING ELSE (argument_settings.hpp). A fact or a name of the program's own is a
//     line outside every capsule (S110), as it was before switches: a row is read inside a capsule,
//     and a name of the program's own is written there.
//
//   - NOT ONE satl READS AS IT STARTS: the start-up lines, the warm threads, the folder the prompt
//     opens in and the display buffer are read before any file is, so a file's line is too late for
//     them (S220) and is told the way that lasts -- typed at the prompt.
//
//   - ITS VALUE IS WRITTEN, NOT WORKED OUT: true or false -- bare or satellite.bool's (D22, the author
//     2026-10-03: "let's allow bare false/true on the special arguments variable as it breaks all of
//     the rules") -- for a switch, a whole number's digits for a count, words in quotes for a folder or
//     a path; nothing after it but the line's end (or its `)` and then the line's end). Nothing else:
//     0, "false" and `false || x` are refused for a switch (S310), as `args.access = 2` is.
//
//   - WRITTEN ONCE IN A FILE (S202), under either spelling, as a satellite.library value is.
//
//   - THE PROGRAM'S OWN FILE SETS THE RUN'S ROW, AND A FILE IT INCLUDES DOES NOT -- a choice the
//     author may reverse (set_the_program_s_switches is the one place). An included file is
//     judged the same, so its line is never wrong; it is that file's own when the file is run by
//     itself, as an included file's satellite.main is, and a file a program includes does not
//     turn the program's tutorial off behind its back. A SPACESHIP -- a file with no
//     satellite.include(satellite) -- never runs by itself, so a switch in one would be read by
//     nothing, and is refused (capsule_scopes.cpp).
//
//   - SET BEFORE ANYTHING IS SAID ABOUT THE PROGRAM. run_satl scans the files and sets the row
//     before the file's shape is checked (include_shape.cpp) and before check_program, so every
//     MISSING code from MS-3 on reads a row the program has already had its say in -- those said
//     before main begins, and those said when the run ends. A line inside a capsule could not be
//     in time for the first, so `args.missing = false` is refused there and pointed here
//     (main_arguments.cpp, program_check.cpp) -- and at the prompt, which saves it for every run.

#include <string>

namespace satellite004 {

class Arguments;
struct CapsuleTable;
struct MachineState;

// WHETHER `key` -- a row's name after `arguments.` -- IS ONE A FILE WRITES AT ITS TOP: a setting
// (argument_settings.hpp). One that satl reads as it starts is refused there as too late, by name.
bool a_switch_a_file_writes(const std::string &key);

// THE SENTENCE FOR A SWITCH WRITTEN INSIDE A CAPSULE -- `args.missing = false`,
// `satellite.library.arguments.missing = false` -- `written` as the line spelled it: it is written
// once, at the top of the file, because one value holds for the whole run and some of what it
// switches is said before main begins. (At the prompt a switch is a setting saved for good.)
std::string a_switch_written_too_late(const std::string &written, const std::string &key);

// THE PROGRAM'S OWN FILE'S SWITCHES, INTO THE RUN'S ROWS: run_satl calls it after the scan and
// before the file's shape is checked. Each one is said under --debug, because satl --debug lists
// the rows before the program is read, when arguments.missing is still the author's default.
void set_the_program_s_switches(const CapsuleTable &table, Arguments &arguments, MachineState &state);

} // namespace satellite004
