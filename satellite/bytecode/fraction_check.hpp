#pragma once
// satellite/bytecode/fraction_check.hpp -- A FRACTION WHERE NONE CAN GO, SAID BEFORE ANYTHING RUNS.
//
// EVERY a / b OF TWO NUMBERS IS A FRACTION, KEPT AS WRITTEN (the author, 2026-10-12: "what about always
// a fraction? 4 / 2 is the fraction"), and his 9b of the same day: `satellite.variable.number n = a / b`
// is refused BEFORE the run -- "declare a fraction, or write satellite.convert.number(a / b)".
//
// A VALUE'S KIND IS IN GENERAL ONLY KNOWN WHEN IT RUNS, but a value given to a name is mostly literals
// and names, and a name's declaration says its kind -- so for those the kind is known here, from the
// tokens and the declarations alone. THE READER IS SMALL, AND SURE OR SILENT: it reads + - * / % ^ and
// the comparisons, brackets, a touching minus, number, float, fraction, binary and hex literals, names
// declared a number, a float, a fraction, a binary or a hex, and satellite.convert.number(...) and
// satellite.convert.float(...). Anything else -- a call, an item, a method, a name of another kind --
// leaves the kind unknown, and an unknown kind is the walker's to judge when the line runs.
//
// WHAT IT REFUSES, in a value given to a name declared a number, a float or a fraction:
//
//   satellite.variable.number n = a / b      a fraction where a number goes (his 9b) -- S301,
//                                            types_do_not_meet, as the walker says it
//   satellite.variable.float x = 7 / 2       a fraction where a float goes, the same way
//   n = a / b + 1                            a fraction under + - * / % ^: fraction arithmetic is not
//                                            built yet -- S210, not_built_yet, as the walker says it
//
// A binary or a hex divided is whole (his 9f), so `n = worth / b1001` is a number and is never refused.

#include "satellite_legal.hpp"

#include <bitset>
#include <cstddef>
#include <string>
#include <vector>

namespace satellite004 {

// `type` is the word `name` was declared with, and the value starts at `value_at`. success, or the
// refusal with `why` in the walker's own words.
signed long long int fraction_given_where_none_goes(token::Code type, const std::string &name,
                                                    const std::vector<std::bitset<16>> &row, std::size_t value_at,
                                                    const LegalNames &declared, std::string &why);

} // namespace satellite004
