#pragma once
// satellite/bytecode/setting_writes.hpp -- A SETTING CHANGED BY A LINE. The author, 2026-10-06:
//
//     "arguments.item(value_to_change_to) and if it's typed ON the prompt it permanently changes
//      the value, and if it's typed in a file it temporarily changes a value, and the alias syntax
//      using = ... that does the same thing, and has the same rules as changing it permanently on
//      the console, or temporarily inside of a file"
//
// SO WHERE THE LINE IS SAYS HOW LONG IT LASTS, AND HOW IT IS SPELLED DOES NOT:
//
//                                  arguments.x(v)   arguments.x = v
//     typed at the prompt          for good         for good
//     in a file, in a capsule      this run         this run
//     in a file, at its top        this run         this run
//
// THERE IS NO .permanent. The author ruled `arguments.missing.permanent(value)` on 2026-10-04; it
// was built with the rest on 2026-10-06 and taken out the same day on his word -- "so you took out
// the .permanent() right??" -- so the prompt is the one way a setting lasts.
//
// FOR GOOD IS config.ini (config/config_file.hpp), which every satl reads as it starts
// (arguments.cpp), and the run's own row too, so the next line reads what was saved. THIS RUN IS
// THE RUN'S ROW ALONE (Arguments::set_for_this_run): config.ini is not touched, and the value is
// gone when satl ends. A file run from the prompt -- `interpret <file>` -- is a satl of its own
// (satl/prompt_run.cpp), so a file's change never reaches the prompt that ran it.
//
// WHICH ROWS: the settings (arguments/argument_settings.hpp), and no fact. A fact is refused where
// it always was (main_arguments.cpp); a name of the program's own is the program's (main's copy).
//
// WHAT THE RUN COPIED OF A ROW IS COPIED AGAIN, so the change holds from that line on: the float's
// precisions (float_precision.hpp) and satellite.log's place. An infinity's width is read where one
// is made. A row satl reads only as it starts is too late for a file's line (why_a_file_line_is_too_late).

#include "value.hpp"
#include "../arguments/argument_settings.hpp"

#include <string>

namespace satellite004 {

struct Argument;
struct MachineState;

enum class Lasting {
    this_run,   // the run's row: a line in a file
    for_good,   // config.ini and the run's row: a line typed at the prompt
};

// THE VALUE A ROW HOLDS, as the arguments variable shows it: text a string, a count or a number a
// number, a switch a bool, a size its exact bytes, a list of words a list.
Value the_value_of_a_row(const Argument &row);

// "" WHEN `value` IS WHAT `setting`'s ROW HOLDS -- true or false for a switch, a whole number of at
// least its least for a count, words for a folder or a path -- and otherwise why not, in the words of
// the line that wrote it (`written`: "args.infinity"), with its code in `code`.
std::string why_a_setting_does_not_take(const Setting &setting, const Value &value, const std::string &written,
                                        signed long long int &code);

// "" WHEN A LINE IN A FILE MAY GIVE `setting` A VALUE FOR ITS OWN RUN, and otherwise the sentence: a
// row satl reads as it starts is read before any file is, and the program's switch is said at the top
// of its file (`at_the_top` -- where it is not too late). The prompt is never too late: what it saves
// is read by every run after.
std::string why_a_file_line_is_too_late(const Setting &setting, const std::string &written, bool at_the_top);

// THE LINE'S VALUE, TAKEN: judged first, so a refused value changes nothing; then config.ini, for
// good; then the run's row and what the run copied of it. success, or the refusal with `why`.
signed long long int change_a_setting(const Setting &setting, const Value &value, Lasting lasting,
                                      const std::string &written, MachineState &state, std::string &why);

// A VALUE WRITTEN AT A FILE'S TOP, AS THE SCAN KEPT IT (argument_switches.cpp) -- "true", the digits
// of a whole number, or the words -- made into the value the line gave.
Value a_value_the_scan_kept(const Setting &setting, const std::string &kept);

} // namespace satellite004
