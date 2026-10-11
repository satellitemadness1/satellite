#pragma once
// satellite/satellite_variable_fraction/satellite_fraction.hpp --
// `satellite.variable.fraction` `1 6 20`. Arm 17 of satelliteObject.
//
// (the author, 2026-09-22) "satellite.variable.fraction my_number =
// some_number_type/another_number_type and we auto convert for the "another"
// type ... let's just start with satellite.variable.fraction my_number =
// satellite_number1/satellite_number2 simply two numbers that are tied together,
// and when the user declares them, we will automatically make them floats with
// decimal points, but display them as just the whole number".
//
// WRITTEN WITH A TOUCHING SLASH: 1/3. The author ruled that on 2026-09-16 --
// "when you encounter number/number with NO space -- that becomes a fraction" --
// which is why a spaced `1 / 3` is whole-number division and REGISTRY.satellite
// has had fraction_token waiting since.
//
// TIED, NOT SIMPLIFIED. "two numbers that are tied together" keeps both numbers
// as they were written, so 2/4 is held and shown as 2/4 and never quietly becomes
// 1/2. What the two are WORTH is what a comparison asks, so 2/4 == 1/2 is true
// (object_fraction.cpp works it out by cross-multiplying). If the author rules
// that a fraction is kept in its lowest terms, that is one division by the
// greatest common divisor where a fraction is made, and 2/4 then displays 1/2.
//
// THE SIGN RIDES ON THE NUMERATOR: -1/3 is (-1, 3). Nothing makes a denominator
// below zero -- the lexer makes a fraction only when a DIGIT follows the slash,
// and a number given to a fraction name is put over 1 -- so a fraction keeps its
// one sign in one place, as a float keeps its one bool.

#include "../satellite_variable_float/satellite_float.hpp"

namespace satellite004 {

struct satellite_fraction {
    satellite_float numerator;           // above the line -- a float, as the author said
    satellite_float denominator;         // below it: never zero, never below zero

    // THE SAME FRACTION WITH ITS SIGN TURNED OVER, for a minus written in front of
    // one: -1/3. The numerator carries it, and a float's zero stays positive, so
    // -0/5 is 0/5.
    satellite_fraction negated() const
    {
        satellite_fraction out = *this;
        out.numerator = numerator.negated();
        return out;
    }
};

} // namespace satellite004
