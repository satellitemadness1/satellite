// satellite/bytecode/scalar_calls.cpp -- what the other scalars answer, reached BY THEIR NUMBERS since
// 2026-10-11 (scalar_calls.hpp says which and why). Each method's own code is a file under
// satellite-numbers/words/ named by its number -- 1.6.11.12.cpp is a hex's .reverse() -- and this file is
// what they share: which method runs which number, run_scalar_number, which calls the one a number names,
// and the pieces several of them use.

#include "scalar_calls.hpp"

#include "function_table.hpp"
#include "word_codes.hpp"
#include "../satellite_variable_float/satellite_float.hpp"

#include <utility>

namespace satellite004 {

bool an_other_scalar(satelliteObject::Kind kind)
{
    switch (kind) {
    case satelliteObject::floating: case satelliteObject::hexadecimal: case satelliteObject::binary:
    case satelliteObject::color: case satelliteObject::fraction: case satelliteObject::percentage:
    case satelliteObject::boolean: case satelliteObject::time: return true;
    default: return false;
    }
}

// EVERY NUMBER BELOW IS IN words/words.tsv, OR THIS FILE DOES NOT COMPILE: a fixed_code of 0 would quietly
// send that method down the walker's refusals instead of to its number (string_calls.cpp says the same).
template <int Type, int... Last>
inline constexpr bool every_scalar_word_numbered = ((word::fixed_code<1, 6, Type, Last> != 0) && ...);
static_assert(every_scalar_word_numbered<10, 2, 3, 4, 5, 6>, "a float method's number is missing from words/words.tsv");
static_assert(every_scalar_word_numbered<11, 2, 4, 7, 8, 9, 10, 11, 12>,
              "a hex method's number is missing from words/words.tsv");
static_assert(every_scalar_word_numbered<5, 2, 5, 8, 9, 10, 11, 12, 13, 14>,
              "a binary method's number is missing from words/words.tsv");
static_assert(every_scalar_word_numbered<19, 1, 2, 3, 4, 5>, "a color method's number is missing from words/words.tsv");
static_assert(every_scalar_word_numbered<20, 1, 2, 3, 4>, "a fraction method's number is missing from words/words.tsv");
static_assert(every_scalar_word_numbered<16, 1, 2>, "a percentage method's number is missing from words/words.tsv");
static_assert(every_scalar_word_numbered<6, 1, 2>, "a bool method's number is missing from words/words.tsv");
static_assert(every_scalar_word_numbered<3, 1>, "a time method's number is missing from words/words.tsv");
static_assert(word::fixed_code<1, 6, 3, 2, 1> != 0, "a time's convert, 1 6 3 2 1, is missing from words/words.tsv");

// ONE METHOD, ONE NUMBER, read off words/words.tsv. `to_string`, `str` and `string` are one code by the time
// anything here sees them (the lexer folds a second spelling into the first), so where 003 has a row for
// each spelling the code runs the row strings run -- 1 6 10 2, satellite.variable.float.string -- and 1 6 10 1,
// satellite.variable.float.to_string, is that number's second name.
token::Code float_number_of(token::Code method)
{
    switch (method) {
    case token::to_string_token: return word::fixed_code<1, 6, 10, 2>;
    case token::to_number_token: return word::fixed_code<1, 6, 10, 3>;
    case token::to_binary_token: return word::fixed_code<1, 6, 10, 4>;       // numbered, and says it is not built
    case token::to_hexadecimal_token: return word::fixed_code<1, 6, 10, 5>;  // the same
    case token::reverse_token: return word::fixed_code<1, 6, 10, 6>;         // FLT-4, numbered and not built (his I)
    default: return 0;
    }
}

// 1 6 11 1 to_number, 3 to_string and 5 to_binary are the second names of 8, 7 and 9; 1 6 11 4, .digits, is
// 003's row with nothing built behind it -- reached since a number's words gave `digits` a method code, and told
// so before anything runs (program_check.cpp's scalar_word_not_built), as a binary's .width is, and by the run of a
// value only it can type -- and 1 6 11 6, hex(x), makes a hex and is no method. .add and
// .reverse() took the next free numbers, 11 and 12, the day a hex was numbered.
token::Code hexadecimal_number_of(token::Code method)
{
    switch (method) {
    case token::width_token: return word::fixed_code<1, 6, 11, 2>;
    case token::digits_token: return word::fixed_code<1, 6, 11, 4>;         // numbered, and says it is not built
    case token::to_string_token: return word::fixed_code<1, 6, 11, 7>;
    case token::to_number_token: return word::fixed_code<1, 6, 11, 8>;
    case token::to_binary_token: return word::fixed_code<1, 6, 11, 9>;
    case token::to_hexadecimal_token: return word::fixed_code<1, 6, 11, 10>;
    case token::add_token: return word::fixed_code<1, 6, 11, 11>;
    case token::reverse_token: return word::fixed_code<1, 6, 11, 12>;
    default: return 0;
    }
}

// 1 6 5 1 to_number and 4 as_number are the second names of 9, 3 to_string of 8, 6 to_hex of 11; 1 6 5 7,
// binary(x), makes a binary and is no method. .width, .digits (5, reached since a number's words gave `digits`
// a method code) and .clear are 003's rows with nothing built behind them: they run their numbers and are told
// so. .add and .reverse() took 13 and 14.
token::Code binary_number_of(token::Code method)
{
    switch (method) {
    case token::width_token: return word::fixed_code<1, 6, 5, 2>;
    case token::digits_token: return word::fixed_code<1, 6, 5, 5>;          // numbered, and says it is not built
    case token::to_string_token: return word::fixed_code<1, 6, 5, 8>;
    case token::to_number_token: return word::fixed_code<1, 6, 5, 9>;
    case token::to_binary_token: return word::fixed_code<1, 6, 5, 10>;
    case token::to_hexadecimal_token: return word::fixed_code<1, 6, 5, 11>;
    case token::clear_token: return word::fixed_code<1, 6, 5, 12>;
    case token::add_token: return word::fixed_code<1, 6, 5, 13>;
    case token::reverse_token: return word::fixed_code<1, 6, 5, 14>;
    default: return 0;
    }
}

// A COLOUR HAD NO ROWS; it took 1 6 19 1 to 5 for what it answered. .transparency is ONE row with two shapes,
// as the token is one: read bare it answers a number, given one it changes the name and answers the colour.
token::Code color_number_of(token::Code method)
{
    switch (method) {
    case token::transparency_token: return word::fixed_code<1, 6, 19, 1>;
    case token::to_string_token: return word::fixed_code<1, 6, 19, 2>;
    case token::to_number_token: return word::fixed_code<1, 6, 19, 3>;
    case token::to_binary_token: return word::fixed_code<1, 6, 19, 4>;
    case token::to_hexadecimal_token: return word::fixed_code<1, 6, 19, 5>;
    default: return 0;
    }
}

token::Code fraction_number_of(token::Code method)
{
    switch (method) {
    case token::numerator_token: return word::fixed_code<1, 6, 20, 1>;
    case token::denominator_token: return word::fixed_code<1, 6, 20, 2>;
    case token::to_string_token: return word::fixed_code<1, 6, 20, 3>;
    case token::reverse_token: return word::fixed_code<1, 6, 20, 4>;   // numbered and not built (his I, 2026-10-11)
    default: return 0;
    }
}

token::Code percentage_number_of(token::Code method)
{
    switch (method) {
    case token::to_string_token: return word::fixed_code<1, 6, 16, 1>;
    case token::add_token: return word::fixed_code<1, 6, 16, 2>;
    default: return 0;
    }
}

token::Code bool_number_of(token::Code method)
{
    switch (method) {
    case token::to_string_token: return word::fixed_code<1, 6, 6, 1>;
    case token::add_token: return word::fixed_code<1, 6, 6, 2>;   // + with text (his A, 2026-10-11)
    default: return 0;
    }
}

// .satellite.time.convert(unit) IS ONE WORD OF TWO PARTS (TIME-1, 2026-10-26), as a window's window.section is: its
// part, `time` -- satellite.variable.time.time, 1 6 3 2 -- runs no number, and is refused written alone before the
// period's rule is asked (time_calls.hpp's a_time_part).
token::Code time_number_of(token::Code method)
{
    switch (method) {
    case token::to_string_token: return word::fixed_code<1, 6, 3, 1>;
    case token::time_convert_token: return word::fixed_code<1, 6, 3, 2, 1>;
    default: return 0;
    }
}

Value run_scalar_number(token::Code method, token::Code number, const Value &receiver, Value *home,
                        const std::vector<Value> &arguments, bool had_parentheses, const std::string &name,
                        ExpressionContext &context)
{
    // THE NUMBER RUNS: table[code], one subtraction and one load (function_table.hpp), and the function its
    // file under satellite-numbers/words/ put in the row at start-up. A row with nothing behind it -- a
    // binary's .width and .clear, 003's words never built -- is said as that, in the sentence a string's is.
    const NumberRow *row = context.functions.method(number);
    if (row == nullptr || row->scenarios.method_call == nullptr) {
        context.refuse(not_built_yet, name + ".satellite." + token::method_name_of(method) + " is not built for " +
                                          receiver.kind_name() + " yet");
        return Value();
    }
    return row->scenarios.method_call(receiver, home, nullptr, arguments, had_parentheses, name, context);
}

namespace scalar_method_parts {

Value text_value(const std::string &written, const std::string &spelled, ExpressionContext &context)
{
    satellite_string text;
    std::size_t bad_offset = 0;
    const signed long long int made = satellite_string::from_utf8(written, text, bad_offset);
    if (made != success) {
        context.refuse(made, spelled + " could not be written as text");
        return Value();
    }
    return Value::of_string(std::move(text));
}

// A PART WITH NOTHING AFTER ITS POINT IS THE WHOLE NUMBER IT IS, as display shows it (the author, 2026-09-22:
// "display them as just the whole number"): 1/3's numerator is 1, and `f.satellite.numerator + 1` is 2, not
// 2.0. A part that has digits after its point is the float: 1.5/2's numerator is 1.5. If the author rules that
// a part is always the float, this is `return Value::of_float(part)` -- and then `f.satellite.numerator / 2`
// divides as a float does rather than as a whole number.
Value part_as_value(const satellite_float &part)
{
    if (part.fraction.is_zero())
        return Value::of_number(part.negative ? -part.whole : part.whole);
    return Value::of_float(part);
}

} // namespace scalar_method_parts

} // namespace satellite004
