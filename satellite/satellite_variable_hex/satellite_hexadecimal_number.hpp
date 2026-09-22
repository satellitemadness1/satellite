#pragma once
// satellite/satellite_variable_hex/satellite_hexadecimal_number.hpp --
// `satellite.variable.hex` `1 6 11`, second spelling `satellite.variable.hexadecimal`
// (words/aliases.tsv). Arm 15 of satelliteObject.
//
// (the author, 2026-09-22) "similar thing for hex, which has an x in front of it,
// with hex numbers only for it 0 - 9 A - F" -- similar to the binary, whose b "is
// optional as long as the number is only 1's and 0's".
//
// THE BINARY'S SHAPE, IN BASE 16 (satellite_binary_number.hpp says why each field
// is there): what it is worth, sign and all, in a satellite_number, and the width
// as written beside it, because x00FF and xFF are two values that display
// differently.

#include "../satellite_variable_number/satellite_number.hpp"

namespace satellite004 {

struct satellite_hexadecimal_number {
    satellite_number worth;              // what the digits are worth, sign and all
    unsigned long long int width = 0;    // how many digits were written, leading zeros included
};

} // namespace satellite004
