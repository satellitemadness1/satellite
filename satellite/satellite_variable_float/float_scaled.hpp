#pragma once
// satellite/satellite_variable_float/float_scaled.hpp -- A FLOAT WORKED OUT AS ONE
// WHOLE NUMBER AND A COUNT OF PLACES, which is how every operation on one is done.
//
// The float is HELD as the author's shape -- a sign, the whole side and the
// fraction side (satellite_float.hpp) -- and WORKED as one satellite_number: 12.05
// is 1205 at 2 places, -0.5 is -5 at 1 place. Two floats are added by bringing
// them to the same places and adding two whole numbers, multiplied by
// multiplying them and adding the places, and so on; every step is the
// satellite_number arithmetic check_numbers.py already proves, and nothing here
// is a second implementation of it.
//
// EVERY FLOAT IS MADE BY ONE OF THE TWO FUNCTIONS BELOW, and that is the point of
// this file. Both round by the rows (float_precision.hpp) and both leave the
// value in THE ONE FORM:
//
//   - the fraction ends in no zero: 12.50 is held as 12.5, and 3.0 as 3 with no
//     places at all;
//   - zero is never negative, as for satellite_number.
//
// So two floats are the same value exactly when their four fields are the same
// (object_float.cpp's float_same), and a float that came out even -- 0.25 + 0.75
// -- goes back to the one-limb fast path instead of dragging 128 places along.
//
// ONE ROUNDING, NEVER TWO. Half away from zero (SATELLITE_INFINITY.md Q17b), at
// the last digit kept. The whole side is asked first, because rounding there
// takes the whole fraction with it; rounding the fraction first and then the
// whole side could round a 4 up to a 5 and that 5 up again.

#include "satellite_float.hpp"
#include "float_precision.hpp"

namespace satellite004 {

// 10 to the power of `count`: from a table while it fits one limb, and
// satellite_number::power beyond that.
satellite_number ten_to_the(unsigned long long int count);

// Half away from zero: 25 / 10 is 3, -25 / 10 is -3, 24 / 10 is 2. The divisor
// is above zero.
satellite_number divided_rounded(const satellite_number &dividend, const satellite_number &divisor);

// The value as one signed whole number at `places` places, which is never fewer
// than its own: 12.05 at 3 places is 12050, -0.5 at 1 place is -5.
satellite_number float_scaled(const satellite_float &value, unsigned long long int places);

// AN EXACT ANSWER, `scaled` at `places` places -- a literal, + - * %, a whole
// power -- rounded by the rows and put in the one form.
satellite_float float_from_scaled(satellite_number scaled, unsigned long long int places);

// A QUOTIENT, `numerator / denominator` -- a division, a negative power. Neither
// is negative and the denominator is not zero; `negative` is the answer's sign.
// It stops at the fewest places that end it (1 / 4 is 0.25, found without
// working 128 places out and taking 126 zeros back off), and at
// arguments.float.decimal places when nothing ends it.
satellite_float float_from_quotient(const satellite_number &numerator, const satellite_number &denominator,
                                    bool negative);

// A plain number as a float -- 7 is 7.0 -- through float_from_scaled, so a number
// longer than arguments.float.whole digits is rounded like any float.
satellite_float float_of_number(const satellite_number &value);

// WHAT satellite.console.display PRINTS, and `.string` answers: the whole side,
// the point, and the fraction to at most `float_precision_in_use.shown` places,
// rounded half away from zero there and with no zero at its end -- but always
// one place, so a float never displays as the number it is not: 12.50 is 12.5
// and 2.0 is 2.0.
std::string float_written(const satellite_float &value);

} // namespace satellite004
