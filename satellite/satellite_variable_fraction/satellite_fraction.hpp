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

#include "../satellite_variable_float/satellite_float.hpp"

namespace satellite004 {

struct satellite_fraction {
    satellite_float numerator;           // above the line -- a float, as the author said
    satellite_float denominator;         // below it
};

} // namespace satellite004
