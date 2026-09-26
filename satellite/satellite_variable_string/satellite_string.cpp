// satellite/satellite_variable_string/satellite_string.cpp -- satellite.variable.string:
// sixteen bits a character with the author's table (character_table.hpp), and
// 40000 for a character that needs more. satellite_string.hpp is the contract;
// this file keeps it.
//
// TWO PATHS, AND THE FIRST IS THE ONE THAT MATTERS (the author, 2026-09-15): "we
// just check if it fits into the 16-bit fast path first thing we do". Every
// function here asks wide_count_ first. At zero -- every ASCII and every 16-bit
// text -- it does exactly what the 16-bit-only string did: one index, one
// std::char_traits call, one loop. Only a string holding a wide character walks.
//
// DECODING IS ONE PASS. from_utf8 decodes straight into char16_t. The decoder
// stops on a character that must go wide (above U+FFFF, or U+9C40, whose number is
// the wide value itself); this file writes its three units and hands the decoder
// back the rest. The rules are the Unicode standard's Table 3-7, written out per
// sequence length; every refusal is string_error at the byte where the bad
// sequence STARTS (check_strings16.py holds it to Python).
//
// SPEED: conversion_loops.hpp says how, string_race.cpp measures it.
//
// MEMORY. A decoded string keeps the capacity it was sized to -- one unit a byte
// -- so text of 2- and 3-byte characters holds up to 2 or 3 times what it uses
// until it is changed or dropped. That is the price of one pass. A wide character
// is 3 units for 3 or 4 bytes, so the size never has to grow while decoding.

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
using conversion_loops::write_utf8;

// What this type can hold: not a surrogate and not above 0x10FFFF.
bool is_the_code_of_a_character(char32_t code)
{
    return code <= 0x10FFFF && (code < 0xD800 || code > 0xDFFF);
}

// Whether a code is stored as 40000 and two units rather than as itself.
bool goes_wide(char32_t code)
{
    return code > 0xFFFF || code == satellite_string::kWide;
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
    std::size_t wide = 0;
    stopped stop = stopped::at_the_end;
    out.bookmark_.store(0, std::memory_order_relaxed);   // `out` is written over from its first unit
    overwrite_string(out.narrow16_, size, [&](char16_t *const first, std::size_t) {
        char16_t *write = first;
        for (;;) {
            stop = decode<char16_t, true>(bytes, size, at, write);
            if (stop != stopped::at_a_wide_character)
                break;
            // THE DECODER CHECKED THE SEQUENCE BEFORE IT STOPPED, so it is a whole
            // valid character of 3 bytes (U+9C40) or 4 (above U+FFFF).
            const unsigned char b0 = bytes[at];
            char32_t code;
            if (b0 >= 0xF0) {
                code = ((b0 & 0x07u) << 18) | ((bytes[at + 1] & 0x3Fu) << 12) | ((bytes[at + 2] & 0x3Fu) << 6) |
                       (bytes[at + 3] & 0x3Fu);
                at += 4;
            } else {
                code = ((b0 & 0x0Fu) << 12) | ((bytes[at + 1] & 0x3Fu) << 6) | (bytes[at + 2] & 0x3Fu);
                at += 3;
            }
            write[0] = kWide;
            write[1] = static_cast<char16_t>(code >> 16);
            write[2] = static_cast<char16_t>(code & 0xFFFFu);
            write += 3;
            ++wide;
        }
        return static_cast<std::size_t>(write - first);
    });
    out.wide_count_ = wide;

    if (stop == stopped::at_the_end)
        return success;
    // `out` holds the characters before the bad sequence, wide ones counted.
    bad_offset = at;
    return string_error;
}

std::string satellite_string::to_utf8() const
{
    if (wide_count_ == 0) [[likely]]
        return encode(narrow16_.data(), narrow16_.size());

    // THE SLOWER PATH, STILL ONE STRING: sized once for the most the units can need --
    // 3 bytes a unit, and a wide character's 3 units are at most 4 -- and written in
    // place. The runs between wide characters go through the fast writer whole, and
    // each wide character is written on its own. A 40000 found from the start of a
    // character is always a marker, so the search is safe. Building a std::string for
    // each run and appending it was x2.73 of plain C++ on text with emoji (string_race,
    // 2026-09-17).
    std::string out;
    const char16_t *const units = narrow16_.data();
    const std::size_t count = narrow16_.size();
    overwrite_string(out, count * 3, [&](char *const first, std::size_t) {
        char *write = first;
        std::size_t k = 0;
        while (k < count) {
            const char16_t *const found = std::char_traits<char16_t>::find(units + k, count - k, kWide);
            const std::size_t marker = found == nullptr ? count : static_cast<std::size_t>(found - units);
            write_utf8<char16_t, false>(units, marker, k, write);
            if (marker == count)
                break;
            // Above U+FFFF, or U+9C40 itself: its number is the two units after the marker.
            const char32_t c = (static_cast<char32_t>(units[marker + 1]) << 16) | units[marker + 2];
            if (c < 0x10000) {
                write[0] = static_cast<char>(0xE0 | (c >> 12));
                write[1] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                write[2] = static_cast<char>(0x80 | (c & 0x3F));
                write += 3;
            } else {
                write[0] = static_cast<char>(0xF0 | (c >> 18));
                write[1] = static_cast<char>(0x80 | ((c >> 12) & 0x3F));
                write[2] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                write[3] = static_cast<char>(0x80 | (c & 0x3F));
                write += 4;
            }
            k = marker + 3;
        }
        return static_cast<std::size_t>(write - first);
    });
    return out;
}

