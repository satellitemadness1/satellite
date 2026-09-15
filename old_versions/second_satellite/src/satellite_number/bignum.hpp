#pragma once

// satellite.variable.number -- exact arbitrary-precision decimal. DESIGN §8.1.
//
// The umbrella header, and the only door a consumer opens. Ported from the
// first satellite at M8; PLAN §6.1 surveyed it at 10 files and 1509 lines,
// internally closed, and it came across close to unchanged. What changed is the
// sign, and bignum_number.hpp is where that argument is written down.
//
// "Technically infinite" means exact decimal: a bignum significand times a
// power of ten. Not rationals (denominators grow without bound and collide with
// the memory watchdog), not integer-only (1/3 -> 0 is unacceptable), and not
// double, which is the representation this replaces.
//
// What the change buys, concretely: 0.1 + 0.2 is 0.3 rather than
// 0.30000000000000004, 2000000 + 1 is not rounded away, and a count of
// nanoseconds divided by 1000000000 is an exact number of seconds rather than
// the nearest double to one.
//
// WHAT IS AND IS NOT EXACT
// ------------------------
// Addition, subtraction and multiplication are always exact, with no bound on
// the number of digits. So is division whose result terminates, which includes
// every division by a power of ten. Division that does NOT terminate -- 1/3 --
// is rounded to a set number of significant digits, because there is no other
// choice available to anyone; the count comes from
// satellite.library.system.division_digits `1 14 2 1`, which M6 built the
// storage for and this milestone gives a meaning -- limits::division_digits().
//
// SIZE, AND IT IS THE ONE NUMBER THAT COULD HAVE MADE THIS PORT NOT FIT.
// PLAN §6.1's open question 4 asks for sizeof(Number) on arrival, because
// DESIGN §8.2 budgets a Value at 40 bytes. MEASURED HERE, 2026-08-31, before
// anything was written:
//
//     v1 as it stood        (sig, exp, ptr)              32
//     with an explicit bool positive_, flat              32
//     with the magnitude cut out as its own type         40
//
// So the sign is free and the port fits, and the SAME measurement decided the
// shape. The bool lands in the padding after `int exp_` and costs nothing;
// cutting a Magnitude type out -- which is the tempting way to make DESIGN
// §8.6's "one sign, held once" literally true for the float -- costs eight
// bytes, because a nested struct's tail padding is not reusable by the type
// holding it, and a 40-byte Number puts Value at 48. bignum_number.hpp holds
// the fields in the order that measurement requires.

#include "satellite_number/bignum_bigint.hpp"
#include "satellite_number/bignum_number.hpp"
