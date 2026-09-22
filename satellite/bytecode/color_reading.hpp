#pragma once
// satellite/bytecode/color_reading.hpp -- THE FEW RULES THE CHECKER AND THE WALKER
// BOTH READ A COLOUR BY, so the two halves (color_check.cpp before anything runs,
// color_values.cpp while it runs) cannot come to disagree about what `ff00aa` is.
// Nothing outside the colour's own two files includes this.

#include "program_walk.hpp"
#include "token_codes.hpp"

#include <string>

namespace satellite004 {
namespace color_reading {

// THE ONE SENTENCE A WIDTH IS REFUSED WITH, the example in the author's own shape.
inline constexpr const char *kSixDigits = "a color is exactly six hex digits, like x00FF00";

// WHAT A COLOR HAS, said whole in every refusal of a method it has not got, so a
// person reads the answer where they read the question.
inline constexpr const char *kWhatAColorHas = "a color has .transparency, .number, .string, .hex and .binary";

// THE AUTHOR'S "0 - 99", both ends in, said the same way from his two ways of
// writing one -- the checker's and the walker's -- so one rule is one sentence.
inline constexpr const char *kWhatATransparencyIs =
    "a transparency is a whole number from 0 (solid) to 99 (almost clear)";

inline bool a_hex_digit(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }

inline bool all_hex_digits(const std::string &text)
{
    if (text.empty()) return false;
    for (const char c : text)
        if (!a_hex_digit(c)) return false;
    return true;
}

inline bool all_digits(const std::string &text)
{
    if (text.empty()) return false;
    for (const char c : text)
        if (c < '0' || c > '9') return false;
    return true;
}

// "1 hex digit", "5 hex digits".
inline std::string how_many_digits(std::size_t count)
{
    return std::to_string(count) + (count == 1 ? " hex digit" : " hex digits");
}

// WHERE A COLOUR'S VALUE STOPS: the line's end, or the comma before its
// transparency. A value written without its x is read as six digits only when it
// stands alone up to one of these -- `c = 000000 + 1` is a sum, and a sum is
// worked out as one.
inline bool the_value_ends(token::Code code)
{
    return code == token::line_end_token || code == token::comment_token || code == token::end_of_file_token ||
           code == token::comma_token;
}

} // namespace color_reading
} // namespace satellite004
