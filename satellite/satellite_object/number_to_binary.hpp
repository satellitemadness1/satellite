#pragma once
// satellite/satellite_object/number_to_binary.hpp -- ONE FUNCTION, ONE FILE.
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
// number -> string, base 2, WITHOUT a leading b: 12 writes as "1100".
//
// A satellite.variable.binary DOES NOT COME HERE, because a number has no width
// and the trip would turn b0011 into "11". Its `.bin` answers its own digits,
// leading zeros kept -- object_convert.cpp's binary branch and
// satelliteObject::to_binary.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int number_to_binary(const satellite_number &from, satellite_string &out)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(from.to_radix_text(2), out, bad_offset);
}

} // namespace satellite004
