#pragma once
// satellite/satellite_variable_string/satellite_string.hpp -- satellite.variable.string.
//
// (the author, 2026-09-15) "we will make a 16-bit fast list, and a 32-bit slower
// number", and "we just check if it fits into the 16-bit fast path first thing
// we do".
//
// A SATELLITE STRING HOLDS CODES, NOT BYTES. Every character is one code:
//
//     0        NUL
//     1-26     a-z
//     27-52    A-Z
//     53-62    0-9
//     63-72    ! @ # $ % ^ & * ( )
//     73-94    - + _ = [ ] { } \ | ; : ' " , . < > / ? ` ~
//     95 96 97 space, tab, newline
//     98-127   the other 30 ASCII control characters, in ASCII order
//              (0x01-0x08, 0x0B-0x1F, 0x7F)
//     128 up   every other character: its Unicode number
//
// So 0-127 is every ASCII character exactly once, in the author's order, and
// comparing codes sorts strings in that order (the order 003 sorted in).
//
// SIXTEEN BITS, AND ONLY SIXTEEN (the author, 2026-09-16): "we are going to get
// rid of the 32-bit strings, and have 16-bit only". There was a char32_t path
// for characters above U+FFFF; it is gone. Every character is one char16_t, two
// bytes, and at(n) is one index with nothing to test first.
//
// WHAT THAT COSTS, SAID PLAINLY: a character above U+FFFF -- an emoji, a rare
// CJK character -- CANNOT BE HELD. from_utf8 refuses it with string_error (4) at
// the byte its sequence starts on, and append_code refuses its code. That is a
// refusal by name and never a silent mangling, which is the rule this type was
// built on. The alternative nobody chose was UTF-16's surrogate pairs, and the
// reason not to is that they break the property the author asked for first:
// size() counts CHARACTERS and at(n) is ONE index. A pair is one character in
// two units, so both would have started lying.
//
// Tokens (// || << >> :: ...) are never stored in a string: a string holds
// characters. Tokens belong to the numbered program.
//
// Every conversion is strict and answers a machine code: success, or
// string_error (4) with the byte offset of the first invalid UTF-8 sequence
// (no overlong forms, no surrogates, nothing above U+10FFFF -- the rules the
// 32-bit strings/satellite_string.cpp was checked against, 30,055 cases).

#include <cstddef>
#include <string>

namespace satellite004 {

class satellite_string {
public:
    satellite_string() = default;

    // UTF-8 bytes -> a string. On failure `out` holds the characters before
    // bad_offset.
    static signed long long int from_utf8(const std::string &utf8, satellite_string &out, std::size_t &bad_offset);
    // A string -> UTF-8 bytes. Cannot fail for a string made by this class.
    std::string to_utf8() const;

    // Unicode number <-> code, and back. Any Unicode scalar value has a code.
    static char32_t code_of(char32_t unicode);
    static char32_t unicode_of(char32_t code);

    bool fast() const { return true; }  // always: there is one path now
    std::size_t size() const { return narrow16_.size(); }
    bool empty() const { return size() == 0; }

    // The code at `index`. Answers success, or position_past_the_end (16).
    signed long long int code_at(std::size_t index, char32_t &code) const;
    char32_t code_at_unchecked(std::size_t index) const
    {
        return static_cast<char32_t>(narrow16_[index]);
    }

    void append(const satellite_string &other);
    // Answers success, or string_error (4) for a code this type cannot hold:
    // a surrogate (0xD800-0xDFFF), anything above 0x10FFFF, and -- since the
    // author's 16-bit-only ruling -- anything above 0xFFFF. Nothing is appended
    // then.
    signed long long int append_code(char32_t code);
    void clear();

    // Characters [start, end). Answers success, position_past_the_end (16) or
    // positions_backwards (17). The result is fast when it fits.
    signed long long int substring(std::size_t start, std::size_t end, satellite_string &out) const;

    // Code by code; a shorter string that is a prefix comes first. -1, 0 or 1.
    static int compare(const satellite_string &left, const satellite_string &right);
    friend bool operator==(const satellite_string &l, const satellite_string &r) { return compare(l, r) == 0; }
    friend bool operator!=(const satellite_string &l, const satellite_string &r) { return compare(l, r) != 0; }
    friend bool operator<(const satellite_string &l, const satellite_string &r) { return compare(l, r) < 0; }

private:
    std::u16string narrow16_;   // every character, two bytes, one index
};

} // namespace satellite004
