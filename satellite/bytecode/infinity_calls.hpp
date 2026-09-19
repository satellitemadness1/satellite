#pragma once
// satellite/bytecode/infinity_calls.hpp -- satellite.infinity() and the infinity's
// own methods, run by the interpreter (SATELLITE_INFINITY.md, INF-2).
//
// THE WORD HAS NO LIBRARY, for file_calls.hpp's reason: `satellite.infinity()`
// answers a HANDLE -- a std::shared_ptr to a satellite_infinity -- and a library's
// scenarios only consume values and answer machine codes (number_row.hpp). So the
// word belongs to the object model, as satellite.file's words do, and the checker
// knows it by is_infinity_word() instead of by a library row.
//
// TWO SPELLINGS, ONE WORD. `satellite.infinity()` lexes to `1 26 0` when its brackets
// are empty, and a call with something in them falls back to `1 26` -- there is no
// `satellite.infinity(x)` row -- so both codes are this word. A word with two
// spellings is the shape that hid satellite.feedback's list for a day (the path that
// was not taught went on answering), so is_infinity_word() names both.

#include "expression.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// satellite.infinity `1 26` and satellite.infinity() `1 26 0`.
bool is_infinity_word(token::Code code);

// WHAT THE CHECKER REFUSES BEFORE ANYTHING RUNS, or "" when the call is one INF-2
// builds. `given` is how many arguments the call has: satellite.infinity(x) is
// infinity ** x (message eight, Q27a), and INF-5 builds it.
std::string infinity_word_not_built(token::Code code, std::size_t given);

// satellite.infinity() -- (infinity), its nines width arguments.infinity. The
// arguments are already evaluated.
Value call_infinity_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context);

// AN INFINITY'S OWN METHOD THAT IS NOT BUILT YET, said with the milestone that
// builds it -- ".power_of is not built yet -- ..." -- or "" for any other method.
// power_of (spelled power_of, to_the_power_of and power), resize and nines were
// numbered in INF-1. The checker asks it of a name declared satellite.variable.infinity,
// and the walker of a value that is one.
std::string infinity_method_not_built(token::Code method);

} // namespace satellite004
