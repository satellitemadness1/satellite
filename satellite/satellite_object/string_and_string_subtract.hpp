#pragma once
// satellite/satellite_object/string_and_string_subtract.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// string - string -> string: THE FIRST OCCURRENCE OF THE RIGHT STRING IS TAKEN
// AWAY (the author, 2026-09-17): "minus takes away the smallest string", with
// "dfksjghjfff" - "fff" as "dfksjghj"; asked which copy goes when there are two,
// "first occurrence", so "abcab" - "ab" is "cab"; and "minus - fff", the string
// after the minus is the one taken away.
//
//     "dfksjghjfff" - "fff"    is "dfksjghj"
//     "abcab" - "ab"           is "cab"
//     "fff" - "dfksjghjfff"    is "fff"      (there is no "dfksjghjfff" in "fff")
//
// NOTHING TO TAKE AWAY IS AN ANSWER, NOT A REFUSAL: a right string that is empty,
// or is not in the left, leaves the left as it was. Subtracting nothing from
// something is that something.
//
// CHARACTER BY CHARACTER, comparing UNITS from the start of each one --
// str_find_str.cpp's walk, for its reason: a wide character's halves can be
// 40000 or the code of a letter (satellite_string.hpp), so a match may only begin
// where a character begins.
//
// CANNOT REFUSE: every position it hands substring() is a character the walk
// counted, inside the left string.

#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

#include <string>
#include <utility>

namespace satellite004 {

inline signed long long int string_and_string_subtract(const satellite_string &left,
                                                       const satellite_string &right,
                                                       satellite_string &out)
{
    const std::size_t taken = right.units(), units = left.units();
    std::size_t unit = 0, character = 0;
    bool found = false;
    while (taken != 0 && unit + taken <= units) {
        if (std::char_traits<char16_t>::compare(left.unit_data() + unit, right.unit_data(), taken) == 0) {
            found = true;
            break;
        }
        std::size_t width = 0;
        left.code_at_unit(unit, width);
        unit += width;
        ++character;
    }
    if (!found) {
        out = left;
        return success;
    }

    // The characters before it, then the characters after it.
    satellite_string answer, after;
    signed long long int code = left.substring(0, character, answer);
    if (code == success)
        code = left.substring(character + right.size(), left.size(), after);
    if (code != success)
        return code;
    answer.append(after);
    out = std::move(answer);
    return success;
}

} // namespace satellite004
