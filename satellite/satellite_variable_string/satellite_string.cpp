// satellite/satellite_variable_string/satellite_string.cpp -- satellite.variable.string:
// sixteen bits a character, with the author's table (character_table.hpp).
// satellite_string.hpp is the contract; this file keeps it.
//
// SIXTEEN BITS AND ONLY SIXTEEN (the author, 2026-09-16): "we are going to get
// rid of the 32-bit strings, and have 16-bit only". There was a char32_t path
// that a string moved onto the moment it met a character above U+FFFF, and every
// function in this file had two halves because of it. It is gone, and so are the
// two halves: one storage, one loop, and at(n) is one index with nothing to test.
//
// A CHARACTER ABOVE U+FFFF IS REFUSED BY NAME, never mangled. from_utf8 answers
// string_error (4) at the byte its sequence starts on -- the same refusal, at the
// same offset, that an invalid sequence gets. The decoder already stopped there
// to widen; now that stop is the answer.
//
// DECODING IS ONE PASS. from_utf8 decodes straight into char16_t. The rules are
// the Unicode standard's Table 3-7, written out per sequence length; every
// refusal is string_error at the byte where the bad sequence STARTS
// (check_strings16.py holds it to Python).
//
// SPEED: conversion_loops.hpp says how, string_race.cpp measures it.
//
// MEMORY. A decoded string keeps the capacity it was sized to -- one character a
// byte -- so text of 2- and 3-byte characters holds up to 2 or 3 times what it
// uses until it is changed or dropped. That is the price of one pass.

#include "satellite_string.hpp"

#include "character_table.hpp"
#include "conversion_loops.hpp"

#include "../machine/machine_codes.hpp"

#include <string>

namespace satellite004 {

namespace {

using character_table::ascii_of_code;
using character_table::code_of_ascii;
using conversion_loops::decode;
using conversion_loops::encode;
using conversion_loops::stopped;

// What this type can hold: not a surrogate, not above 0x10FFFF, and -- the
// author's ruling -- not above 0xFFFF either.
bool is_the_code_of_a_character(char32_t code)
{
    return code <= 0xFFFF && (code < 0xD800 || code > 0xDFFF);
}

} // namespace

char32_t satellite_string::code_of(char32_t unicode)
{
    return unicode < 128 ? code_of_ascii[unicode] : unicode;
}

char32_t satellite_string::unicode_of(char32_t code)
{
    return code < 128 ? ascii_of_code[code] : code;
}

signed long long int satellite_string::from_utf8(const std::string &utf8, satellite_string &out, std::size_t &bad_offset)
{
    const std::size_t size = utf8.size();
    const unsigned char *const bytes = reinterpret_cast<const unsigned char *>(utf8.data());
    std::size_t at = 0;
    stopped stop = stopped::at_the_end;
    overwrite_string(out.narrow16_, size, [&](char16_t *const first, std::size_t) {
        char16_t *write = first;
        stop = decode<char16_t, true>(bytes, size, at, write);
        return static_cast<std::size_t>(write - first);
    });

    if (stop == stopped::at_the_end)
        return success;

    // A CHARACTER ABOVE U+FFFF IS NOW A REFUSAL AND NOT A REASON TO WIDEN. The
    // decoder stops at the same place it always did; what changed is the answer.
    // `out` holds the characters before it, which is what from_utf8 has always
    // promised for a refusal.
    bad_offset = at;
    return string_error;
}

std::string satellite_string::to_utf8() const
{
    return encode(narrow16_.data(), narrow16_.size());
}

signed long long int satellite_string::code_at(std::size_t index, char32_t &code) const
{
    if (index >= size())
        return position_past_the_end;
    code = code_at_unchecked(index);
    return success;
}

void satellite_string::append(const satellite_string &other)
{
    narrow16_.append(other.narrow16_);   // safe when `other` is *this
}

signed long long int satellite_string::append_code(char32_t code)
{
    if (!is_the_code_of_a_character(code))
        return string_error;
    narrow16_.push_back(static_cast<char16_t>(code));
    return success;
}

void satellite_string::clear()
{
    narrow16_.clear();
}

signed long long int satellite_string::substring(std::size_t start, std::size_t end, satellite_string &out) const
{
    if (start > end)                    // 003's order: backwards first, then past the end
        return positions_backwards;
    if (end > size())
        return position_past_the_end;
    if (&out == this) {
        out.narrow16_.resize(end);
        out.narrow16_.erase(0, start);
        return success;
    }
    out.narrow16_.assign(narrow16_, start, end - start);
    return success;
}

int satellite_string::compare(const satellite_string &left, const satellite_string &right)
{
    const std::size_t left_size = left.size(), right_size = right.size();
    const std::size_t common = left_size < right_size ? left_size : right_size;
    const int order = std::char_traits<char16_t>::compare(left.narrow16_.data(), right.narrow16_.data(), common);
    if (order != 0)
        return order < 0 ? -1 : 1;
    return left_size < right_size ? -1 : left_size > right_size ? 1 : 0;
}

} // namespace satellite004
