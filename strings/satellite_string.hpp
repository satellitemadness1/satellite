#pragma once
// satellite-004/strings/satellite_string.hpp -- a satellite string is 32 bits a
// character: one char32_t per Unicode code point (UTF-32).
//
// WHY 32 BITS (the author, 2026-09-14): every character of every language is
// the same width, so a string's n-th character is one array index, and a word
// list in any language is just more characters. Unicode tops out at U+10FFFF
// (1,114,112 code points), which fits in 21 bits; 32 covers all of it with room.
//
// THE THREE FORMS AND WHERE EACH LIVES:
//   std::string     UTF-8 bytes -- what the .satl source file and the terminal hold
//   std::u32string  char32_t    -- what a satellite_string32 IS while running
//   bits text       "0100..."   -- what a .sati file holds, 32 '0'/'1' per character
//
// EVERY CONVERSION IS STRICT and answers a machine code: success, or
// string_error (4) with the byte offset of the first thing that is not valid.
// Accepting a malformed byte silently is how look-alike names and injected bytes
// get into a program (DESIGN §9), so nothing is ever "repaired".
//
// Rules, from the Unicode standard (and checked against Python's decoder):
//   - no overlong forms (C0 80 for NUL), no surrogates U+D800..U+DFFF,
//     nothing above U+10FFFF, no stray or missing continuation bytes.
//
// Written 2026-09-14 for PLAN M3.

#include <string>

namespace satellite004 {

// UTF-8 bytes -> char32_t. On failure `bad_offset` is the byte where the first
// invalid sequence starts, and `out` holds the characters before it.
signed long long int utf8_to_char32(const std::string &utf8, std::u32string &out, size_t &bad_offset);

// char32_t -> UTF-8 bytes. On failure `bad_offset` is the index of the first
// character that is not a Unicode scalar value.
signed long long int char32_to_utf8(const std::u32string &text, std::string &out, size_t &bad_offset);

// char32_t -> the .sati bit text: 32 '0'/'1' per character, most significant bit first.
void char32_to_bits(const std::u32string &text, std::string &bits);

// .sati bit text -> char32_t. Fails when the length is not a multiple of 32, a
// character is not '0' or '1', or a 32-bit value is not a Unicode scalar value.
// `bad_offset` is the character position in `bits` where the problem is.
signed long long int bits_to_char32(const std::string &bits, std::u32string &out, size_t &bad_offset);

// The author's bits_to_cxx_str: .sati bit text -> the UTF-8 std::string a
// library takes. Done once when a .sati is loaded, never per library call.
signed long long int bits_to_cxx_str(const std::string &bits, std::string &out, size_t &bad_offset);

} // namespace satellite004
