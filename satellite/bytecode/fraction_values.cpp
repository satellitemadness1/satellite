// satellite/bytecode/fraction_values.cpp -- what the checker and the walker ask about
// a fraction (fraction_values.hpp says why this is its own file). What a fraction
// DOES is satellite_object/object_fraction.cpp's.
//
// (the author, 2026-09-22) "let's just start with satellite.variable.fraction
// my_number = satellite_number1/satellite_number2 simply two numbers that are tied
// together, and when the user declares them, we will automatically make them
// floats with decimal points, but display them as just the whole number".

#include "fraction_values.hpp"

#include "float_values.hpp"
#include "word_codes.hpp"
#include "../satellite_object/object_fraction.hpp"

#include <utility>

namespace satellite004 {
namespace {

token::Code code_in(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<token::Code>(row[at].to_ulong()) : 0;
}

// ONE OF A FRACTION'S NUMBERS AS A PROGRAM GETS IT BACK -- .numerator and
// .denominator. A part with nothing after its point is the whole NUMBER it is, as
// display shows it ("display them as just the whole number"): 1/3's numerator is
// 1, and `f.numerator + 1` is 2, not 2.0. A part that has digits after its point
// is the float: 1.5/2's numerator is 1.5. If the author rules that a part is
// always the float, this is `return Value::of_float(part)` -- and then
// `f.numerator / 2` divides as a float does rather than as a whole number.
Value part_as_value(const satellite_float &part)
{
    if (part.fraction.is_zero())
        return Value::of_number(part.negative ? -part.whole : part.whole);
    return Value::of_float(part);
}

} // namespace

// EACH NUMBER IS READ AS THE FLOAT IT IS -- "we will automatically make them
// floats with decimal points" -- through float_literal, so `1.5/2` holds 1.5 and 2
// exactly as `1.5` and `2.0` are held, rounded by the same rows. The lexer makes a
// fraction_token only with a digit pressed against both sides of the slash, so the
// code after it is a number's; anything else there (1/50%) is named.
signed long long int fraction_literal(const std::string &top, const std::vector<std::bitset<16>> &row,
                                      std::size_t &at, Value &out, std::string &why)
{
    ++at;                                                   // the touching slash
    if (code_in(row, at) != token::number_token) {
        why = top + "/ is not followed by a number written out -- a fraction is two numbers and a touching "
                    "slash, like 1/3";
        return satl_line_not_understood;
    }
    const std::string bottom = text_at(row, at);
    const std::string written = top + "/" + bottom;
    // 1/2/3 IS TWO SLASHES, and a fraction has one. Read left to right it would be
    // a fraction divided by 3, which is fraction arithmetic -- so it is named here,
    // where the second slash is, rather than as an operator it is not.
    if (code_in(row, at) == token::fraction_token) {
        std::size_t k = at + 1;
        const std::string more = code_in(row, k) == token::number_token ? text_at(row, k) : std::string();
        why = written + "/" + more + " has two touching slashes, and a fraction has one: 1/3";
        return satl_line_not_understood;
    }
    Value above, below;
    if (const signed long long int read = float_literal(top, above, why); read != success)
        return read;
    if (const signed long long int read = float_literal(bottom, below, why); read != success)
        return read;
    const satellite_float &under = *below.as_float();
    if (under.whole.is_zero() && under.fraction.is_zero()) {
        why = written + ": a fraction's bottom number cannot be 0";
        return division_by_zero;
    }
    out = Value::of_fraction(satellite_fraction{*above.as_float(), under});
    return success;
}

// A SPACED SLASH IN A FRACTION'S VALUE IS REFUSED BEFORE ANYTHING RUNS.
// `satellite.variable.fraction f = 1 / 3` is whole-number division -- 0 -- and
// would be kept as 0/1 without a word, when the person plainly meant 1/3. The
// author's rule is that a touching slash is the fraction (2026-09-16), so the
// answer names the spelling that is: "a fraction is written with a touching
// slash: 1/3".
//
// THE WHOLE VALUE IS READ, NOT ONLY ITS FIRST CODE, because `f = n / 2` and
// `f = (a + b) / 2` are the same mistake. What is inside a call's brackets or an
// index is not: `f = halves[n / 2]` picks a fraction, and `f = pick(n / 2)` is
// whatever pick answers. A bracket that only groups -- `(1 / 3)` -- hides nothing.
// If the author rules the other way, this answers success and a spaced slash
// stores its whole-number answer over 1.
signed long long int fraction_is_written_right(const std::vector<std::bitset<16>> &row, std::size_t at,
                                            const std::unordered_map<std::string, token::Code> &,
                                            std::string &why)
{
    std::vector<bool> opened;         // each open bracket: true when it is a call's or an index's
    std::size_t inside = 0;           // how many of those are open
    token::Code previous = 0;
    std::string before;               // the number written just before, if the last code was one
    while (at < row.size()) {
        const token::Code code = code_in(row, at);
        if (code == token::line_end_token || code == token::end_of_file_token)
            break;
        if (token::carries_a_count(code)) {
            const std::string text = text_at(row, at);
            before = code == token::number_token ? text : std::string();
            previous = code;
            continue;
        }
        if (code == token::left_parenthesis_token || code == token::left_square_bracket_token ||
            code == token::left_brace_token) {
            const bool a_call = code != token::left_parenthesis_token || previous == token::name_token ||
                                previous == token::right_parenthesis_token ||
                                previous == token::right_square_bracket_token || word::is_word_code(previous) ||
                                token::is_method_code(previous);
            opened.push_back(a_call);
            if (a_call) ++inside;
        } else if (code == token::right_parenthesis_token || code == token::right_square_bracket_token ||
                   code == token::right_brace_token) {
            if (!opened.empty()) {
                if (opened.back()) --inside;
                opened.pop_back();
            }
        } else if (code == token::divide_token && inside == 0) {
            std::size_t k = at + 1;
            const std::string after = code_in(row, k) == token::number_token ? text_at(row, k) : std::string();
            const std::string spaced = before.empty() || after.empty() ? "a / with a space on both sides"
                                                                      : before + " / " + after;
            why = spaced + " is whole-number division -- a fraction is written with a touching slash: " +
                  (before.empty() || after.empty() ? std::string("1/3") : before + "/" + after);
            return types_do_not_meet;
        }
        before.clear();
        previous = code;
        ++at;
    }
    return success;
}

// WHAT IS BUILT, AND NOTHING ELSE. This answer is final for a fraction name
// (program_check.cpp's method_right_for), so .number, .binary, .hex and every
// other method are refused by name before a line runs.
signed long long int fraction_method_check(token::Code method, const std::string &spelling, std::string &why)
{
    if (method == token::numerator_token || method == token::denominator_token || method == token::to_string_token)
        return success;
    why = spelling + " is not built for satellite.variable.fraction yet -- a fraction has .numerator, "
                     ".denominator and .string so far";
    return not_built_yet;
}

// A PLAIN NUMBER GIVEN TO A FRACTION NAME IS THAT NUMBER OVER 1: `f = 3` holds 3/1,
// which is what 3 is as a fraction and what display then shows. Any other kind is
// refused by value_fits in the sentence every declared type uses -- a float
// meeting a fraction is the finale, and `f = 0.5` "holds a float". If the author
// rules that a number given to a fraction name is refused, this line goes.
signed long long int fraction_on_store(token::Code holds, Value &value, std::string &)
{
    if (holds == word::code_of(1, 6, 20) && value.is_number())
        value = Value::of_fraction(fraction_of_number(*value.as_number()));
    return success;
}

// A FRACTION'S METHODS ARE ITS TWO NUMBERS AND ITS TEXT, answered here, and every
// other method is refused here in the fraction's own words rather than left to a
// conversion every type shares.
Value fraction_method(token::Code method, const Value &receiver, Value *, const std::vector<Value> &arguments,
                   bool, const std::string &name, ExpressionContext &context, bool &answered)
{
    answered = true;
    const satellite_fraction &value = *receiver.as_fraction();
    const std::string spelling = name + "." + token::method_name_of(method);
    std::string why;
    if (fraction_method_check(method, spelling, why) != success) {
        context.refuse(not_built_yet, why);
        return Value();
    }
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood, spelling + " takes no argument");
        return Value();
    }
    if (method == token::numerator_token)
        return part_as_value(value.numerator);
    if (method == token::denominator_token)
        return part_as_value(value.denominator);
    satellite_string text;
    fraction_to_string(value, text, why);
    return Value::of_string(std::move(text));
}

} // namespace satellite004
