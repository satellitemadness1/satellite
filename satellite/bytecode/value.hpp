#pragma once
// satellite/bytecode/value.hpp -- where a capsule's variables live, and the one
// name the walker knows the object model by.
//
// WHAT CHANGED, 2026-09-16: `Value` IS `satelliteValue` NOW. This file used to
// define its own value type -- a Kind enum beside a std::string, a
// satellite_number and a bool, all three always present. The object model
// replaces it (satellite_object/satellite_value.hpp), and this file keeps only
// what is about the WALKER rather than about values: a variable, and the table a
// running body holds them in.
//
// THE ALIAS IS DELIBERATE AND IS NOT A TRANSITION SHIM. `Value` is what the
// walker calls what an expression is worth, and satelliteValue is what the
// object model calls it. Both names are right in their own file, and one
// `using` is cheaper than renaming the word `Value` through every line of
// expression.cpp and program_walk.cpp -- where it reads correctly already.
//
// WHAT THE NEW TYPE BUYS THE WALKER, beyond the arms it did not have:
//   - A STRING IS A satellite_string, not a std::string of UTF-8 bytes. The
//     language's own 16/32-bit string with the author's character table reaches
//     the interpreter for the first time; UTF-8 is now only a doorway, at the
//     literal coming in (`of_utf8`) and at a library's text scenario going out
//     (`text_utf8`).
//   - ONE PLACE DECIDES WHAT TWO KINDS DO. `left.add(right, out, why)` routes on
//     the pair of tags to a one-function-one-file header. expression.cpp no
//     longer carries a branch per pair.
//
// THERE ARE NO GLOBALS (the author, 2026-09-16): "we begin exe inside of main,
// and end exe inside of main... the only globals are the includes, other files".
// So a VariableTable belongs to ONE running body and is created by run_body when
// that body starts. A capsule cannot see its caller's variables, because it is
// handed a different table -- enforced by construction rather than by a check
// that could be forgotten.
//
// A VARIABLE REMEMBERS THE WORD THAT DECLARED IT. `satellite.variable.number n`
// stores the code of satellite.variable.number beside the value, so a later
// `n = "text"` is refused with types_do_not_meet (27) instead of quietly making
// n a string. satellite is typed by its declaration, and this is where that
// survives past the line that wrote it.

#include "token_codes.hpp"
#include "../satellite_object/satellite_spacesuit.hpp"

#include <string>
#include <unordered_map>

namespace satellite004 {

using Value = satelliteValue;

struct Variable {
    token::Code declared = 0;   // the word code of satellite.variable.number, .string, ...
    Value value;
};

// One running body's variables. Created per body, never shared: see the header.
using VariableTable = std::unordered_map<std::string, Variable>;

} // namespace satellite004
