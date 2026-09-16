// satellite/satellite_variable_string/satellite_string.cpp -- satellite.variable.string:
// the 16-bit fast path and the 32-bit slower path, with the author's table
// (character_table.hpp). satellite_string.hpp is the contract; this file keeps it.
//
// DECODING IS ONE PASS in the common case. from_utf8 decodes straight into
// char16_t. Only on meeting a character above U+FFFF -- always a 4-byte sequence --
// does it copy the codes it already has into char32_t and carry on there. The
// rules are strings/satellite_string.cpp's (the Unicode standard's Table 3-7),
// written out per sequence length; every refusal is string_error at the byte
// where the bad sequence STARTS, as that decoder answers (check_strings16.py holds
// it to Python on all 30,041 of that file's UTF-8 cases, and more).
//
// SPEED: conversion_loops.hpp says how, string_race.cpp measures it.
//
// MEMORY. The storage of the path not in use holds nothing: widening releases the
// char16_t storage, and a string that is fast again releases the char32_t one.
// A decoded string keeps the capacity it was sized to -- one character a byte --
// so text of 2- and 3-byte characters holds up to 2 or 3 times what it uses until
// it is changed or dropped (the price of one pass; the old decoder reserved the
// same, at 4 bytes a character).
//
// THE RULE every function keeps (the header's private section): wide_ is true
// exactly when some code is above 0xFFFF.

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

bool is_the_code_of_a_character(char32_t code)
{
    return code <= 0x10FFFF && (code < 0xD800 || code > 0xDFFF); // codes above 127 are Unicode numbers
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
    // The fast path first: straight into char16_t, at most one character a byte.
    // A wide `out` keeps its char32_t storage until the answer is known to be fast.
    std::size_t at = 0;
    stopped narrow_stop = stopped::at_the_end;
    overwrite_string(out.narrow16_, size, [&](char16_t *const first, std::size_t) {
        char16_t *write = first;
        narrow_stop = decode<char16_t, true>(bytes, size, at, write);
        return static_cast<std::size_t>(write - first);
    });
    if (narrow_stop != stopped::at_a_wide_character) {
        if (out.wide_) {
            std::u32string().swap(out.wide32_);
            out.wide_ = false;
        }
        if (narrow_stop == stopped::at_the_end)
            return success;
        bad_offset = at;
        return string_error;
    }
    // A character above U+FFFF: what is decoded so far moves to char32_t, and
    // decoding carries on there from that character.
    const std::size_t made = out.narrow16_.size();
    const char16_t *const decoded = out.narrow16_.data();
    stopped wide_stop = stopped::at_the_end;
    overwrite_string(out.wide32_, made + (size - at), [&](char32_t *const first, std::size_t) {
        for (std::size_t k = 0; k < made; k++)
            first[k] = decoded[k];
        char32_t *write = first + made;
        wide_stop = decode<char32_t, false>(bytes, size, at, write);
        return static_cast<std::size_t>(write - first);
    });
    std::u16string().swap(out.narrow16_);
    out.wide_ = true;
    if (wide_stop == stopped::at_the_end)
        return success;
    bad_offset = at;                                        // what came before stays wide: it holds the wide character
    return string_error;
}

std::string satellite_string::to_utf8() const
{
    return wide_ ? encode(wide32_.data(), wide32_.size()) : encode(narrow16_.data(), narrow16_.size());
}

signed long long int satellite_string::code_at(std::size_t index, char32_t &code) const
{
    if (index >= size())
        return position_past_the_end;
    code = code_at_unchecked(index);
    return success;
}

void satellite_string::become_wide(std::size_t room_for_more)
{
    const std::size_t count = narrow16_.size();
    wide32_.reserve(count + room_for_more);
    wide32_.resize(count);
    for (std::size_t k = 0; k < count; k++)
        wide32_[k] = narrow16_[k];
    std::u16string().swap(narrow16_);
    wide_ = true;
}

void satellite_string::append(const satellite_string &other)
{
    if (!other.wide_) {
        if (!wide_) {
            narrow16_.append(other.narrow16_);              // fast + fast stays fast; safe when other is *this
            return;
        }
        const std::size_t count = wide32_.size(), more = other.narrow16_.size(); // other is fast, so not *this
        wide32_.resize(count + more);
        for (std::size_t k = 0; k < more; k++)
            wide32_[count + k] = other.narrow16_[k];
        return;
    }
    if (!wide_)
        become_wide(other.wide32_.size());                  // other is wide, so not *this
    wide32_.append(other.wide32_);                          // safe when other is *this
}

signed long long int satellite_string::append_code(char32_t code)
{
    if (!is_the_code_of_a_character(code))
        return string_error;
    if (code > 0xFFFF && !wide_)
        become_wide(1);
    if (wide_)
        wide32_.push_back(code);
    else
        narrow16_.push_back(static_cast<char16_t>(code));
    return success;
}

void satellite_string::clear()
{
    narrow16_.clear();
    if (wide_) {
        std::u32string().swap(wide32_);
        wide_ = false;
    }
}

signed long long int satellite_string::substring(std::size_t start, std::size_t end, satellite_string &out) const
{
    if (start > end)                                        // 003's order: backwards first, then past the end
        return positions_backwards;
    if (end > size())
        return position_past_the_end;
    const std::size_t length = end - start;
    if (!wide_) {
        if (&out == this) {
            out.narrow16_.resize(end);
            out.narrow16_.erase(0, start);
        } else {
            if (out.wide_) { std::u32string().swap(out.wide32_); out.wide_ = false; }
            out.narrow16_.assign(narrow16_, start, length);
        }
        return success;
    }
    const char32_t *const from = wide32_.data() + start;
    char32_t every_bit = 0;                                 // above 0xFFFF exactly when some code is
    for (std::size_t k = 0; k < length; k++)
        every_bit |= from[k];
    if (every_bit > 0xFFFF) {
        if (&out == this) {
            out.wide32_.resize(end);
            out.wide32_.erase(0, start);
        } else {
            if (!out.wide_) { std::u16string().swap(out.narrow16_); out.wide_ = true; }
            out.wide32_.assign(wide32_, start, length);
        }
        return success;
    }
    // No wide character in the piece: it is fast again. Writing narrow16_ never
    // touches wide32_, so this is safe when out is *this.
    out.narrow16_.resize(length);
    for (std::size_t k = 0; k < length; k++)
        out.narrow16_[k] = static_cast<char16_t>(from[k]);
    if (out.wide_) { std::u32string().swap(out.wide32_); out.wide_ = false; }
    return success;
}

int satellite_string::compare(const satellite_string &left, const satellite_string &right)
{
    const std::size_t left_size = left.size(), right_size = right.size();
    const std::size_t common = left_size < right_size ? left_size : right_size;
    int order = 0;
    if (!left.wide_ && !right.wide_)
        order = std::char_traits<char16_t>::compare(left.narrow16_.data(), right.narrow16_.data(), common);
    else if (left.wide_ && right.wide_)
        order = std::char_traits<char32_t>::compare(left.wide32_.data(), right.wide32_.data(), common);
    else
        for (std::size_t k = 0; k < common && order == 0; k++) {
            const char32_t l = left.code_at_unchecked(k), r = right.code_at_unchecked(k);
            order = l < r ? -1 : l > r ? 1 : 0;
        }
    if (order != 0)
        return order < 0 ? -1 : 1;
    return left_size < right_size ? -1 : left_size > right_size ? 1 : 0;
}

} // namespace satellite004
