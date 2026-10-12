#pragma once
// satellite/bytecode/fraction_values.hpp -- EVERYTHING THE CHECKER AND THE WALKER ASK
// ABOUT A FRACTION, `satellite.variable.fraction` (1 6 20). The value itself is
// satellite_object/object_fraction.hpp's.
//
// ONE FILE PER TYPE, SO FOUR TYPES COULD BE BUILT AT ONCE (2026-09-22): the
// checker, the walker and the expression each ask this header at one line, and
// the type's builder changes this file and not theirs.

#include "expression.hpp"
#include "satellite_legal.hpp"
#include "token_codes.hpp"

#include <bitset>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace satellite004 {

// A FRACTION LITERAL -- 1/3, 1.5/2 -- a number, a TOUCHING slash and a number
// (the author, 2026-09-16: "when you encounter number/number with NO space --
// that becomes a fraction"). expression.cpp has read the number before the slash
// as `top` and sends it here with `at` on the fraction_token; `at` is left past
// the number after it.
signed long long int fraction_literal(const std::string &top, const std::vector<std::bitset<16>> &row,
                                      std::size_t &at, Value &out, std::string &why);

// A SPACED SLASH IN A FRACTION'S VALUE IS THE FRACTION SINCE 2026-10-12 (the author: "what about always a
// fraction? 4 / 2 is the fraction"), so `satellite.variable.fraction f = 1 / 3` holds 1/3 and is no longer
// refused before the run as whole-number division. What the checker still refuses about a fraction's value
// -- fraction arithmetic in it -- is fraction_check.hpp's.

// THE CHECKER: the first method written on a name declared satellite.variable.fraction -- and
// THE WALKER, for a method a fraction has no number for (expression.cpp refuses it in this
// sentence). `spelling` is `name.method`, for the sentence.
signed long long int fraction_method_check(token::Code method, const std::string &spelling, std::string &why);

// THE WALKER: a value arriving in a name declared `holds` -- ANY name, so this
// answers success untouched for every `holds` that is not its business. It may
// change `value` (a number given to a fraction name becomes a fraction).
signed long long int fraction_on_store(token::Code holds, Value &value, std::string &why);

// A FRACTION'S METHODS RUN BY THEIR NUMBERS since 2026-10-11 (scalar_calls.hpp): each one's
// code is a file under satellite-numbers/words/, 1 6 20 n. It was fraction_method here.

} // namespace satellite004