// to_utf8's bytes on the end of `out`. THE FAST PATH IS to_utf8's OWN, written at the end of
// what `out` already holds -- one byte a unit first, grown once at the first code that is not
// ASCII. A string holding a wide character is rare enough to go through to_utf8 whole.
void satellite_string::append_utf8_to(std::string &out) const
{
    if (wide_count_ != 0) {
        out += to_utf8();
        return;
    }
    const char16_t *const units = narrow16_.data();
    const std::size_t count = narrow16_.size();
    const std::size_t start = out.size();
    std::size_t k = 0;
    overwrite_string(out, start + count, [&](char *const first, std::size_t) {
        char *write = first + start;
        write_utf8<char16_t, true>(units, count, k, write);
        return static_cast<std::size_t>(write - first);
    });
    if (k == count)
        return;
    const std::size_t written = out.size();
    overwrite_string(out, written + (count - k) * 3, [&](char *const first, std::size_t) {
        char *write = first + written;
        write_utf8<char16_t, false>(units, count, k, write);
        return static_cast<std::size_t>(write - first);
    });
}

// FROM THE BOOKMARK WHEN IT IS NOT PAST THE CHARACTER, and from the front when it is
// (satellite_string.hpp says why a walk can never go backwards). Either way the walk
// ends by moving the bookmark to where it stopped, so the next character along is one
// step -- a loop over s[i] is a walk of the string once, not once for every i.
std::size_t satellite_string::unit_of(std::size_t character) const
{
    constexpr std::uint64_t kLow = 0xFFFFFFFFu;
    const std::uint64_t mark = bookmark_.load(std::memory_order_relaxed);
    std::size_t c = 0, unit = 0;
    if (static_cast<std::size_t>(mark >> 32) <= character) {
        c = static_cast<std::size_t>(mark >> 32);
        unit = static_cast<std::size_t>(mark & kLow);
    }
    for (; c < character; ++c)
        unit += narrow16_[unit] == kWide ? 3 : 1;
    if (unit <= kLow)   // a character is never past its unit, so it fits too
        bookmark_.store((static_cast<std::uint64_t>(character) << 32) | unit, std::memory_order_relaxed);
    return unit;
}

char32_t satellite_string::code_at_walking(std::size_t index) const
{
    std::size_t width = 0;
    return code_at_unit(unit_of(index), width);
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
    const std::size_t wide = other.wide_count_;   // read first: `other` may be *this
    narrow16_.append(other.narrow16_);            // safe when `other` is *this
    wide_count_ += wide;
}

signed long long int satellite_string::append_code(char32_t code)
{
    if (!is_the_code_of_a_character(code))
        return string_error;
    if (!goes_wide(code)) [[likely]] {
        narrow16_.push_back(static_cast<char16_t>(code));
        return success;
    }
    const char16_t units[3] = {kWide, static_cast<char16_t>(code >> 16), static_cast<char16_t>(code & 0xFFFFu)};
    narrow16_.append(units, 3);
    ++wide_count_;
    return success;
}

void satellite_string::clear()
{
    narrow16_.clear();
    wide_count_ = 0;
    bookmark_.store(0, std::memory_order_relaxed);
}

signed long long int satellite_string::substring(std::size_t start, std::size_t end, satellite_string &out) const
{
    if (start > end)                    // 003's order: backwards first, then past the end
        return positions_backwards;
    if (end > size())
        return position_past_the_end;

    std::size_t first = start, last = end, wide = 0;
    if (wide_count_ != 0) {
        // Characters to units, and the wide characters between them counted, in one walk.
        first = unit_of(start);
        last = first;
        for (std::size_t c = start; c < end; ++c) {
            const bool is_wide = narrow16_[last] == kWide;
            wide += is_wide;
            last += is_wide ? 3 : 1;
        }
    }
    if (&out == this) {
        out.narrow16_.resize(last);
        out.narrow16_.erase(0, first);
    } else {
        out.narrow16_.assign(narrow16_, first, last - first);
    }
    out.wide_count_ = wide;
    out.bookmark_.store(0, std::memory_order_relaxed);   // its characters are not where they were
    return success;
}

int satellite_string::compare(const satellite_string &left, const satellite_string &right)
{
    if (left.wide_count_ == 0 && right.wide_count_ == 0) [[likely]] {
        // Every unit is a whole character and its code, so unit order is code order.
        const std::size_t left_size = left.narrow16_.size(), right_size = right.narrow16_.size();
        const std::size_t common = left_size < right_size ? left_size : right_size;
        const int order = std::char_traits<char16_t>::compare(left.narrow16_.data(), right.narrow16_.data(), common);
        if (order != 0)
            return order < 0 ? -1 : 1;
        return left_size < right_size ? -1 : left_size > right_size ? 1 : 0;
    }

    // THE SLOWER PATH COMPARES CODES, NOT UNITS: a wide character's first unit is
    // 40000, which would sort it below U+FFFF when its code is above it.
    std::size_t l = 0, r = 0, left_width = 0, right_width = 0;
    const std::size_t left_units = left.narrow16_.size(), right_units = right.narrow16_.size();
    while (l < left_units && r < right_units) {
        const char32_t a = left.code_at_unit(l, left_width);
        const char32_t b = right.code_at_unit(r, right_width);
        if (a != b)
            return a < b ? -1 : 1;
        l += left_width;
        r += right_width;
    }
    return l < left_units ? 1 : r < right_units ? -1 : 0;
}

} // namespace satellite004
