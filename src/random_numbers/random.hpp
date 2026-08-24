#pragma once

#include "satellite_number/bignum.hpp"

#include <string>

// satellite.random — DESIGN §18.
//
// Three tiers that differ in nothing a program can see except how long they
// take. Each one seeds a pcg32_k16384 from kernel entropy, throws draws away
// for a randomised span of milliseconds, folds what it threw away into an
// accumulator, reseeds from that accumulator and answers from the second
// generator.
//
// The tiers are a STATISTICAL character, not a security property. PCG makes no
// cryptographic claim and its state is recoverable from its output, so no tier
// here is "secure" and none of them is described that way — a longer spin buys
// a wider spread of possible seeds, and nothing else. §18 records what each
// step was measured to be worth.
//
// The public surface is two shapes per tier:
//
//     satellite.random.ultra(40)             uniform over [0, 10^40)
//     satellite.random.ultra.range(1, 100)   uniform over [1, 100], inclusive
//
// Both answer a satellite.variable.number at full precision. The 32-bit bound a
// generator offers cannot express either one, so the draw itself happens in
// src/satellite_number/random.cpp and this file only decides how long to spin first.

namespace satellite {

enum class RandomTier {
    Fast,     //   50-100 ms
    Normal,   //  250-300 ms
    Ultra,    // 2000-3000 ms
};

// The tier a path segment names — "fast", "normal" or "ultra". False for
// anything else, which is what makes satellite.random.quick a "no such module
// function" rather than a fourth tier nobody wrote.
bool random_tier(const std::string &word, RandomTier &out);

// The throwaway window, in milliseconds, for errors and for the tests.
void random_window(RandomTier tier, long long &minimum_ms, long long &maximum_ms);

// Uniform over [0, 10^digits). `digits` must be in
// [0, Number::MAX_RANDOM_DIGITS]; the caller checks, because only the caller
// can say which argument was wrong.
//
// About one draw in ten of a `digits`-digit request renders with FEWER digits
// than that, because a leading zero is not printed and a uniform draw has one
// a tenth of the time. That is what uniform means and it is not a defect, but
// it is not what the name promises either, so §18 says it out loud.
bool random_digits(RandomTier tier, int digits, Number &out);

// Uniform over [low, high], inclusive at BOTH ends. Both bounds must be
// integers and low must not exceed high; the caller checks. False when the span
// is wider than Number::MAX_RANDOM_DIGITS decimal digits.
bool random_range(RandomTier tier, const Number &low, const Number &high,
                  Number &out);

} // namespace satellite
