#pragma once

// The digit-level helpers float_value.cpp, float_arith.cpp and
// float_power.cpp share. See satellite_float.hpp for the type itself.
//
// EVERYTHING HERE WORKS THROUGH Number's PUBLIC SURFACE -- to_string, parse,
// and the exact operations -- and that is a decision rather than a shortcut.
// PLAN §6.1 calls satellite_number "internally closed, porting as-is", and M8
// kept it that way; a float that reached into the limbs would be the second
// module with an opinion about the representation. The cost is a string per
// decomposition, which is real and measured against nothing yet: the day a
// profile names this file, the fix is a digit window on Number, beside
// small_parts() where the sanctioned windows live -- not a friend declaration
// here.

#include "satellite_number/bignum.hpp"

#include <string>

namespace satellite::floats {

// A magnitude as bare digits and a scale:  value = 0.digits x 10^point,
// with no leading zero on `digits` and no trailing zero either (Number's
// canonical form has none to give). Zero is the empty string.
//
//     12.5    ->  "125"  point 2        0.014  ->  "14"  point -1
//     1e+40   ->  "1"    point 41       0      ->  ""    point 0
//
// One decomposition serving places(), rendering, truncation and the rounding
// step, so the four cannot disagree about where the decimal point is.
struct Decimal {
    std::string digits;
    long long point = 0;
};

// Decompose a non-negative Number. Handles both of render.cpp's notations --
// fixed and exponent -- because to_string() switches between them on padding.
Decimal decompose(const Number &magnitude);

// The digits right of the point: max(0, len - point).
unsigned places_of(const Decimal &d);

// A non-negative Number rebuilt from a Decimal.
Number compose(const Decimal &d);

// The magnitude with everything below 10^-n dropped -- floor, in places.
Number truncate_places(const Number &magnitude, unsigned n);

// 10^-n, the one ulp step every adjustment below moves by. `n` is bounded by
// the float_digits range (config_internal.hpp: at most INT_MAX, because this
// exponent is an int in Number's small form).
Number place_step(unsigned n);

// floor(log10(magnitude)) for a positive magnitude: 350 -> 2, 0.05 -> -2.
// The digit-budget estimates in divide, sqrt and power start here.
long long floor_log10(const Number &magnitude);

} // namespace satellite::floats
