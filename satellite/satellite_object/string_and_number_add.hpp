#pragma once
// satellite/satellite_object/string_and_number_add.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// string + number: THE NUMBER JOINS AS ITS DIGITS. The author, 2026-09-25: *"we need to
// auto convert here for the user into string, so when we have a string and we add a
// number to it, it has to auto convert"*. So `"total: " + total` is "total: 4", exactly
// what `"total: " + total.string` was -- the ruling landing in the file that was kept
// for it (number_and_string_add.hpp called this pair a seam, not a wall). The other
// order, number + string, still refuses: his words name a string with a number added
// to it, and `4 + "2"` is 6 in 003 and "42" by this rule -- his to choose.
//
// BOTH ORDERS GET A FILE even though both answer the same thing today, because
// the pair table is indexed by (left, right) and a reader looking for 'what
// happens when a string meets a number' must find it under that name.

#include "number_to_string.hpp"
#include "string_and_string_add.hpp"
#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int string_and_number_add(const satellite_string &text,
                                                  const satellite_number &number,
                                                  satellite_string &out)
{
    satellite_string digits;
    const signed long long int converted = number_to_string(number, digits);
    if (converted != success)
        return converted;
    return string_and_string_add(text, digits, out);
}

} // namespace satellite004
