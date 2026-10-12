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
#include "word_codes.hpp"

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

// A MAP'S WORD WRITTEN WHERE A ROW WOULD BE -- `args.size`, `satellite.arguments.first` -- with no brackets, as
// the walker reads it: no row is named so, and the method runs (main_arguments.cpp, read_an_argument). The
// arguments' methods are a map's, written under satellite since 2026-10-11, so the checker judges such a run
// as that method written with the other period, before anything runs. `at` on the `.` after the name.
bool a_map_word_where_a_row_would_be(const std::vector<std::bitset<16>> &row, std::size_t at);

// THE KIND OF THE ROW satl HOLDS UNDER `key`, as the walker would read it (read_an_argument): a row gathered at
// start-up or a setting -- text a string, a count, a number or a size a number, a switch a bool, a list a list of
// text -- or a library word with a fact or a switch to answer (live_row), asked now as it is when its line runs.
// how_many_kinds for anything else: a program's own row, a word with no row behind it (machine, session), and a name
// the walker refuses before it reads (argument0, argument_1). The checker's (the author's N of 2026-10-11): what a
// word after a row is on.
satelliteObject::Kind kind_of_a_row_satl_holds(const std::string &key, const Arguments *arguments,
                                               const FunctionTable &functions);

// A WORD OF THE LANGUAGE AFTER A ROW, written with no brackets (the author's N of 2026-10-11):
// `satellite.arguments.username.size` is the row username and then its .size, where past_the_argument_names reads
// one dotted run. Split as the walker splits it -- the longest run that names a row it reads -- for the rows the
// checker can know: satl's own (kind_of_a_row_satl_holds, which says `kind`). `at` on the `.` after the arguments'
// name; answers the `.` before the word, with `key` the row before it, or `at` with `key` empty when no word follows
// such a row. A program's own rows, `args.my_row.size`, are the run's to know: one may be named with a word in it.
std::size_t a_word_after_the_rows(const std::vector<std::bitset<16>> &row, std::size_t at, const Arguments *arguments,
                                  const FunctionTable &functions, std::string &key, satelliteObject::Kind &kind);

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
// `the_runs` is THE RUN'S OWN: satellite.arguments (the author, 2026-10-04: "arguments.missing
// or arguments.anything needs to be able to be read anywhere"), which reads satl's rows and has
// no copy of main's.
Value read_an_argument(const std::vector<std::bitset<16>> &row, std::size_t &at, const std::string &name,
                       const Value &arguments, ExpressionContext &context, bool &read, std::string &read_as,
                       bool the_runs = false);

// THE RUN'S OWN ARGUMENTS ARE satellite.arguments (the author, 2026-10-11: "arguments becomes
// satellite.arguments but the arguments like .username .machine don't get the word "satellite", so we
// don't type satellite.arguments.satellite.username, we just enter satellite.arguments.username"): ONE
// WORD, 1 34, read in every capsule and at the prompt where the bare name `arguments` was read from
// 2026-10-04 until that day (the author: "arguments.anything needs to be able to be read anywhere"). Its
// rows are read where they are (read_an_argument, with no copy), its settings changed
// (setting_writes.hpp); a name of the program's own is main's to write. ITS METHODS ARE A MAP'S AND ARE
// UNDER SATELLITE, its rows are data and are not: satellite.arguments.satellite.size,
// satellite.arguments.username -- after `.satellite.` a method, after `.` a row, so the two never meet.
// kTheRunsArguments is how every sentence spells it, and the name the walker and the checker carry for it.
inline const char *const kTheRunsArguments = "satellite.arguments";
inline constexpr token::Code kTheRunsArgumentsWord = word::fixed_code<1, 34>;
static_assert(kTheRunsArgumentsWord != 0, "satellite.arguments (1 34) is missing from words/words.tsv");

// THE BARE NAME IT HAD UNTIL THEN IS A NAME LIKE ANY OTHER: a program's own -- satellite.main's parameter,
// most often, `satellite.main(satellite.variable.arguments arguments)` -- and where no name of the body is
// called that, refused before anything runs, naming the new spelling. `after` is the code after the name;
// answers the sentence, "arguments.username is written satellite.arguments.username now -- ...".
inline const char *const kTheOldBareArguments = "arguments";
std::string the_bare_arguments_refused(const std::vector<std::bitset<16>> &row, std::size_t after);

// A KEY HANDED TO THE ARGUMENTS VARIABLE AS TEXT (MS-1, and a fresh reader the same day) --
// `args["arg3"]`, `args.contains("arguments3")`, `args.remove("argument03")` -- names the row
// the dotted read names: argument3, ONE row under every spelling. (.get and .set are not here:
// the checker refuses both on the arguments variable before anything runs.) The old argument_N and an argument numbered 0 are refused in the dotted read's
// own words. A key that is not text is left as it is. False when it has refused.
bool an_argument_key(Value &key, const std::string &name, ExpressionContext &context);

// .get AND .set ON THE ARGUMENTS VARIABLE ITSELF ARE REFUSED (the integrator's ruling of 2026-10-11: as they
// were while the two were matched by name, and no further -- the author's arguments.get("program") is a
// question of his own). The variable's other methods are a map's, 1 4 1 n, under satellite; a row is read
// and written by its name. "" for any other method, or the refusal, naming what to write instead. Asked by
// the checker, before anything runs, and by the walker with the same words.
std::string get_or_set_on_the_arguments(token::Code method, const std::string &name);

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

// A BARE true OR false AS THE ONE VALUE IN A CALL'S BRACKETS -- `satellite.arguments.missing(false)` --
// `at` on the code after the `(`: "true" or "false" when that name is all there is before its `)`
// and the `)` ends the line; nullptr otherwise. The file-top switch line asks it, to refuse the bare
// word naming satellite.bool.<it>, as a_bare_true_or_false below says.
const char *a_bare_true_or_false_in_brackets(const std::vector<std::bitset<16>> &row, std::size_t at);
// A BARE true OR false, WRITTEN AS A SWITCH LINE'S VALUE at the top of a file -- `satellite.arguments.missing
// = false` -- `at` on the value's first code: "true" or "false" when it is that name, bare, with nothing after
// it but the line's end; nullptr for anything else. ASKED ONLY TO REFUSE IT, naming satellite.bool.<it>: a bare
// true or false is no value anywhere in satellite, the arguments included (the author, 2026-10-11: "So there
// should be no true then only an error message, and satellite.bool.true"). That ends MS-2's D22 of 2026-10-03
// ("let's allow bare false/true on the special arguments variable as it breaks all of the rules"), under which
// the bare word was the value here; in a capsule and at the prompt it is now judged as any value is, and
// refused as an undeclared name is (expression.cpp name_not_declared_why).
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
