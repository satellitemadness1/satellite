// satellite/bytecode/color_values.cpp -- what the checker and the walker ask about
// a color (color_values.hpp says why this is its own file).
//
// NOT BUILT YET: every answer here is what the language did before the file
// existed.

#include "color_values.hpp"

#include "../satellite_variable_number/number_conversions.hpp"

#include <utility>

namespace satellite004 {

signed long long int color_is_written_right(const std::vector<std::bitset<16>> &, std::size_t,
                                            const std::unordered_map<std::string, token::Code> &, std::string &)
{
    return success;
}

signed long long int color_method_check(token::Code, const std::string &, std::string &)
{
    return success;
}

signed long long int color_on_store(token::Code, Value &, std::string &)
{
    return success;
}

Value color_method(token::Code, const Value &, Value *, const std::vector<Value> &, bool, const std::string &,
                   ExpressionContext &, bool &answered)
{
    answered = false;
    return Value();
}

} // namespace satellite004
