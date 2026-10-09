#pragma once
// satellite/bytecode/library_values.hpp -- satellite.library: VALUES WRITTEN AT THE TOP OF A
// FILE, and read by name everywhere a value goes.
//
// (the author, 2026-09-23) "we need to design it so that globals don't work, but
// satellite.library does work". The two halves of that sentence are the two rules here:
//
//   - A VALUE IS WRITTEN DOWN, NEVER WORKED OUT. `satellite.library.span = 25` stands at the
//     top of its file, outside every capsule, and its value is ONE LITERAL: a number, a
//     float, text, a binary, a hex, a percentage, a fraction, or satellite.bool.true or
//     .false, with a minus sign in front if it has one. Nothing runs to make it --
//     execution begins and ends inside main (the author, 2026-09-16) -- so there is no
//     moment outside a capsule for `= helper()` to happen in. The check reads each one
//     once, before anything runs (read_library_values).
//
//   - NOTHING CHANGES ONE. `satellite.library.span = 30` inside a capsule is refused before
//     anything runs (S250): a value every capsule could change is a global, and satellite
//     has none. A body that wants to work with it copies it into a variable of its own.
//
// WHO SEES WHICH, 003's rule (DESIGN §7.2, M25): a file's own capsules read
// `satellite.library.span`; a file that includes settings.satl reads
// `satellite.library.settings.span`; a file reaches no values but its own and those of the
// files it includes itself, exactly as it reaches capsules (capsule_scopes.hpp). Measured
// on the author's programs 2026-09-23: 320 values written, every one a literal; 417 bare
// reads, every one of its own file's; 290 reads through `settings` and `combine_settings`;
// and no capsule anywhere writes one.
//
// WHAT 003 HAD AND THIS DOES NOT, said so nobody finds it by surprise: 003's globals were
// VARIABLES -- any capsule could write one, and threads shared it behind a lock (003's
// evaluator/globals.hpp). That is the half the author ruled out. 003 also answered
// `satellite.library.capsule_name.variable` for a capsule's literal locals (M26); no
// program of his uses it, and it is not built.
//
// FOUR READERS, ONE SHAPE: the scan records each value (library_line, capsule_scan.hpp);
// the check reads each once and judges every line that names one; the expression hands
// one back. All of them read the names after satellite.library through library_names_at,
// so they cannot disagree about what `satellite.library.settings.span.upper()` names.

#include "bytecode_registry.hpp"
#include "capsule_scopes.hpp"
#include "expression.hpp"
#include "function_table.hpp"
#include "value.hpp"
#include "../machine/machine_state.hpp"

#include <bitset>
#include <cstddef>
#include <string>
#include <vector>

namespace satellite004 {

// satellite.library, the word the two spellings start with.
bool is_library_word(token::Code code);

// THE NAME A VALUE WRITTEN AS `written` IS KEPT UNDER. A name spelled like a method --
// `size`, `color`, `string` -- lexes after satellite.library's dot as that method's CODE,
// and a code reads back as the method's first spelling (`colour`, `to_string`), which is
// all the lexer keeps; every other name is itself.
std::string library_name_of(const std::string &written);

// THE NAMES AFTER satellite.library, `at` on its code: `span`, or `settings.span` when the
// first is a file row `r` includes -- `r` is the row's index in the registry. `at` is
// left on what follows them: a method on the value, an operator, the line's end. False,
// with `at` unmoved, when no name follows the word's `.`.
bool library_names_at(const CapsuleTable &table, std::size_t r, const std::vector<std::bitset<16>> &row,
                      std::size_t &at, std::vector<std::string> &names);

// THE VALUE `names` REACH from the file row `r` is, or null with the sentence in `why`.
const LibraryValue *library_value_named(const CapsuleTable &table, std::size_t r,
                                        const std::vector<std::string> &names, std::string &why);

// THE CHECK'S HALF, INSIDE A LINE (names_in_statement): `at` on the satellite.library code,
// left past the names when they reach a value. The line's own registry says which file
// it is in; a typed line has none, and is refused by name. Brackets after the names, or a
// name that is no method, are refused here; `written` and `type` -- the word its literal's
// type is declared with, satellite.variable.number for 25 -- are handed back so a method
// after it is judged as it is on a variable of that type (read_library_values ran first).
signed long long int library_read_is_right(const CapsuleTable &table, const BytecodeRegistry &registry,
                                           const std::vector<std::bitset<16>> &row, std::size_t &at,
                                           std::string &written, token::Code &type, std::string &why);

// A STATEMENT THAT STARTS WITH ONE (check_statement): always refused, and `at` left past
// it. Writing it is S250; a line that only reads it does nothing and says so.
signed long long int library_statement(const CapsuleTable &table, const BytecodeRegistry &registry,
                                       const std::vector<std::bitset<16>> &row, std::size_t &at,
                                       std::string &why);

// EVERY FILE'S VALUES, READ ONCE, BEFORE ANYTHING RUNS -- called by check_program after
// the scan's refusals and before any capsule is judged. A value the literal readers
// refuse (text that is not UTF-8) is reported with its line and caret.
signed long long int read_library_values(const BytecodeRegistry &registry, const CapsuleTable &table,
                                         const FunctionTable &functions, MachineState &state);

// THE EXPRESSION'S HALF: the value, `at` on the satellite.library code and left past the
// names -- a method after them is the caller's to call.
Value library_value_at(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context);

} // namespace satellite004
