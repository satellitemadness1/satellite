#pragma once
// satellite/bytecode/color_values.hpp -- EVERYTHING THE CHECKER AND THE WALKER ASK
// ABOUT A COLOR, `satellite.variable.color` (1 6 19). The value itself is
// satellite_object/object_color.hpp's.
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

// THE CHECKER, BEFORE ANYTHING RUNS: the value given to a name declared satellite.variable.color,
// `at` on the first code straight after the `=` -- in the declaration and in every
// later `name = ...`. `declared` is every name the checker knows and its type word.
signed long long int color_is_written_right(const std::vector<std::bitset<16>> &row, std::size_t at,
                                            const LegalNames &declared,
                                            std::string &why);

// THE CHECKER: where the names of `c = ...` start, for a name declared satellite.variable.color.
// `at` is the first code after the `=` and `from` is where the checker would start.
// SIX HEX DIGITS WRITTEN WITHOUT THE x -- `c = ff00aa` -- ARE NOT A NAME, so this
// answers the code after them; for any other value it answers `from` untouched.
std::size_t color_names_start(const std::vector<std::bitset<16>> &row, std::size_t at, std::size_t from,
                              const LegalNames &declared);

// THE CHECKER: the first method written on a name declared satellite.variable.color -- and
// THE WALKER, for a method a colour has no number for (expression.cpp refuses it in this
// sentence). `spelling` is `name.method`, for the sentence.
signed long long int color_method_check(token::Code method, const std::string &spelling, std::string &why);

// THE WALKER: the value given to a name declared `holds`, `at` just past the `=`.
// FALSE, `at` untouched, for every name that is not a colour's -- the walker then
// reads the value as it reads every other. For a colour name it reads the value
// itself: `000000` and `ff00aa` as six hex digits, anything else as the expression
// it is, and then `, 50` after it, the transparency. `at` is left on what follows,
// and a refusal is in `context`, as evaluate_expression leaves one.
bool color_reads_its_value(token::Code holds, const std::vector<std::bitset<16>> &row, std::size_t &at, Value &value,
                           ExpressionContext &context);

// THE WALKER: a value arriving in a name declared `holds` -- ANY name, so this
// answers success untouched for every `holds` that is not its business. It may
// change `value` (a six-digit hex given to a color name becomes a color).
signed long long int color_on_store(token::Code holds, Value &value, std::string &why);

// A TRANSPARENCY GIVEN -- the author's "0 - 99", both ends in -- read the same way from
// both of his ways of writing one: `c = x000000, 50` (color_reads_its_value, above) and
// `c.satellite.transparency(50)` (1 6 19 1). `out` is it; false, with `why`, when it is not one.
bool a_transparency(const Value &given, unsigned int &out, std::string &why);

// A COLOUR'S METHODS RUN BY THEIR NUMBERS since 2026-10-11 (scalar_calls.hpp): each one's
// code is a file under satellite-numbers/words/, 1 6 19 n. It was color_method here.

} // namespace satellite004
