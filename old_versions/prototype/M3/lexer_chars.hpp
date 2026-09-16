#pragma once

// Character classification for satellite source text -- part of the lexer prototype.
//
// Written against the SatChar code table (DESIGN §5.2).
// Space, tab, newline live in the raw area.
// Underscore is an identifier character (DESIGN §5.1).

#include "satellite_string/satellite_string.hpp"

namespace satellite {

inline bool is_letter(SatChar c)
{
    return c >= SAT_A && c < SAT_DIGIT_0;
}

inline bool is_digit(SatChar c)
{
    return c >= SAT_DIGIT_0 && c < SAT_PUNCT_BASE;
}

inline bool is_newline(SatChar c)
{
    return c == SAT_RAW_BASE + '\n';
}

inline bool is_space(SatChar c)
{
    return c == SAT_RAW_BASE + ' '  || c == SAT_RAW_BASE + '\t'
        || c == SAT_RAW_BASE + '\r' || is_newline(c);
}

inline bool word_start(SatChar c)
{
    return is_letter(c) || c == SAT_UNDERSCORE;
}

inline bool word_cont(SatChar c)
{
    return is_letter(c) || is_digit(c) || c == SAT_UNDERSCORE;
}

inline bool is_bin_digit(char c)
{
    return c == '0' || c == '1';
}

inline bool is_hex_digit(char c)
{
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

inline bool bits_valid_digit(unsigned radix, char c)
{
    return radix == 2 ? is_bin_digit(c) : is_hex_digit(c);
}

} // namespace satellite

