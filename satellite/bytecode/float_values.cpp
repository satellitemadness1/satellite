// satellite/bytecode/float_values.cpp -- what the checker and the walker ask about
// a float (float_values.hpp says why this is its own file). What a float DOES is
// satellite_object/object_float.cpp's; how it is worked out is
// satellite_variable_float/float_scaled.cpp's.

#include "float_values.hpp"

#include "word_codes.hpp"
#include "../arguments/arguments.hpp"
#include "../satellite_object/object_float.hpp"
#include "../satellite_variable_float/float_precision.hpp"
#include "../satellite_variable_float/float_scaled.hpp"
#include "../satellite_variable_number/number_conversions.hpp"

#include <climits>
#include <utility>

namespace satellite004 {
namespace {

// A ROW AS A COUNT OF DIGITS. A row past one limb is more digits than any
// machine can hold, so it means every digit there is -- the same thing, and not
// a cap: gather_config has already refused a row below 1.
unsigned long long int digits_of_row(const Arguments &arguments, const char *name, unsigned long long int fallback)
{
    if (arguments.find(name) == nullptr)
        return fallback;
    const satellite_number &row = arguments.number(name);
    return row.fits_one_limb() ? row.limb(0) : ULLONG_MAX;
}

} // namespace

void float_precision_from(const Arguments &arguments)
{
    float_precision_in_use.whole = digits_of_row(arguments, "arguments.float.whole", 4096);
    float_precision_in_use.decimal = digits_of_row(arguments, "arguments.float.decimal", 128);
    float_precision_in_use.shown = digits_of_row(arguments, "arguments.infinity_display", 32);
}

// 12.34 IS THE WHOLE SIDE, THE POINT, AND THE FRACTION SIDE -- read as 1234 at 2
// places and made through float_from_scaled, so a literal with more places than
// arguments.float.decimal is rounded exactly as an answer would be, and 12.50 is
// held as 12.5. The lexer joins a point into a number only when a digit follows
// it, so `12.` and `.5` never arrive here; `1.2.3` does, and is refused by name.
signed long long int float_literal(const std::string &digits, Value &out, std::string &why)
{
    const std::string::size_type point = digits.find('.');
    if (point != std::string::npos && digits.find('.', point + 1) != std::string::npos) {
        why = digits + " has more than one point, and a float is written with one: 12.34";
        return int_error;
    }
    std::string bare = digits;
    if (point != std::string::npos)
        bare.erase(point, 1);
    satellite_number scaled;
    const signed long long int held = number_fast_path::from_token_text(bare, number_fast_path::kDecimal, scaled);
    if (held != success) {
        why = digits + " is not a float this can read";
        return held;
    }
    const unsigned long long int places = point == std::string::npos ? 0 : digits.size() - point - 1;
    out = Value::of_float(float_from_scaled(std::move(scaled), places));
    return success;
}

// ANYTHING MAY BE GIVEN TO A FLOAT NAME AS FAR AS THE CHECKER CAN SEE. What an
// expression is worth is a run-time fact, and the store below turns a plain
// number into a float; a string or a list is refused there by value_fits, in the
// sentence every other declared type uses.
signed long long int float_is_written_right(const std::vector<std::bitset<16>> &, std::size_t,
                                            const std::unordered_map<std::string, token::Code> &, std::string &)
{
    return success;
}

// THE FOUR CONVERSIONS EVERY NUMBER-LIKE TYPE SHARES, and nothing else yet. This
// answer is final for a float name (program_check.cpp's method_right_for), so a
// method not listed here is refused before a line runs. .binary and .hex pass the
// checker as every type's conversions do, and are refused where they are reached
// -- they wait for the finale.
signed long long int float_method_check(token::Code method, const std::string &spelling, std::string &why)
{
    if (method == token::to_string_token || method == token::to_number_token ||
        method == token::to_binary_token || method == token::to_hexadecimal_token)
        return success;
    why = spelling + " is not built for satellite.variable.float yet -- a float has .string and .number so far";
    return not_built_yet;
}

// A PLAIN NUMBER GIVEN TO A FLOAT NAME BECOMES A FLOAT: `satellite.variable.float f
// = 3` holds 3.0, and `f = f * 2` after it stays a float whatever f is multiplied
// by. The other way is refused, by value_fits: a float given to a number name
// "holds a float", because turning 2.5 into 2 or 3 would be a guess.
signed long long int float_on_store(token::Code holds, Value &value, std::string &)
{
    if (holds == word::code_of(1, 6, 10) && value.is_number())
        value = Value::of_float(float_of_number(*value.as_number()));
    return success;
}

// A FLOAT'S METHODS ARE THE CONVERSIONS, answered here rather than by the shared
// ones, so the sentence a refusal carries -- `12.5 is not a whole number` -- reaches
// the person instead of "there is no conversion from that".
Value float_method(token::Code method, const Value &receiver, Value *, const std::vector<Value> &arguments,
                   bool, const std::string &name, ExpressionContext &context, bool &answered)
{
    answered = true;
    const satellite_float &value = *receiver.as_float();
    const std::string spelling = name + "." + token::method_name_of(method);
    std::string why;
    if (!arguments.empty() &&
        (method == token::to_string_token || method == token::to_number_token ||
         method == token::to_binary_token || method == token::to_hexadecimal_token)) {
        context.refuse(satl_line_not_understood, spelling + " is a conversion and takes no argument");
        return Value();
    }
    if (method == token::to_string_token) {
        satellite_string text;
        float_to_string(value, text, why);
        return Value::of_string(std::move(text));
    }
    signed long long int code = not_built_yet;
    if (method == token::to_number_token) {
        satellite_number whole;
        code = float_to_number(value, whole, why);
        if (code == success)
            return Value::of_number(std::move(whole));
        why = spelling + ": " + why;
    } else if (method == token::to_binary_token || method == token::to_hexadecimal_token) {
        satellite_string ignored;
        code = method == token::to_binary_token ? float_to_binary(value, ignored, why)
                                                : float_to_hexadecimal(value, ignored, why);
        why = spelling + ": " + why;
    } else {
        float_method_check(method, spelling, why);
    }
    context.refuse(code, why);
    return Value();
}

} // namespace satellite004
