#pragma once
// satellite/satellite_variable_number/number_conversions.hpp -- THE CONVERSION
// FAST PATHS: a satellite_number into text, and text back into a satellite_number,
// in each of the three radixes a program can write.
//
// (the author, 2026-09-16) "we need to convert these satellite_numbers into
// strings with a function, create fast paths for conversion and tokens, and then
// wire the tokens into the interpreter."
//
// THE LEXER WRITES THREE TOKENS, AND TWO OF THEM ARE NUMBERS. A program can
// spell `34587`, `b1100` or `xFFAA`, and bytecode_registry.cpp turns those into
// number_token, binary_token and hexadecimal_token -- each carrying its digits as
// a counted payload, WITHOUT the leading b or x, which the lexer strips. The
// number and the hex go through `from_token_text` below, and the token code is
// what chooses the radix rather than the text being sniffed for a prefix.
//
// binary_token DOES NOT COME HERE ANY MORE (2026-09-16): a b literal is a
// satellite.variable.binary, its width kept, and expression.cpp builds it with
// satellite_binary_number::from_digits. Base 2 stays in this file because a
// NUMBER still converts to and from base-2 text (`.bin`).
//
// THE WAY BACK MATTERS AS MUCH AS THE WAY IN, and for a reason that is not
// symmetry: satellite.console.display's library takes a std::string or an
// `unsigned long long int` (number_row.hpp), and a satellite_number is neither.
// A number that fits one limb can go as the count; one that does not -- and the
// whole point of this type is that it does not have to -- can only reach the
// output as TEXT. So `to_text()` is not a convenience here, it is the only exact
// path for a large number, and display_text_of() is what the interpreter calls.
//
// NOTHING CONVERTS SILENTLY. Each function answers a machine code and leaves its
// output untouched when it refuses, so a program never receives a value that
// stands for text the interpreter could not read (DESIGN §1.1).

#include "satellite_number.hpp"
#include "../machine/machine_codes.hpp"

#include <string>

namespace satellite004 {
namespace number_fast_path {

inline constexpr unsigned int kDecimal = 10;
inline constexpr unsigned int kBinary = 2;
inline constexpr unsigned int kHexadecimal = 16;

// A number as text, in any of the three radixes. Decimal carries the '-';
// so do the other two (to_radix_text), so every radix round-trips through
// from_token_text below.
inline std::string to_text(const satellite_number &value, unsigned int radix = kDecimal)
{
    return radix == kDecimal ? value.to_text() : value.to_radix_text(radix);
}

// Text into a number. `bad_offset` is the first byte that cannot be part of one,
// exactly as satellite_number::from_text defines it, so a refusal can point at
// the character rather than repeat the whole literal back.
inline signed long long int from_text(const std::string &text, unsigned int radix,
                                      satellite_number &out, std::size_t &bad_offset)
{
    return radix == kDecimal ? satellite_number::from_text(text, out, bad_offset)
                             : satellite_number::from_radix_text(text, radix, out, bad_offset);
}

// A literal's payload, given the radix its token already decided. The digits
// arrive with no b or x on them -- the lexer strips those -- which is why this
// takes the radix as an argument and never looks at the text to find it.
inline signed long long int from_token_text(const std::string &digits, unsigned int radix, satellite_number &out)
{
    std::size_t bad_offset = 0;
    return from_text(digits, radix, out, bad_offset);
}

// WHAT display() IS GIVEN. A number that fits one limb and is not negative can
// go to the library's `count` scenario as an unsigned long long; anything else
// -- negative, or larger than one limb -- must go as TEXT or lose its value.
// This says which, and the interpreter branches on it rather than guessing.
inline bool fits_a_count(const satellite_number &value)
{
    return value.fits_one_limb() && !value.negative();
}

inline unsigned long long int as_count(const satellite_number &value)
{
    return value.limb(0);
}

} // namespace number_fast_path
} // namespace satellite004
