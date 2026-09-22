// satellite/bytecode/float_values.cpp -- what the checker and the walker ask about
// a float (float_values.hpp says why this is its own file).
//
// NOT BUILT YET: every answer here is what the language did before the file
// existed.

#include "float_values.hpp"

#include "../satellite_variable_number/number_conversions.hpp"

#include <utility>

namespace satellite004 {

// TODAY'S ANSWER, KEPT: `12.34` is refused as it was before this file existed.
signed long long int float_literal(const std::string &digits, Value &out, std::string &why)
{
    satellite_number value;
    const signed long long int held = number_fast_path::from_token_text(digits, number_fast_path::kDecimal, value);
    if (held != success) {
        why = digits + " is not a number this can read";
        return held;
    }
    out = Value::of_number(std::move(value));
    return success;
}

signed long long int float_is_written_right(const std::vector<std::bitset<16>> &, std::size_t,
                                            const std::unordered_map<std::string, token::Code> &, std::string &)
{
    return success;
}

signed long long int float_method_check(token::Code, const std::string &, std::string &)
{
    return success;
}

signed long long int float_on_store(token::Code, Value &, std::string &)
{
    return success;
}

Value float_method(token::Code, const Value &, Value *, const std::vector<Value> &, bool, const std::string &,
                   ExpressionContext &, bool &answered)
{
    answered = false;
    return Value();
}

} // namespace satellite004
