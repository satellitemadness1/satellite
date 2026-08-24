#pragma once

// Part of satellite_number/bignum.hpp -- include that, which includes this.
// Split out only to keep every file under the line ceiling; the declarations
// below are byte-for-byte what bignum.hpp held.

#include <string>
#include <vector>

namespace satellite {

// A source of 32-bit draws, and the ONLY thing this file knows about a random
// number generator.
//
// It is here rather than in random.hpp because the sampler below is bignum work
// — rejection sampling over limbs — and putting it the other way round would
// make bignum.hpp include the PCG headers, which are a vendored third-party
// tree that nothing else in the interpreter has any business seeing. One
// virtual call per limb is not measurable against a call that has just spent
// between 50 and 3000 ms throwing draws away on purpose (§18).
class Bits32 {
public:
    virtual ~Bits32() = default;

    // Uniform over the whole 32-bit range. A generator that cannot promise that
    // is not one this sampler's uniformity argument holds for.
    virtual unsigned next() = 0;
};

// Arbitrary-precision unsigned integer, base 10^9, little-endian limbs.
//
// Base 10^9 rather than 2^32, per §8.1, because this backs a DECIMAL type:
// decimal input, decimal output and scaling by 10^k are most of the work, and
// all three are limb shifts in base 10^9 against full base conversions in base
// 2^32. 10^9 is also the largest power of ten whose products fit in a uint64
// (10^18 < 2^64), so multiplication needs no intermediate wider than the
// machine already has.
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
    // §8.7's model bills a number by this and never by its digits.
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
    // §18 wanted this in BITS: draw bit_length(bound) of them, reject if the
    // result is not below bound, which rejects at most half the time. That
    // algorithm assumes a binary bignum, and §8.1 deliberately made this one
    // base 10^9 — so a bit length is not a thing this representation knows, and
    // computing one costs more than the sampling does. The limb-aligned form
    // below has the SAME guarantee for the same reason: the top limb is drawn
    // over [0, top+1) rather than the full base, so the sampling space is at
    // most (top+1)/top times the bound, which is at most 2x when top is 1.
    static BigInt random_below(const BigInt &bound, Bits32 &bits);

    // "0" when zero; never a leading zero otherwise.
    std::string to_digits() const;

    bool fits_ll() const;
    long long to_ll() const;

private:
    static BigInt mul_small(const BigInt &a, unsigned m);
    void trim();

    std::vector<unsigned> limbs_;
};

} // namespace satellite
