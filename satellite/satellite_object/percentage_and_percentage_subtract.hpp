#pragma once
// satellite/satellite_object/percentage_and_percentage_subtract.hpp -- ONE FUNCTION, ONE FILE.
//
// percentage - percentage -> percentage. `50% - 75%` is -25%. Exact, as the sum is.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_percentage/satellite_percentage.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int percentage_and_percentage_subtract(const satellite_percentage &left,
                                                              const satellite_percentage &right,
                                                              satellite_percentage &out)
{
    out.scaled = left.scaled - right.scaled;
    return success;
}

} // namespace satellite004
