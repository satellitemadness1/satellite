#pragma once
// satellite/bytecode/fraction_values.hpp -- EVERYTHING THE CHECKER AND THE WALKER ASK
// ABOUT A FRACTION, `satellite.variable.fraction` (1 6 20). The value itself is
// satellite_object/object_fraction.hpp's.
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

// A FRACTION LITERAL -- 1/3, 1.5/2 -- a number, a TOUCHING slash and a number
// (the author, 2026-09-16: "when you encounter number/number with NO space --
// that becomes a fraction"). expression.cpp has read the number before the slash
// as `top` and sends it here with `at` on the fraction_token; `at` is left past
// the number after it.
signed long long int fraction_literal(const std::string &top, const std::vector<std::bitset<16>> &row,
                                      std::size_t &at, Value &out, std::string &why);

// THE CHECKER, BEFORE ANYTHING RUNS: the value given to a name declared satellite.variable.fraction,
// `at` on the first code straight after the `=` -- in the declaration and in every
// later `name = ...`. `declared` is every name the checker knows and its type word.
signed long long int fraction_is_written_right(const std::vector<std::bitset<16>> &row, std::size_t at,
                                            const std::unordered_map<std::string, token::Code> &declared,
                                            std::string &why);

// THE CHECKER: the first method written on a name declared satellite.variable.fraction. `spelling`
// is `name.method`, for the sentence.
signed long long int fraction_method_check(token::Code method, const std::string &spelling, std::string &why);

// THE WALKER: a value arriving in a name declared `holds` -- ANY name, so this
// answers success untouched for every `holds` that is not its business. It may
// change `value` (a number given to a fraction name becomes a fraction).
signed long long int fraction_on_store(token::Code holds, Value &value, std::string &why);

// THE WALKER: a method on a value of this type. `slot` is the variable itself when
// the method is written straight on a name (a method that changes the value writes
// through it), and nullptr otherwise. `answered` false leaves the method to the
// conversions every type shares (.string, .number, .binary, .hex).
Value fraction_method(token::Code method, const Value &receiver, Value *slot, const std::vector<Value> &arguments,
                   bool had_parentheses, const std::string &name, ExpressionContext &context, bool &answered);

} // namespace satellite004
