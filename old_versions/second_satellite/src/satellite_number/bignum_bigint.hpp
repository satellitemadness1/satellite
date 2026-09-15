#pragma once

// Part of satellite_number/bignum.hpp -- include that, which includes this.
// Split out only to keep every file under the line target; the declarations
// below are the first satellite's, with two changes and both are named here.
//
// THIS IS THE ONE PLACE satellite_number REACHES OUTSIDE ITSELF, AND IT IS A
// CORRECTION TO PLAN §6.1 RATHER THAN A COST. That section and
// SCRATCH.md/PORTING.md §2 both say this module "ports alone" -- every include
// a sibling or the standard library -- and in the first satellite it did,
// because it carried its own `Bits32` and its own MAX_RANDOM_DIGITS. This tree
// already has both, in satellite_random/random.hpp, which landed at M2 ahead of
// any milestone that calls it. Bringing v1's copies across would put two facts
// in two places each, against FORMAT/CXX.md §1's second rule, and the failure
// that follows is the one that registry note already describes: two ceilings
// that agree until somebody edits one.
//
// It costs nothing else. random.hpp deliberately names no PCG type -- that is
// what it was written for -- so including it here pulls in <cstdint> and
// <memory> and no Apache-2.0 header. pcg/README.md §"The licence problem" is
// the argument and this is its first consumer.

#include "satellite_random/random.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace satellite {

// Arbitrary-precision unsigned integer, base 10^9, little-endian limbs.
//
// Base 10^9 rather than 2^32, per DESIGN §8.1, because this backs a DECIMAL
// type: decimal input, decimal output and scaling by 10^k are most of the work,
// and all three are limb shifts in base 10^9 against full base conversions in
// base 2^32. 10^9 is also the largest power of ten whose products fit in a
// uint64 (10^18 < 2^64), so multiplication needs no intermediate wider than the
// machine already has.
//
// UNSIGNED, AND AFTER M8 THAT IS TRUE WITHOUT QUALIFICATION. v1 had exactly one
// signedness opinion in this class -- fits_ll()/to_ll(), which asked whether a
// magnitude fit a LONG LONG -- and it existed because v1's Number smuggled its
// sign into the significand, so the boxed and small forms had to meet at a
// signed type. DESIGN §8.1 takes the sign out; the pair below is the same code
// asking the question the class can actually answer, and the conversion to a
// signed integer is Number::to_integer's, one layer up, where the sign lives.
class BigInt {
public:
    static constexpr unsigned BASE = 1000000000u;
    static constexpr int BASE_DIGITS = 9;

    BigInt() = default;

    static BigInt from_u64(unsigned long long value);
    static BigInt from_digits(const char *digits, size_t count);

    bool is_zero() const { return limbs_.empty(); }

    // Decimal digits, which is what a decimal type actually needs to know.
    size_t digit_count() const;

    // Limbs, which is what a MEMORY question needs to know, and it is NOT
    // digit_count() divided by nine: trim() drops leading zero limbs, so the
    // count is a fact about the storage rather than a fact about the value.
    size_t limb_count() const { return limbs_.size(); }

    static int compare(const BigInt &a, const BigInt &b);
    static BigInt add(const BigInt &a, const BigInt &b);
    static BigInt sub(const BigInt &a, const BigInt &b);   // requires a >= b
    static BigInt mul(const BigInt &a, const BigInt &b);
    static BigInt mul_pow10(const BigInt &a, unsigned k);

    // Truncating division. `b` must be non-zero; the caller checks, because
    // "division by zero" is a satellite error with a span attached and this
    // layer has no way to raise one.
    static void divmod(const BigInt &a, const BigInt &b, BigInt &q, BigInt &r);

    // Uniform over [0, bound), by rejection sampling. `bound` must not be zero;
    // the caller checks, for the same reason divmod's does.
    //
    // The bit-length algorithm everybody reaches for first -- draw
    // bit_length(bound) bits, reject if the result is not below bound -- assumes
    // a BINARY bignum, and DESIGN §8.1 deliberately made this one base 10^9. So
    // a bit length is not a thing this representation knows, and computing one
    // costs more than the sampling does. The limb-aligned form has the SAME
    // guarantee for the same reason: the top limb is drawn over [0, top+1)
    // rather than the full base, so the sampling space is at most (top+1)/top
    // times the bound, which is at most 2x when top is 1.
    static BigInt random_below(const BigInt &bound, Bits32 &bits);

    // "0" when zero; never a leading zero otherwise.
    std::string to_digits() const;

    // Whether the magnitude fits the small form's significand, and its value
    // there. See the class note: this pair is v1's fits_ll()/to_ll() with the
    // sign taken out of the question.
    bool fits_u64() const;
    unsigned long long to_u64() const;

private:
    static BigInt mul_small(const BigInt &a, unsigned m);
    void trim();

    std::vector<unsigned> limbs_;
};

} // namespace satellite
