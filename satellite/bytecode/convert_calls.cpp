// satellite/bytecode/convert_calls.cpp -- satellite.convert.number(x), .float(x) and .whole(x).
// convert_calls.hpp says what each answers and why the words have no library.

#include "convert_calls.hpp"

#include "word_codes.hpp"
#include "../machine/s_codes.hpp"
#include "../satellite_object/object_fraction.hpp"
#include "../satellite_variable_float/float_scaled.hpp"

#include <string>
#include <utility>

namespace satellite004 {
namespace {

constexpr token::Code kToNumber = word::fixed_code<1, 35, 1>;
constexpr token::Code kToFloat = word::fixed_code<1, 35, 2>;
constexpr token::Code kToWhole = word::fixed_code<1, 35, 3>;

token::Code code_in(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<token::Code>(row[at].to_ulong()) : 0;
}

// THE WHOLE NUMBER, CUT TOWARD ZERO -- satellite.convert.whole's, which is what `/` of two numbers was
// before 2026-10-12, so a program written for that keeps its answers (the author: "change the programming
// so they still work"). 7/2 is 3, -7/2 is -3, 2.75 is 2. Nothing is said: the cut is what was asked for.
bool cut_to_a_number(const Value &value, Value &out)
{
    if (value.is_number()) {
        out = value;
        return true;
    }
    if (const satellite_fraction *held = value.as_fraction()) {
        out = Value::of_number(fraction_cut_toward_zero(*held));
        return true;
    }
    if (const satellite_float *held = value.as_float()) {   // the whole side, signed
        out = Value::of_number(held->negative ? -held->whole : held->whole);
        return true;
    }
    return false;
}

// THE WHOLE NUMBER NEAREST IT, or nothing for a kind that is not converted yet: .5 and above is one
// more, below .5 is not (the author, 2026-10-12: ".500000000 is plus one .499999999 is not plus one"),
// and half away from zero on the minus side, as every float rounds (Q17b). `lost` is set when
// something after the point was dropped, for the warning.
bool rounded_to_a_number(const Value &value, Value &out, bool &lost)
{
    if (value.is_number()) {
        out = value;
        return true;
    }
    if (const satellite_fraction *held = value.as_fraction()) {
        out = Value::of_number(fraction_rounded(*held, lost));
        return true;
    }
    if (const satellite_float *held = value.as_float()) {
        // 2.75 is 3, 2.4999 is 2, -2.5 is -3: the digits after the point are `fraction` over 10^places,
        // and they are half or more exactly when twice them is not below 10^places.
        satellite_number whole = held->whole;
        if (!held->fraction.is_zero()) {
            lost = true;
            if (satellite_number::compare(held->fraction + held->fraction, ten_to_the(held->places)) >= 0)
                whole += satellite_number(1ull);
        }
        out = Value::of_number(held->negative ? -whole : whole);
        return true;
    }
    return false;
}

// THE FLOAT IT IS WORTH: a fraction's top number over its bottom one, divided as two floats are
// (object_float.cpp's float_divide), so convert.float(1 / 3) is 1.0 / 3 to the last place.
signed long long int made_a_float(const Value &value, Value &out, std::string &why)
{
    if (value.is_float()) {
        out = value;
        return success;
    }
    if (const satellite_number *held = value.as_number()) {
        out = Value::of_float(float_of_number(*held));
        return success;
    }
    if (const satellite_fraction *held = value.as_fraction())
        return Value::of_float(held->numerator).divide(Value::of_float(held->denominator), out, why);
    return not_built_yet;
}

} // namespace

bool is_convert_word(token::Code code)
{
    return code == kToNumber || code == kToFloat || code == kToWhole;
}

Value call_convert_word(token::Code code, const std::vector<std::bitset<16>> &row, std::size_t &at,
                        ExpressionContext &context)
{
    const bool to_number = code == kToNumber;
    const bool to_whole = code == kToWhole;
    const std::string spelled = to_number  ? "satellite.convert.number"
                                : to_whole ? "satellite.convert.whole"
                                           : "satellite.convert.float";
    const std::size_t open = at;
    ++at;                                                    // the (
    if (code_in(row, at) == token::right_parenthesis_token) {
        context.refuse(satl_line_not_understood, spelled + " takes one argument, and was given 0", open);
        return Value();
    }
    const Value value = to_number  ? evaluate_rounded_expression(row, at, context)
                        : to_whole ? evaluate_whole_expression(row, at, context)
                                   : evaluate_expression(row, at, context);
    if (context.code != success)
        return Value();
    if (code_in(row, at) != token::right_parenthesis_token) {
        context.refuse(satl_line_not_understood,
                       spelled + (code_in(row, at) == token::comma_token
                                      ? " takes one argument, and was given more"
                                      : " was given something it could not read to the end of -- check that every "
                                        "math sign has a space on both sides"),
                       open);
        return Value();
    }
    ++at;                                                    // the )

    Value answer;
    std::string why;
    signed long long int code_made = not_built_yet;
    if (to_number) {
        bool lost = false;
        code_made = rounded_to_a_number(value, answer, lost) ? success : not_built_yet;
        if (lost)   // S021, "With a warning" -- "But don't stop the interpreter" (the author, 2026-10-12)
            warn_number_rounded(
                context.state,
                [&] {
                    const satellite_fraction *fraction = value.as_fraction();
                    return fraction != nullptr ? fraction_written(*fraction) : float_written(*value.as_float());
                },
                answer.as_number()->to_text());
    } else if (to_whole)
        code_made = cut_to_a_number(value, answer) ? success : not_built_yet;
    else
        code_made = made_a_float(value, answer, why);
    if (code_made == success)
        return answer;
    if (why.empty())
        why = spelled + " was given " + value.kind_name() +
              ", and only a number, a fraction and a float are converted yet -- the rest of satellite.convert is "
              "built later";
    context.refuse(code_made, why, open);
    return Value();
}

} // namespace satellite004
