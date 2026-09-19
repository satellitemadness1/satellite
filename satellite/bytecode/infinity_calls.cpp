// satellite/bytecode/infinity_calls.cpp -- satellite.infinity() and what the
// infinity's own methods say until they are built. infinity_calls.hpp says why the
// word has no library.

#include "infinity_calls.hpp"

#include "word_codes.hpp"
#include "../arguments/arguments.hpp"

namespace satellite004 {

bool is_infinity_word(token::Code code)
{
    return code == word::code_of(1, 26) || code == word::code_of(1, 26, 0);
}

std::string infinity_word_not_built(token::Code code, std::size_t given)
{
    if (!is_infinity_word(code) || given == 0)
        return "";
    return "satellite.infinity(x) -- infinity to the power of x -- is not built yet (SATELLITE_INFINITY.md "
           "INF-5); satellite.infinity() is";
}

Value call_infinity_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context)
{
    const std::string refused = infinity_word_not_built(code, arguments.size());
    if (!refused.empty()) {
        context.refuse(not_built_yet, refused);
        return Value();
    }
    // THE NINES WIDTH IS THE ROW, READ WHEN THE INFINITY IS MADE (Part 9): 128 unless
    // satellite_config.hpp says otherwise, and gather_config() has already refused a
    // row that is not a count of at least one digit. No arguments behind the state is
    // a caller with no config at all, and it gets the author's own default.
    static const satellite_number kAuthorsWidth(128ull);
    const Arguments *config = context.state.arguments;
    const satellite_number &width =
        config != nullptr && config->find("arguments.infinity") != nullptr ? config->number("arguments.infinity")
                                                                            : kAuthorsWidth;
    return Value::of_infinity(satellite_infinity::infinity(width));
}

std::string infinity_method_not_built(token::Code method)
{
    switch (method) {
    case token::power_of_token:
        return "is not built yet -- SATELLITE_INFINITY.md INF-4 builds a whole power, and INF-5 an infinite one";
    case token::resize_token:
    case token::nines_token:
        return "is not built yet -- SATELLITE_INFINITY.md INF-7 builds .resize and .nines";
    default:
        return "";
    }
}

} // namespace satellite004
