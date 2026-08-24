#pragma once

#include <climits>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

// satellite.variable.number — exact arbitrary-precision decimal, §8.1.
//
// "Technically infinite" means exact decimal: a bignum significand times a
// power of ten. Not rationals (denominators grow without bound and collide with
// the memory watchdog), not integer-only (1/3 -> 0 is unacceptable), and not
// double, which is the representation this replaces.
//
// What the change buys, concretely: 0.1 + 0.2 is 0.3 rather than
// 0.30000000000000004, 2000000 + 1 is not rounded away, and a count of
// nanoseconds divided by 1000000000 is an exact number of seconds rather than
// the nearest double to one. §8.1.1's rendering rule — "print the value", never
// "print N significant digits" — was phrased to survive exactly this migration,
// and it does: every program written against the double version prints what it
// printed before, minus the float noise.
//
// WHAT IS AND IS NOT EXACT
// ------------------------
// Addition, subtraction and multiplication are always exact, with no bound on
// the number of digits. So is division whose result terminates, which includes
// every division by a power of ten. Division that does NOT terminate — 1/3 — is
// rounded to a set number of significant digits, because there is no other
// choice available to anyone; the count is satellite.library.system.
// division_digits and it defaults to 34, decimal128's precision.
//
// SIZE
// ----
// §8.1 verified the shape: `long long sig; int exp; shared_ptr<const BigInt>
// big` is 32 bytes, so sizeof(ValueBase) stays 40 with SatString's 32 still
// dominating. Replacing a double with this costs the variant nothing.

#include "satellite_number/bignum_bigint.hpp"
#include "satellite_number/bignum_number.hpp"
