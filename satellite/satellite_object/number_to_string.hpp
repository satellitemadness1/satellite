#pragma once
// satellite/satellite_object/number_to_string.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "then we do complicated template work for all
// possibilities -- float to binary, number to string".
//
// A CONVERSION IS A PAIR TOO, and it is named the same way the operations are:
// <from>_to_<to>.hpp, one function, named exactly as the file. When
// satellite_float, satellite_binary_number and satellite_hexadecimal_number are
// built, float_to_binary.hpp goes in beside these and nothing already written
// has to move.
//
// number -> string, in base 10, carrying the '-'. This is the conversion the
// language already spells satellite.variable.number.to_string (1 6 4 6), and the
// one satellite.console.display leans on: a number past one limb cannot go to a
// library's count scenario without losing its value, so text is its only exact
// road out.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int number_to_string(const satellite_number &from, satellite_string &out)
{
    // to_text() answers ASCII digits and at most a '-', so from_utf8 cannot
    // refuse them -- but its code is returned rather than dropped, because a
    // conversion that cannot fail today is not a conversion that may not fail
    // when the number's text changes.
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(from.to_text(), out, bad_offset);
}

} // namespace satellite004
