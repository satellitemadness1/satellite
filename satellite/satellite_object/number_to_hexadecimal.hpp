#pragma once
// satellite/satellite_object/number_to_hexadecimal.hpp -- ONE FUNCTION, ONE FILE.
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
// number -> string, base 16, upper case, without the leading x. xFFAA reads as
// 65450 and 65450 writes back as "FFAA".
//
// WHEN satellite_hexadecimal_number IS BUILT this file is where it lands, for
// the same reason number_to_binary.hpp is.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int number_to_hexadecimal(const satellite_number &from, satellite_string &out)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(from.to_radix_text(16), out, bad_offset);
}

} // namespace satellite004
