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
// (argument1 -- satl itself -- argument2 the file, argument3 onwards its words, then
// length and program: MS-1, numbered as typed), the machine's facts (username,
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
// satellite.library.arguments that is not a row already. Text is a string, a
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
// names written that is a row under satellite.library.arguments -- or "" when the names
// spell none, and the refusal stays the one it was.
std::string arguments_row_written_as_a_word(const std::vector<std::bitset<16>> &row, std::size_t at,
                                            bool after_system);

// THE WALKER: `at` on the `.` after the arguments name `name`. Reads the LONGEST
// run of names that is a row -- `.memory.total` before `.memory` -- live when a
// library answers it, and leaves `at` after it. `read` false means no row starts
// here (a method follows instead); a run that names no row is refused.
// `read_as` is the row's name, as it is filed: "memory.total".
//
// A ROW satl HOLDS IS READ AS IT IS NOW (2026-10-06), out of the run's own rows and not
// out of the copy main was handed: a line may change a setting while the program runs
// (setting_writes.hpp), by this name or any other, and the next read must see it.
// `the_runs` is THE RUN'S OWN: the bare name `arguments` where no name of the body is
// called that (the author, 2026-10-04: "arguments.missing or arguments.anything needs
// to be able to be read anywhere"), which reads satl's rows and has no copy of main's.
Value read_an_argument(const std::vector<std::bitset<16>> &row, std::size_t &at, const std::string &name,
                       const Value &arguments, ExpressionContext &context, bool &read, std::string &read_as,
                       bool the_runs = false);

// THE RUN'S OWN ARGUMENTS BY THEIR BARE NAME (the author, 2026-10-04: "arguments.anything needs
// to be able to be read anywhere"): `arguments` in a capsule that declares no name called that,
// and at the prompt. Its rows are read where they are (read_an_argument, with no copy), its
// settings changed (setting_writes.hpp); a name of the program's own is main's to write.
inline const char *const kTheRunsArguments = "arguments";

// A KEY HANDED TO THE ARGUMENTS VARIABLE AS TEXT (MS-1, and a fresh reader the same day) --
// `args["arg3"]`, `args.contains("arguments3")`, `args.remove("argument03")` -- names the row
// the dotted read names: argument3, ONE row under every spelling. (.get and .set are not here:
// the checker refuses both on the arguments variable before anything runs.) The old argument_N and an argument numbered 0 are refused in the dotted read's
// own words. A key that is not text is left as it is. False when it has refused.
bool an_argument_key(Value &key, const std::string &name, ExpressionContext &context);

// THE WALKER, WRITING -- `argz.some_var = some_value` (the author, 2026-09-23: "the syntax
// is: satellite.variable.arguments any_name then any_name.some_var = some_value"). 003
// refused both a new row (S0532) and a held one (S0724); 004 takes a new one.
//
// A NAME OF THE PROGRAM'S OWN IS ITS OWN: added the first time, overwritten after (A21),
// shown by satellite.console.display(argz) after the rows satl gave it -- and `argz.x(5)` is
// `argz.x = 5` (the author, 2026-10-06: "the alias syntax using = ... does the same thing").
// A SETTING IS CHANGED (setting_writes.hpp, 2026-10-06): for this run by a line in a file, for
// good -- config.ini -- by one typed at the prompt. Until then `argz.access = false` in a program
// wrote config.ini, and no other setting could be written.
// A ROW satl HOLDS IS REFUSED -- a fact is what the machine has (Part 4C: read-only by
// having no write path), the command line and config.ini are what the person gave, and
// a copy that said otherwise would be the program lying to itself about them. So is a
// name INSIDE a row, `args.l.size`: that is a member of the row l, not a row.
//
// "" when `name.key = ...` may be written, and otherwise the refusal. Shared by the
// checker, which refuses the line before anything runs (`rows` nullptr: it knows satl's
// rows and not the program's), and the walker, which asks again before the value is
// worked out -- run_setting_assignment's order, so a refused row never asks for input.
//
// SINCE 2026-10-06 A SETTING IS WRITTEN BY EVERY NAME FOR THE ARGUMENTS (the author: "if it's
// typed ON the prompt it permanently changes the value, and if it's typed in a file it temporarily
// changes a value"), and `where` says which of those this line is: typed at the prompt or in a file,
// and by main's name or the run's own arguments' bare one (setting_writes.hpp has the table).
struct ArgumentWrite {
    bool at_the_prompt = false;   // for good, and never too late
    bool the_runs = false;        // the bare `arguments`: satl's rows, no copy of main's
};
std::string why_an_argument_is_not_written(const std::string &key, const std::string &name, const Value *rows,
                                           const Arguments *arguments, const FunctionTable &functions,
                                           const ArgumentWrite &where);
