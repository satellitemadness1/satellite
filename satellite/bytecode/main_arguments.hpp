#pragma once
// satellite/bytecode/main_arguments.hpp -- THE ARGUMENTS VARIABLE:
//
//     satellite.capsule satellite.main(satellite.variable.arguments anything_typed_in_here)
//
// (the author, 2026-09-22) "we have talked so much about this arguments variable,
// it holds arguments.username/linux_username arguments.memory.total
// arguments.memory.used ... build the arguments variable so the program works
// first, and include whatever you can in the arguments variable", and then "let's
// skip the satellite.container.list<satellite.variable.string> stuff and just
// write satellite.variable.arguments name_user_wants_to_use". SATELLITE_ARGUMENTS.md
// Part 1 is the brief it answers: the arguments are ONE object holding every
// argument by name, turned on by main declaring it -- `satellite.main()` with no
// parameter asks for nothing.
//
// EVERY ROW satl HOLDS, AS ONE INDEX: the author's config rows, the command line
// (program, argument_1 onwards, length), the machine's facts (username,
// memory.total, cores ...) and every fact and setting the numbered libraries
// answer live (memory.used, memory.free, access ...). Keyed by the row's name
// without `arguments.`, so the variable's own name supplies it back:
// `arguments.username` is the row `username` of the variable `arguments`, and
// `args.username` is the same row when main names it `args`.
//
// THE OLD SPELLING STILL WORKS. Every 004 program before this was written
// `satellite.main(satellite.container.list<satellite.variable.string> arguments)`,
// and main's parameter is the arguments variable whatever type it was declared as.

#include "function_table.hpp"
#include "value.hpp"
#include "expression.hpp"

#include <bitset>
#include <cstddef>
#include <string>
#include <vector>

namespace satellite004 {

class Arguments;

// Every row (an empty index when `arguments` is nullptr), in the order satl
// gathered them, then every live fact and setting under
// satellite.library.main.arguments that is not a row already. Text is a string, a
// count or a number is a number, a switch is a bool, a size is its exact bytes.
Value the_arguments_value(const Arguments *arguments, const FunctionTable &functions);

// THE CHECKER: `at` on the `.` after an arguments name; answers the code after the
// dotted run of row names (`.memory.total`), stopping at a method's `(`. The second
// also spells the run as its row is named: "memory.total".
std::size_t past_the_argument_names(const std::vector<std::bitset<16>> &row, std::size_t at);
std::size_t past_the_argument_names(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &key);

// A MACHINE FACT WRITTEN AS satellite.<fact> (ERRORS2 #9): `satellite.machine.cores()` is no
// word, and was told "machine has no satellite.variable line declaring it" -- true, and about
// something else. `at` on the first name after `satellite.` (or after `satellite.system.`, when
// `after_system`); answers the refusal that says what 004 spells it -- the longest run of the
// names written that is a row under satellite.library.main.arguments -- or "" when the names
// spell none, and the refusal stays the one it was.
std::string arguments_row_written_as_a_word(const std::vector<std::bitset<16>> &row, std::size_t at,
                                            bool after_system);

// THE WALKER: `at` on the `.` after the arguments name `name`. Reads the LONGEST
// run of names that is a row -- `.memory.total` before `.memory` -- live when a
// library answers it, and leaves `at` after it. `read` false means no row starts
// here (a method follows instead); a run that names no row is refused.
// `read_as` is the row's name, as it is filed: "memory.total".
Value read_an_argument(const std::vector<std::bitset<16>> &row, std::size_t &at, const std::string &name,
                       const Value &arguments, ExpressionContext &context, bool &read, std::string &read_as);

// THE WALKER, WRITING -- `argz.some_var = some_value` (the author, 2026-09-23: "the syntax
// is: satellite.variable.arguments any_name then any_name.some_var = some_value"). 003
// refused both a new row (S0532) and a held one (S0724); 004 takes a new one.
//
// A NAME OF THE PROGRAM'S OWN IS ITS OWN: added the first time, overwritten after (A21),
// shown by satellite.console.display(argz) after the rows satl gave it.
// A SETTING IS WRITTEN THROUGH: `argz.access = false` is B2's `arguments.access = false`.
// A ROW satl HOLDS IS REFUSED -- a fact is what the machine has (Part 4C: read-only by
// having no write path), the command line and config.ini are what the person gave, and
// a copy that said otherwise would be the program lying to itself about them. So is a
// name INSIDE a row, `args.l.size`: that is a member of the row l, not a row.
//
// "" when `name.key = ...` may be written, and otherwise the refusal. Shared by the
// checker, which refuses the line before anything runs (`rows` nullptr: it knows satl's
// rows and not the program's), and the walker, which asks again before the value is
// worked out -- run_setting_assignment's order, so a refused row never asks for input.
std::string why_an_argument_is_not_written(const std::string &key, const std::string &name, const Value *rows,
                                           const Arguments *arguments, const FunctionTable &functions);
// And once it may be: the setting first, when `key` is one, then the variable's own copy.
void write_an_argument(const std::string &key, Value value, const std::string &name, Value &arguments,
                       ExpressionContext &context);
// A ROW THAT HOLDS A CONTAINER, TO CHANGE IN PLACE: `args.l.append(5)`, `args.l[1] = 5`.
// The row's own slot in the variable (copied first when the rows are shared), or nullptr
// and a refusal for a row satl holds or one the program never wrote.
Value *an_argument_to_change(const std::string &key, const std::string &name, Value &arguments,
                             ExpressionContext &context);

} // namespace satellite004
