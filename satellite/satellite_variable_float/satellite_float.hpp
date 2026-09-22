#pragma once
// satellite/satellite_variable_float/satellite_float.hpp -- `satellite.variable.float`
// `1 6 10`, second spelling `satellite.variable.double` (words/aliases.tsv). Arm 14
// of satelliteObject.
//
// (the author, 2026-09-22) "a float is going to be two satellite_number's put
// together, one before the decimal, one after, plus a sign which is positive by
// default, and a precision that it gets from arguments". And on the precision:
// "should we divide the precision to arguments.float.whole(4096) and
// arguments.float.decimal(4096) so users can set different precisions? I dunno,
// I think we should have the different values thing". MILESTONES M20 decided the
// same shape on 2026-08-27: "a float is a bool and two satellite_numbers".
//
// THE ZERO AFTER THE POINT IS WHY `places` IS HERE. 12.05 and 12.5 are two
// floats, but the digits after the point read as a number are 5 both times --
// a number cannot keep a leading zero. So the fraction is a whole number OVER
// 10^places: 12.05 is (12, 5, 2 places) and 12.5 is (12, 5, 1 place).
// SATELLITE_INFINITY.md Part 10 held the fraction at a fixed number of places
// instead (12.05 as 05000...); a count is the same value without writing every
// float out to arguments.float.decimal digits. If the author rules for the fixed
// width, this field goes and the fraction is always that many places.

#include "../satellite_variable_number/satellite_number.hpp"

namespace satellite004 {

struct satellite_float {
    bool negative = false;               // the sign -- positive by default (the author)
    satellite_number whole;              // the digits before the point, never negative
    satellite_number fraction;           // the digits after it, as a whole number ...
    unsigned long long int places = 0;   // ... over 10^places: .05 is 5 at 2 places

    // THE SAME FLOAT WITH ITS SIGN TURNED OVER, for a minus written in front of
    // one: -12.5. Zero stays positive, as a satellite_number's does, so -0.0 is 0.0.
    satellite_float negated() const
    {
        satellite_float out = *this;
        out.negative = !negative && !(whole.is_zero() && fraction.is_zero());
        return out;
    }
};

// EVERY FLOAT IS HELD IN ONE FORM -- no zero at the end of the fraction, so 12.50
// is (12, 5, 1 place) like 12.5 -- and satellite_variable_float/float_scaled.cpp is
// what keeps it so: every float is made through float_from_scaled or
// float_from_quotient there.

} // namespace satellite004
