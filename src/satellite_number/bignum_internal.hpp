#pragma once

// Private to src/satellite_number/. The public surface is
// satellite_number/bignum.hpp and it has not changed.
//
// The first satellite's bignum.cpp was 855 lines. It is split by what the code
// already separated with its own section banners: the limb arithmetic
// underneath, and Number's four faces on top -- construction and parsing,
// queries and rounding, arithmetic, and rendering.
//
// POW10 and the scaling helper were in an anonymous namespace when this was one
// translation unit. The helper is used by both compare() and add(), which are
// in different files, so it needs external linkage.

#include "satellite_number/bignum.hpp"

#include <algorithm>
#include <cstdlib>

namespace satellite {

// 10^k for k in [0, 19], which is every power of ten an unsigned long long
// holds.
//
// NINETEEN ROWS AND NOT EIGHTEEN, WHICH IS ONE OF THE THINGS THE SIGN CHANGE
// PAID FOR. v1's table stopped at 10^18 because its significand was a signed
// long long and 10^19 does not fit one. DESIGN §8.1 takes the sign out of the
// magnitude, so the ceiling is ULLONG_MAX (about 1.8e19) and 10^19 is under it
// -- one more scale that stays in the allocation-free path.
constexpr unsigned long long POW10[] = {
    1ULL,
    10ULL,
    100ULL,
    1000ULL,
    10000ULL,
    100000ULL,
    1000000ULL,
    10000000ULL,
    100000000ULL,
    1000000000ULL,
    10000000000ULL,
    100000000000ULL,
    1000000000000ULL,
    10000000000000ULL,
    100000000000000ULL,
    1000000000000000ULL,
    10000000000000000ULL,
    100000000000000000ULL,
    1000000000000000000ULL,
    10000000000000000000ULL,
};
constexpr int MAX_POW10 = 19;

static_assert(sizeof(POW10) / sizeof(POW10[0]) == MAX_POW10 + 1,
              "MAX_POW10 indexes POW10 and the two must not drift");

// v * 10^k, refusing rather than wrapping. The refusal is what sends the
// operation down the BigInt path instead of producing a wrong answer.
//
// A MAGNITUDE AND NOT A VALUE, which is the whole of what changed here: the
// sign is Number's `positive_` and never reaches this function, so the overflow
// check is the unsigned one and there is no negation to be undefined.
bool scale_u64(unsigned long long v, int k, unsigned long long &out);

} // namespace satellite
