#pragma once
// satellite/bytecode/color_values.hpp -- EVERYTHING THE CHECKER AND THE WALKER ASK
// ABOUT A COLOR, `satellite.variable.color` (1 6 19). The value itself is
// satellite_object/object_color.hpp's.
//
// ONE FILE PER TYPE, SO FOUR TYPES COULD BE BUILT AT ONCE (2026-09-22): the
// checker, the walker and the expression each ask this header at one line, and
// the type's builder changes this file and not theirs.

#include "expression.hpp"
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
                                            const std::unordered_map<std::string, token::Code> &declared,
                                            std::string &why);

// THE CHECKER: the first method written on a name declared satellite.variable.color. `spelling`
// is `name.method`, for the sentence.
signed long long int color_method_check(token::Code method, const std::string &spelling, std::string &why);

// THE WALKER: a value arriving in a name declared `holds` -- ANY name, so this
// answers success untouched for every `holds` that is not its business. It may
// change `value` (a number given to a color name becomes a color).
signed long long int color_on_store(token::Code holds, Value &value, std::string &why);

// THE WALKER: a method on a value of this type. `slot` is the variable itself when
// the method is written straight on a name (a method that changes the value writes
// through it), and nullptr otherwise. `answered` false leaves the method to the
// conversions every type shares (.string, .number, .binary, .hex).
Value color_method(token::Code method, const Value &receiver, Value *slot, const std::vector<Value> &arguments,
                   bool had_parentheses, const std::string &name, ExpressionContext &context, bool &answered);

} // namespace satellite004
