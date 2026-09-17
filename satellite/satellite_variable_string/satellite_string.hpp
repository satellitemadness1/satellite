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
// SIXTEEN BITS A CHARACTER, AND 40000 IS WIDE (the author, 2026-09-17): "strings
// have a wide character to store extra strings but 40000 is not a smile... its
// the 16-bit value for wide", "or a chinese character no". Every character is one
// char16_t -- except a character that needs more than sixteen bits, which is
// stored as THREE: 40000, then its number as one 32-bit integer in two char16_t
// (the author's D3.1, 2026-09-16: "32-bits only when we use the number 40000 as a
// 16-bit code"). This is the same wide_token the bytecode uses, and it means the
// same thing here.
//
//     "a😀"   a      40000   0x0001  0xF600
//              1     wide     the 32-bit number 0x1F600
//
// 40000 IS NEVER A CHARACTER. U+9C40 -- the CJK character whose Unicode number
// happens to be 40000 -- is stored wide as well (40000, 0x0000, 0x9C40), so a
// 40000 in a string only ever means "wide". Its CODE is still 0x9C40: what a
// character is does not change with how it is stored.
//
// THE FAST PATH IS KEPT, and it is the author's first request: size() counts
// CHARACTERS and at(n) is ONE index. A string counts its wide characters as they
// arrive, so size() is units minus two a wide character and is never a scan, and
// a string with none -- every ASCII and every 16-bit text -- indexes straight in
// with nothing to test but one counter. A string holding a wide character walks
// to the index instead: the slower path, as the author put it, and only for the
// strings that need it. This is not UTF-16's surrogate pairs, which were refused
// for making size() count units: here size() still counts characters.
//
// A WIDE CHARACTER'S LOW HALF CAN BE 40000 ITSELF (U+19C40 is 40000, 0x0001,
// 0x9C40), so nothing may scan the units for 40000 from anywhere but the start of
// a character. Everything in this class walks character by character, and
// units()/code_at_unit() are how code outside it does the same.
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
#include <utility>

namespace satellite004 {

class satellite_string {
public:
    // The 16-bit value for wide: the two units after it are one 32-bit character.
    static constexpr char16_t kWide = 40000;

    satellite_string() = default;

    // A MOVED-FROM STRING IS AN EMPTY ONE, its wide count with it. A defaulted move
    // copies wide_count_ and leaves it behind on a string whose units have gone, so
    // size() -- units minus two a wide character -- would wrap below zero and
    // code_at would walk past the end. Copies are the defaults.
    satellite_string(const satellite_string &) = default;
    satellite_string &operator=(const satellite_string &) = default;
    satellite_string(satellite_string &&from) noexcept
        : narrow16_(std::move(from.narrow16_)), wide_count_(from.wide_count_)
    {
        from.narrow16_.clear();
        from.wide_count_ = 0;
    }
    satellite_string &operator=(satellite_string &&from) noexcept
    {
        if (&from != this) {   // s = std::move(s) keeps s
            narrow16_ = std::move(from.narrow16_);
            wide_count_ = from.wide_count_;
            from.narrow16_.clear();
            from.wide_count_ = 0;
        }
        return *this;
    }

    // UTF-8 bytes -> a string. On failure `out` holds the characters before
    // bad_offset.
    static signed long long int from_utf8(const std::string &utf8, satellite_string &out, std::size_t &bad_offset);
    // A string -> UTF-8 bytes. Cannot fail for a string made by this class.
    std::string to_utf8() const;

    // Unicode number <-> code, and back. Any Unicode scalar value has a code.
    static char32_t code_of(char32_t unicode);
    static char32_t unicode_of(char32_t code);

    // Whether this string is on the 16-bit fast path: it holds no wide character.
    bool fast() const { return wide_count_ == 0; }
    // CHARACTERS, never units: a wide character is one.
    std::size_t size() const { return narrow16_.size() - 2 * wide_count_; }
    bool empty() const { return narrow16_.empty(); }

    // The code of the CHARACTER at `index`. Answers success, or
    // position_past_the_end (16).
    signed long long int code_at(std::size_t index, char32_t &code) const;
    // The same with no check: one index on the fast path, a walk on the slower one.
    char32_t code_at_unchecked(std::size_t index) const
    {
        if (wide_count_ == 0) [[likely]]
            return static_cast<char32_t>(narrow16_[index]);
        return code_at_walking(index);
    }

    // THE UNITS, for code that walks a string once instead of indexing it again and
    // again (find, removal). Start at unit 0 and step by `width`: every stop is the
    // start of a character, which is the only place 40000 means wide.
    std::size_t units() const { return narrow16_.size(); }
    const char16_t *unit_data() const { return narrow16_.data(); }
    char32_t code_at_unit(std::size_t unit, std::size_t &width) const
    {
        if (narrow16_[unit] != kWide) {
            width = 1;
            return static_cast<char32_t>(narrow16_[unit]);
        }
        width = 3;
        return (static_cast<char32_t>(narrow16_[unit + 1]) << 16) | static_cast<char32_t>(narrow16_[unit + 2]);
    }

    void append(const satellite_string &other);
    // Answers success, or string_error (4) for a code this type cannot hold: a
    // surrogate (0xD800-0xDFFF) or anything above 0x10FFFF. Nothing is appended
    // then. A code above 0xFFFF, or 0x9C40, is stored wide.
    signed long long int append_code(char32_t code);
    void clear();

    // Characters [start, end). Answers success, position_past_the_end (16) or
    // positions_backwards (17).
    signed long long int substring(std::size_t start, std::size_t end, satellite_string &out) const;

    // Code by code; a shorter string that is a prefix comes first. -1, 0 or 1.
    static int compare(const satellite_string &left, const satellite_string &right);
    friend bool operator==(const satellite_string &l, const satellite_string &r) { return compare(l, r) == 0; }
    friend bool operator!=(const satellite_string &l, const satellite_string &r) { return compare(l, r) != 0; }
    friend bool operator<(const satellite_string &l, const satellite_string &r) { return compare(l, r) < 0; }

private:
    // The unit a character starts at, walking from the front: the slower path.
    std::size_t unit_of(std::size_t character) const;
    char32_t code_at_walking(std::size_t index) const;

    std::u16string narrow16_;       // every character, two bytes -- or six for a wide one
    std::size_t wide_count_ = 0;    // how many of them are wide: size() is never a scan
};

} // namespace satellite004
