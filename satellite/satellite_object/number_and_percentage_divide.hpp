#pragma once
// satellite/satellite_object/number_and_percentage_divide.hpp -- ONE FUNCTION, ONE FILE.
//
// number / percentage -> number: what the number is that percentage OF. `200 / 50%`
// is 400. REFUSES division_by_zero (22) for 0%, and answer_is_not_whole (24) when
// the answer is not whole: `5 / 200%` is 2.5.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_percentage/satellite_percentage.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int number_and_percentage_divide(const satellite_number &left,
                                                        const satellite_percentage &right,
                                                        satellite_number &out)
{
    if (right.scaled.is_zero())
        return division_by_zero;
    return satellite_percentage::whole_divide(left * satellite_percentage::whole(), right.scaled, out);
}

} // namespace satellite004