// And once it may be: the setting first, when `key` is one -- for good or for this run, as
// `where` says -- then the variable's own copy, when it has one.
void write_an_argument(const std::string &key, Value value, const std::string &name, Value &arguments,
                       ExpressionContext &context, const ArgumentWrite &where);

// THE CALL THAT WRITES A ROW (the author, 2026-10-06: "arguments.item(value_to_change_to)"). `at` on
// the `.` after the arguments' name. True when the line goes on `.<row>(` -- the row's dotted names,
// the last one touching its `(` -- with `key` the row ("float.decimal") and `open` on the `(`. A
// setting's whole name, or ONE plain name (`args.my_value(5)`), is a write; the variable's own methods
// -- `args.size()`, `args.remove("x")`, a map's .get and .set -- are not, and neither is a call that
// reaches into a row, `args.b.call_put(9)`: they are read as they always were. `any_names` -- a file's top,
// where nothing is the program's own and a call is a line to judge whatever it names -- takes any plain names.
bool an_argument_call(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &key, std::size_t &open,
                      bool any_names = false);

// A BARE true OR false AS THE ONE VALUE IN A CALL'S BRACKETS -- `arguments.missing(false)` --
// `at` on the code after the `(`: "true" or "false" when that name is all there is before its `)`
// and the `)` ends the line; nullptr otherwise. MS-2's D22, for the call as for the `=`.
const char *a_bare_true_or_false_in_brackets(const std::vector<std::bitset<16>> &row, std::size_t at);
// A BARE true OR false, WRITTEN TO THE ARGUMENTS (MS-2's D22, the author 2026-10-03: "let's
// allow bare false/true on the special arguments variable as it breaks all of the rules, and also
// allow it to take satellite.bool.true, or satellite.bool.false as that is clear by what the user
// meant"). `at` on the first code of the value in `args.row = ...`, `args["row"] = ...`,
// `satellite.library.arguments.access = ...` or a file's `arguments.missing = ...`: "true" or
// "false" when it is that name, bare, with nothing after it but the line's end -- nullptr for
// anything else, so `args.row = false || x` is read as it always was. satellite.bool.true and
// .false need nothing here: they are the two values wherever a value goes. THERE, THE BARE WORD IS
// THE VALUE, ALWAYS -- "as it breaks all of the rules" -- read off the codes alone, so the checker
// and the walker cannot disagree: asking which names were declared, they did (the fresh reader,
// 2026-10-04: a `false` declared further down a loop's body gave one line two values in one run).
// A variable a program named false is read there as `(false)`, in brackets; no program of the
// author's declares one (measured that day).
const char *a_bare_true_or_false(const std::vector<std::bitset<16>> &row, std::size_t at);

// WHETHER A WORD IS ONE OF THE ARGUMENTS': spelled under satellite.library.arguments, or under
// satellite.library.main.arguments, where they lived until 2026-10-03.
bool a_word_of_the_arguments(token::Code word);

// A ROW THAT HOLDS A CONTAINER, TO CHANGE IN PLACE: `args.l.append(5)`, `args.l[1] = 5`.
// The row's own slot in the variable (copied first when the rows are shared), or nullptr
// and a refusal for a row satl holds, a setting -- given a value whole, never changed in
// place -- or one the program never wrote.
Value *an_argument_to_change(const std::string &key, const std::string &name, Value &arguments,
                             ExpressionContext &context, bool the_runs = false);

} // namespace satellite004
