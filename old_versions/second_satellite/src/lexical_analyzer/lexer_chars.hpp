#pragma once

// What a character IS, for the lexer -- the first half of DESIGN §5.
//
// WRITTEN AGAINST THE CODE TABLE, NEVER AGAINST C'S CTYPE (DESIGN §5.2). This
// is not a style preference and the failure is silent rather than loud: a space
// has no code-table entry at all, so it arrives as SAT_RAW_BASE + ' ' -- which
// is 0x8020, a number isspace() has never heard of and which compares equal to
// no char literal. A lexer that reaches for isspace() on a SatChar skips
// nothing and emits one Error token per space in the file.
//
// THE CODE TABLE SAYS WHAT A CHARACTER IS; THIS FILE SAYS WHAT ROLE IT PLAYS,
// and underscore is where the two disagree on purpose (DESIGN §5.1). '_' is
// punctuation in the table and an identifier character here, because under the
// naive rule `my_time` lexes as THREE tokens and every example in DESIGN
// breaks; the slicing example list_name[some_number_start:some_number_end]
// degrades from 6 tokens to 16. Named beside the table rather than written as a
// bare number, which is what SAT_UNDERSCORE is for.

#include "satellite_string/satellite_string.hpp"

namespace satellite {

// a..z and A..Z both, which is codes 1..52 -- one range because the table puts
// the two alphabets next to each other and the lexer never needs to tell them
// apart. Case matters to the language nowhere in §5.
inline bool is_letter(SatChar c)
{
    return c >= SAT_A && c < SAT_DIGIT_0;
}

inline bool is_digit(SatChar c)
{
    return c >= SAT_DIGIT_0 && c < SAT_PUNCT_BASE;
}

// A newline is a TOKEN and not merely a skip (DESIGN §6.2), so it is asked
// about separately from the rest of the whitespace.
inline bool is_newline(SatChar c)
{
    return c == SAT_RAW_BASE + '\n';
}

// All four live in the raw area, which is the whole point of §5.2.
inline bool is_space(SatChar c)
{
    return c == SAT_RAW_BASE + ' '  || c == SAT_RAW_BASE + '\t'
        || c == SAT_RAW_BASE + '\r' || is_newline(c);
}

// DESIGN §5.1. A word may begin with a letter or an underscore, and never with
// a digit -- `2x` is a Number followed by a Word, which is what makes the
// number rule below able to run second without ambiguity.
inline bool word_start(SatChar c)
{
    return is_letter(c) || c == SAT_UNDERSCORE;
}

inline bool word_cont(SatChar c)
{
    return is_letter(c) || is_digit(c) || c == SAT_UNDERSCORE;
}

// The digits of a Bits literal (DESIGN §8.5), asked over the DECODED byte
// rather than the SatChar: a literal's body has already been decoded to a
// std::string by the time its radix is being decided, and asking the same
// question in two alphabets is how the two answers drift.
//
// HEX ACCEPTS BOTH CASES, x00ff and x00FF alike, because the width is what
// carries meaning in §8.5 and case does not; the literal's own text is kept
// verbatim in the token, so unparse still prints back what was written.
inline bool is_binary_digit(char c)
{
    return c == '0' || c == '1';
}

inline bool is_hex_digit(char c)
{
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

inline bool is_digit_in_radix(unsigned radix, char c)
{
    return radix == 2 ? is_binary_digit(c) : is_hex_digit(c);
}

} // namespace satellite
