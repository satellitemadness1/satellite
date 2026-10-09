#pragma once
// satellite/satellite_object/string_pieces.hpp -- what M16's string methods do to the
// characters, on the language's own string (satellite_string.hpp) and nothing else.
//
// (MILESTONES M16) "The list is already frozen -- 23 methods at `1 6 1 n` -- so none of
// this is a design question." Each method's behaviour is 003's, as the 32-bit library
// under satellite-numbers/satellite.variable.string.<method>/ holds it and
// strings/check_string_methods.py checks it against 003's satl: contains "" is true,
// "a,,b" splits into three pieces, replace never rescans what it wrote in, trim takes
// space, tab, carriage return and newline. Positions are the caller's (string_calls.cpp),
// because counting from 1 is the language's rule and not the characters'.
//
// A HEADER, AS string_case.hpp IS, so M21 can move those libraries onto this type by
// including it rather than by writing the loops a second time.
//
// EVERY WALK STARTS A CHARACTER AT UNIT 0 AND STEPS BY ITS WIDTH, and that is the one
// rule here that is not obvious. A wide character is three units -- 40000, then its
// number in two -- and its low half can be ANY sixteen bits: U+1005F ends in 95, the
// code of a space, and U+19C40 ends in 40000 itself. A search that compared units from
// every unit, or a trim that walked back from the end, would find a space or a match
// inside one character. str_find_str.cpp walks the same way for the same reason.
//
// ends_with IS THE SAME TRAP FROM THE OTHER END: "😀" is 40000, 0x0001, 0xF600, and
// the two-character text "a" + U+F600 is 0x0001, 0xF600 -- the last two units of the
// emoji. So the tail is compared only where a character of the receiver starts.

#include "../satellite_variable_string/satellite_string.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace satellite004 {
namespace string_pieces {

inline constexpr std::size_t kNowhere = static_cast<std::size_t>(-1);

// Whether `needle`'s units stand in `text` from `unit`, which is a character's start.
inline bool units_match_at(const satellite_string &text, std::size_t unit, const satellite_string &needle)
{
    return unit + needle.units() <= text.units() &&
           std::char_traits<char16_t>::compare(text.unit_data() + unit, needle.unit_data(), needle.units()) == 0;
}

// The UNIT where `needle` next begins, from the character that starts at `from`, or
// kNowhere. An empty needle is found where the search starts, as every library answers.
inline std::size_t find_from(const satellite_string &text, const satellite_string &needle, std::size_t from)
{
    for (std::size_t unit = from; unit + needle.units() <= text.units();) {
        if (units_match_at(text, unit, needle))
            return unit;
        std::size_t width = 0;
        text.code_at_unit(unit, width);
        unit += width;
    }
    return kNowhere;
}

inline bool contains(const satellite_string &text, const satellite_string &needle)
{
    return find_from(text, needle, 0) != kNowhere;
}

// The front is always a character's start, and the needle is whole characters, so a
// match of its units there ends on a character's end too.
inline bool starts_with(const satellite_string &text, const satellite_string &head)
{
    return units_match_at(text, 0, head);
}

inline bool ends_with(const satellite_string &text, const satellite_string &tail)
{
    if (tail.units() > text.units())
        return false;
    const std::size_t wanted = text.units() - tail.units();
    std::size_t unit = 0;
    if (!text.fast()) {                       // walk to the tail; only a start may begin it
        while (unit < wanted) {
            std::size_t width = 0;
            text.code_at_unit(unit, width);
            unit += width;
        }
    } else {
        unit = wanted;                        // no wide character: every unit starts one
    }
    return unit == wanted && units_match_at(text, unit, tail);
}

// The character that starts at `unit`, added to `out` whole.
inline std::size_t copy_character(const satellite_string &text, std::size_t unit, satellite_string &out)
{
    std::size_t width = 0;
    out.append_code(text.code_at_unit(unit, width));   // a code this string holds is always one it can take
    return width;
}

// Every appearance of `from` swapped for `to`, left to right, and never looked for again
// inside what was written in -- so replace("a", "aa") ends. `from` is never empty here:
// the caller refuses that first, as 003 did (S421).
inline satellite_string replace_all(const satellite_string &text, const satellite_string &from,
                                    const satellite_string &to)
{
    satellite_string out;
    for (std::size_t unit = 0; unit < text.units();) {
        if (units_match_at(text, unit, from)) {
            out.append(to);
            unit += from.units();
        } else {
            unit += copy_character(text, unit, out);
        }
    }
    return out;
}

// The pieces between separators. An EMPTY separator is every character on its own, and
// an empty piece is kept -- "a,,b" is three pieces -- because a dropped piece is a lost
// column in a line of CSV (003's two rules).
inline std::vector<satellite_string> split(const satellite_string &text, const satellite_string &separator)
{
    std::vector<satellite_string> pieces(1);
    for (std::size_t unit = 0; unit < text.units();) {
        if (separator.empty()) {
            if (unit != 0)
                pieces.emplace_back();
            unit += copy_character(text, unit, pieces.back());
        } else if (units_match_at(text, unit, separator)) {
            pieces.emplace_back();
            unit += separator.units();
        } else {
            unit += copy_character(text, unit, pieces.back());
        }
    }
    if (separator.empty() && text.empty())
        pieces.clear();                        // no characters, so no pieces of one character
    return pieces;
}

// Without space, tab, carriage return or newline at either end -- the four 003 took, the
// whitespace with no code of its own among the 128 (satellite_string.hpp's table). Walked
// FORWARD to both ends: the last blank-looking unit may be a wide character's low half.
inline satellite_string trimmed(const satellite_string &text)
{
    static const char32_t blanks[4] = {satellite_string::code_of(U' '), satellite_string::code_of(U'\t'),
                                       satellite_string::code_of(U'\r'), satellite_string::code_of(U'\n')};
    auto blank = [](char32_t code) {
        return code == blanks[0] || code == blanks[1] || code == blanks[2] || code == blanks[3];
    };
    std::size_t first = kNowhere, end = 0, character = 0;
    for (std::size_t unit = 0; unit < text.units(); ++character) {
        std::size_t width = 0;
        if (!blank(text.code_at_unit(unit, width))) {
            if (first == kNowhere)
                first = character;
            end = character + 1;
        }
        unit += width;
    }
    satellite_string out;
    if (first != kNowhere)
        text.substring(first, end, out);
    return out;
}

} // namespace string_pieces
} // namespace satellite004
