#pragma once
// satellite/satellite_object/string_to_number.hpp -- ONE FUNCTION, ONE FILE.
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
// string -> number, base 10. The way back, and the strict one: an optional '-'
// then digits and nothing else -- no '+', no spaces, no separators, no decimal
// point. Anything else is int_error (3) and `out` is left untouched, so a
// program never receives a value standing for text that could not be read.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int string_to_number(const satellite_string &from, satellite_number &out)
{
    std::size_t bad_offset = 0;
    return satellite_number::from_text(from.to_utf8(), out, bad_offset);
}

} // namespace satellite004
