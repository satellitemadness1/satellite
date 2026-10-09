#pragma once
// satellite/satellite_object/percentage_and_number_divide.hpp -- ONE FUNCTION, ONE FILE.
//
// percentage / number -> percentage. `50% / 4` is 12.5%, rounded to 32 digits after
// the point. REFUSES division_by_zero (22).

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_percentage/satellite_percentage.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int percentage_and_number_divide(const satellite_percentage &left,
                                                        const satellite_number &right,
                                                        satellite_percentage &out)
{
    if (right.is_zero())
        return division_by_zero;
    out.scaled = satellite_percentage::rounded_divide(left.scaled, right);
    return success;
}

} // namespace satellite004
