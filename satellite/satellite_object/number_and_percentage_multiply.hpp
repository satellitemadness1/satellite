#pragma once
// satellite/satellite_object/number_and_percentage_multiply.hpp -- ONE FUNCTION, ONE FILE.
//
// number * percentage -> number: that percentage OF the number. `200 * 50%` is 100,
// and `5 * 1000000000000%` is 50000000000 -- the author's "1000000000000% as
// something you can multiply by". percentage * number is the same answer, and the
// object model calls this for both orders.
//
// REFUSES answer_is_not_whole (24) when the answer is not whole: `3 * 50%` is 1.5,
// and a satellite.variable.number holds whole numbers until satellite_float lands.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_percentage/satellite_percentage.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int number_and_percentage_multiply(const satellite_number &left,
                                                          const satellite_percentage &right,
                                                          satellite_number &out)
{
    return satellite_percentage::whole_divide(left * right.scaled, satellite_percentage::whole(), out);
}

} // namespace satellite004
