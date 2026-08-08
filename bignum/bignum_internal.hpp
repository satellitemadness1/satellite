#pragma once

// Private to bignum/. The public surface is ../bignum.hpp and it has not
// changed.
//
// bignum.cpp was 855 lines. Split by what the code already separated with its
// own section banners: the limb arithmetic underneath, and Number's four faces
// on top -- construction and parsing, queries and rounding, arithmetic, and
// §8.1.1 rendering.
//
// POW10 and scale_ll were in an anonymous namespace when this was one
// translation unit. scale_ll is used by both compare() and add(), which are now
// in different files, so it needs external linkage.

#include "../bignum.hpp"

#include <algorithm>
#include <cstdlib>

namespace satellite {

// 10^k for k in [0, 18], which is every power of ten a long long holds.
constexpr long long POW10[] = {
    1LL,
    10LL,
    100LL,
    1000LL,
    10000LL,
    100000LL,
    1000000LL,
    10000000LL,
    100000000LL,
    1000000000LL,
    10000000000LL,
    100000000000LL,
    1000000000000LL,
    10000000000000LL,
    100000000000000LL,
    1000000000000000LL,
    10000000000000000LL,
    100000000000000000LL,
    1000000000000000000LL,
};
constexpr int MAX_POW10 = 18;

// v * 10^k, refusing rather than wrapping. The refusal is what sends the
// operation down the BigInt path instead of producing a wrong answer.
bool scale_ll(long long v, int k, long long &out);

} // namespace satellite
