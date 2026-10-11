#pragma once
// satellite/satellite_object/number_and_percentage_add.hpp -- ONE FUNCTION, ONE FILE.
//
// number + percentage -> number, GROWN by that percentage. `200 + 50%` is 300.
// The same rule as subtract, which is the author's: `infinity - 50%` carries
// x0.5 (MILESTONES M11), so a percentage added or taken away scales the number
// rather than being added to it as a count.
//
// REFUSES answer_is_not_whole (24): `3 + 50%` is 4.5.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_percentage/satellite_percentage.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int number_and_percentage_add(const satellite_number &left,
                                                     const satellite_percentage &right,
                                                     satellite_number &out)
{
    return satellite_percentage::whole_divide(left * (satellite_percentage::whole() + right.scaled),
                                              satellite_percentage::whole(), out);
}

} // namespace satellite004
