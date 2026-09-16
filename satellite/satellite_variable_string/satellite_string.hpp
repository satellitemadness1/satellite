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
// THE TWO PATHS. When a string is made, the first thing checked is whether every
// code fits in 16 bits. If so it is held as char16_t -- the fast path: two bytes
// a character, and at(n) is one index. A string holding any character above
// U+FFFF (an emoji, a rare CJK character) is held as char32_t -- the slower
// path, four bytes a character, still one index. A string that no longer holds
// such a character after an operation goes back to the fast path.
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

    bool fast() const { return !wide_; } // true: 16 bits a character
    std::size_t size() const { return wide_ ? wide32_.size() : narrow16_.size(); }
    bool empty() const { return size() == 0; }

    // The code at `index`. Answers success, or position_past_the_end (16).
    signed long long int code_at(std::size_t index, char32_t &code) const;
    char32_t code_at_unchecked(std::size_t index) const
    {
        return wide_ ? wide32_[index] : static_cast<char32_t>(narrow16_[index]);
    }

    void append(const satellite_string &other);   // goes to 32 bits only if `other` holds a wide character
    // Answers success, or string_error (4) for a code that is no character's
    // code (a surrogate 0xD800-0xDFFF, or above 0x10FFFF); nothing is appended then.
    // (Changed from void, 2026-09-15: a void function could not refuse, and a
    // stored non-character would make to_utf8's "cannot fail" untrue.)
    signed long long int append_code(char32_t code);
    void clear();                                 // empty and fast; wide storage is released

    // Characters [start, end). Answers success, position_past_the_end (16) or
    // positions_backwards (17). The result is fast when it fits.
    signed long long int substring(std::size_t start, std::size_t end, satellite_string &out) const;

    // Code by code; a shorter string that is a prefix comes first. -1, 0 or 1.
    static int compare(const satellite_string &left, const satellite_string &right);
    friend bool operator==(const satellite_string &l, const satellite_string &r) { return compare(l, r) == 0; }
    friend bool operator!=(const satellite_string &l, const satellite_string &r) { return compare(l, r) != 0; }
    friend bool operator<(const satellite_string &l, const satellite_string &r) { return compare(l, r) < 0; }

private:
    // THE RULE every function keeps: wide_ is true exactly when some code is
    // above 0xFFFF, and the storage of the path not in use is empty and holds no
    // memory (so a string never pays for both widths).
    bool wide_ = false;
    std::u16string narrow16_; // the fast path
    std::u32string wide32_;   // the slower path; narrow16_ is empty while wide_

    void become_wide(std::size_t room_for_more); // narrow16_ -> wide32_, keeping every code
};

} // namespace satellite004
