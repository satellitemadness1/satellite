#pragma once
// satellite/satellite_object/percentage_and_percentage_multiply.hpp -- ONE FUNCTION, ONE FILE.
//
// percentage * percentage -> percentage: a percentage OF a percentage. `50% * 50%`
// is 25%, and `10% * 10%` is 1%. Scaled, that is P * Q / (100 * 10^32), which is
// the one place two percentages can make more than 32 digits after the point, so
// it is rounded half away from zero like every other answer (satellite_percentage.hpp).

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_percentage/satellite_percentage.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int percentage_and_percentage_multiply(const satellite_percentage &left,
                                                              const satellite_percentage &right,
                                                              satellite_percentage &out)
{
    out.scaled = satellite_percentage::rounded_divide(left.scaled * right.scaled, satellite_percentage::whole());
    return success;
}

} // namespace satellite004
