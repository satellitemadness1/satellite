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
// dotted run of row names (`.memory.total`), stopping at a method's `(`.
std::size_t past_the_argument_names(const std::vector<std::bitset<16>> &row, std::size_t at);

// THE WALKER: `at` on the `.` after the arguments name `name`. Reads the LONGEST
// run of names that is a row -- `.memory.total` before `.memory` -- live when a
// library answers it, and leaves `at` after it. `read` false means no row starts
// here (a method follows instead); a run that names no row is refused.
Value read_an_argument(const std::vector<std::bitset<16>> &row, std::size_t &at, const std::string &name,
                       const Value &arguments, ExpressionContext &context, bool &read);

} // namespace satellite004
