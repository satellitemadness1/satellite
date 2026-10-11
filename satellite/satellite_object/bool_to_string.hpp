#pragma once
// satellite/satellite_object/bool_to_string.hpp -- ONE FUNCTION, ONE FILE.
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
// bool -> string: "true" or "false", the spellings satellite.bool.true and
// satellite.bool.false already print through satellite.console.display, so a
// bool converted by hand and a bool displayed read the same.

#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int bool_to_string(bool from, satellite_string &out)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(from ? "true" : "false", out, bad_offset);
}

} // namespace satellite004
