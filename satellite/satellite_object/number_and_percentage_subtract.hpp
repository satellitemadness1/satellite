#pragma once
// satellite/satellite_object/number_and_percentage_subtract.hpp -- ONE FUNCTION, ONE FILE.
//
// number - percentage -> number, REDUCED by that percentage. `200 - 50%` is 100 --
// the author's own example is `infinity - 50%`, which carries x0.5 (MILESTONES
// M11), and this is that rule on a number. `200 - 150%` is -100.
//
// REFUSES answer_is_not_whole (24): `3 - 50%` is 1.5.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_percentage/satellite_percentage.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int number_and_percentage_subtract(const satellite_number &left,
                                                          const satellite_percentage &right,
                                                          satellite_number &out)
{
    return satellite_percentage::whole_divide(left * (satellite_percentage::whole() - right.scaled),
                                              satellite_percentage::whole(), out);
}

} // namespace satellite004
