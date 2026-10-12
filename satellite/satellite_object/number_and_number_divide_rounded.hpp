#pragma once
// satellite/satellite_object/number_and_number_divide_rounded.hpp -- ONE FUNCTION, ONE FILE
// (number_and_number_divide.hpp says why the shape).
//
// number / number -> number, ROUNDED: .5 and above is one more, below .5 is not (the author,
// 2026-10-12: "7 / 2 = 4 if a number", ".500000000 is plus one .499999999 is not plus one"). 7 / 2
// is 4, 5 / 4 is 1, 6 / 4 is 2, 4 / 2 is 2 -- and on the minus side half away from zero, as every
// float rounds (SATELLITE_INFINITY.md Q17b): -7 / 2 is -4. satellite.convert.number(a / b)'s,
// worked out at its root by the evaluator (expression.cpp), so no fraction is made to be rounded.
//
// THE .5 TEST IS EXACT, WITH NO FLOAT IN IT: twice what is left over against the divisor. 7 / 2
// is 3 with 1 left over, and 1 + 1 is not below 2, so it is 4.
//
// `lost` is set when something was left over -- what the author's warning is about ("With a
// warning", S021) -- and left alone when the division came out even: 4 / 2 is 2 and says nothing.
//
// REFUSES division_by_zero (22), as number_and_number_divide does.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int number_and_number_divide_rounded(const satellite_number &left,
                                                             const satellite_number &right,
                                                             satellite_number &out, bool &lost)
{
    satellite_number remainder;
    const signed long long int code = satellite_number::divide(left, right, out, remainder);   // toward zero
    if (code != success || remainder.is_zero())
        return code;
    lost = true;
    // THE REMAINDER HAS THE DIVIDEND'S SIGN, so both sides of the test are measured without theirs.
    const satellite_number twice = remainder.negative() ? -(remainder + remainder) : remainder + remainder;
    if (satellite_number::compare(twice, right.negative() ? -right : right) >= 0)
        out += satellite_number(1ull, left.negative() != right.negative());
    return success;
}

} // namespace satellite004
