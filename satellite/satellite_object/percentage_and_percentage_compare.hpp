#pragma once
// satellite/satellite_object/percentage_and_percentage_compare.hpp -- ONE FUNCTION, ONE FILE.
//
// percentage against percentage: -1, 0 or 1. Both hold 32 digits after the point,
// so comparing the scaled whole numbers IS comparing the percentages.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_percentage/satellite_percentage.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline int percentage_and_percentage_compare(const satellite_percentage &left, const satellite_percentage &right)
{
    return satellite_number::compare(left.scaled, right.scaled);
}

} // namespace satellite004
