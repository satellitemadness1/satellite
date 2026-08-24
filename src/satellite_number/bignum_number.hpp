#pragma once

// Part of satellite_number/bignum.hpp -- include that, which includes this.
// Split out only to keep every file under the line ceiling; the declarations
// below are byte-for-byte what bignum.hpp held.

#include "satellite_number/bignum_bigint.hpp"

#include <climits>
#include <memory>
#include <string>
#include <type_traits>

namespace satellite {

class Number {
public:
    // Non-terminating division rounds to this many significant digits unless
    // satellite.library.system.division_digits says otherwise. 34 is
    // decimal128's precision: wide enough that ordinary arithmetic never
    // notices, narrow enough that 1/3 is readable.
    static constexpr int DEFAULT_DIVISION_DIGITS = 34;
    static constexpr int MAX_DIVISION_DIGITS = 10000;

    // The widest draw satellite.random will make, in decimal digits, and the
    // ceiling on how wide a `.range` may be. It is a REFUSAL and not a clamp:
    // a program that asks for a million digits has made a mistake, and silently
    // handing back a hundred thousand would hide it.
    //
    // 100000 rather than a round power of two because the unit it bounds is
    // decimal digits. One draw at the ceiling is ~11k limbs and about 44 KB
    // that a program then has to do something with; the arithmetic underneath
    // is exact at any size, so the limit is about what a caller can use rather
    // than about what this file can compute.
    static constexpr int MAX_RANDOM_DIGITS = 100000;

    Number() = default;

    // Integral only — and the float overloads are DELETED rather than simply
    // absent. §8.1 verified both halves of that by compiling them:
    //
    //   * with a plain Number(long long) and no float overload, `Value v = 3.14`
    //     compiles and silently truncates to 3, with zero warnings under this
    //     project's actual -Wall -Wextra;
    //   * with `Number(double) = delete` alone, `Number(42)` breaks, because a
    //     deleted overload still participates in overload resolution.
    //
    // The constrained template plus the deleted floats is the pair that gives
    // an error on 3.14 and an integer on 42. `bool` is excluded so that
    // `Value v = true` still selects the variant's bool alternative rather than
    // becoming the number 1.
    template <class T,
              class = std::enable_if_t<
                  std::is_integral_v<T> &&
                  !std::is_same_v<std::remove_cv_t<T>, bool>>>
    Number(T value)
    {
        if constexpr (std::is_signed_v<T>) {
            const long long narrow = static_cast<long long>(value);
            // LLONG_MIN has no positive counterpart, so negated() on it would
            // be undefined. Sending it big is what lets every other operation
            // assume `sig_` can be negated.
            if (narrow == LLONG_MIN)
                *this = make(-1, BigInt::from_u64(
                                     static_cast<unsigned long long>(LLONG_MAX) + 1),
                             0);
            else
                sig_ = narrow;
        } else {
            const unsigned long long wide =
                static_cast<unsigned long long>(value);
            if (wide <= static_cast<unsigned long long>(LLONG_MAX))
                sig_ = static_cast<long long>(wide);
            else
                *this = from_u64(wide);
        }
    }

    Number(float) = delete;
    Number(double) = delete;
    Number(long double) = delete;

    static Number from_u64(unsigned long long value);

    // digits [ '.' digits ] [ ('e'|'E') ['+'|'-'] digits ]. False, with `out`
    // untouched, on anything else.
    static bool parse(const std::string &text, Number &out);

    // §8.1.1: print the VALUE. Fixed notation inside [1e-6, 1e21), scientific
    // outside it, where digits stop being informative.
    std::string to_string() const;

    bool is_zero() const { return sign() == 0; }
    bool is_negative() const { return sign() < 0; }

    // True when the value has no fractional part, which is what an index, a
    // slice bound and an exit status all require.
    bool is_integer() const;

    // Exact integer conversion. False when there is a fractional part or the
    // value does not fit, so a caller never has to guess whether it was
    // truncated or rounded.
    bool to_integer(long long &out) const;

    // The decimal digits the value is WRITTEN with, from its most significant
    // digit down to its least: 100 has three, 1230 has four, 12.5 has three,
    // and zero has one because "0" is a digit. Not the significand's count --
    // 100 normalizes to 1e2, and a .digits() that answered 1 there was
    // reporting the storage rather than the number. Leading zeros are padding
    // and not counted, so 0.001 has one.
    //
    // This is what `.digits()` answers, and it is deliberately NOT what
    // `.length()` answers. §17's registry made `has` a new word rather than a
    // reuse of `contains` on the principle that one word means one thing;
    // `length` means "how many items" on a string, a list and a map, and a
    // number holds no items. So `.length()` on a number is an error that names
    // this method, which is a better outcome than a second meaning.
    size_t digit_count() const;

    // Bytes this number owns BEYOND its 40-byte Value node, under §8.7's
    // model. Zero for the small form — a significand and an exponent fit in
    // the node, which is the whole point of that form — and four per limb once
    // the magnitude is boxed.
    size_t payload_bytes() const;

    // Identity of the boxed magnitude, or null when there is none. The ONLY
    // thing a caller may do with this is compare it: §8.7 counts shared
    // storage once, and two numbers copied from one another share exactly this
    // pointer. It is not a handle to read through, and the const void * says so.
    const void *payload_id() const { return big_.get(); }

    // The small form, and the ONLY sanctioned window onto it. §17's register
    // type holds a number inline as significand-and-exponent, and these two are
    // how it gets in and out.
    //
    // They exist as members rather than as a caller reading sig_ and exp_
    // because reading those directly is wrong in a way that type-checks: when
    // big_ is set, sig_ holds the SIGN, not the significand (see make()). A
    // caller who forgot that would read 1 or -1 as the value of every large
    // number, silently, and only for numbers big enough that a test with small
    // inputs would never show it.
    //
    // small_parts returns false — leaving sig and exp untouched — for exactly
    // the numbers where the small form does not exist. The register type is
    // then obliged to box, which is §17's promotion.
    bool small_parts(long long &sig, int &exp) const;

    // The inverse. Not a constructor, because it is not a conversion anyone
    // should reach for by accident: it takes the internal representation, and
    // the only caller that should have it is one that got it from small_parts.
    static Number from_small(long long sig, int exp);

    Number abs() const;
    Number negated() const;
    Number floor() const;
    Number ceil() const;
    Number round() const;     // half away from zero

    static int compare(const Number &a, const Number &b);

    static Number add(const Number &a, const Number &b);
    static Number sub(const Number &a, const Number &b);
    static Number mul(const Number &a, const Number &b);

    // Both require a non-zero `b`; the caller checks and raises the satellite
    // error, because only the caller has the span to attach it to.
    static Number divide(const Number &a, const Number &b, int digits);
    static Number modulo(const Number &a, const Number &b);

    // Uniform over [0, bound). False, leaving `out` untouched, when `bound` is
    // not a positive integer or is wider than MAX_RANDOM_DIGITS — the caller
    // turns that into a satellite error, because only the caller has a span to
    // hang one on.
    //
    // This is the whole of satellite.random's arbitrary precision: the 32-bit
    // bound a generator offers cannot express 10^40, let alone a range between
    // two numbers a program wrote down, so the draw has to happen out here
    // where the digits live.
    static bool random_below(const Number &bound, Bits32 &bits, Number &out);

    // VALUE equality, not representation equality, and it has to be spelled out
    // because the two differ: 1 is `sig 1 exp 0` and 1.0 is `sig 10 exp -1`.
    // The defaulted member-wise operator== would call those unequal, and Value
    // is a std::variant whose own operator== would then inherit the mistake.
    bool operator==(const Number &other) const
    {
        return compare(*this, other) == 0;
    }

private:
    // floor, ceil and round all drop the fractional digits and differ only in
    // what they then do about what was dropped, so they share one body.
    enum class Rounding { Floor, Ceil, HalfAwayFromZero };
    Number rounded(Rounding mode) const;

    int sign() const;
    BigInt magnitude() const;

    // The magnitude scaled up to `target`, which must not exceed exp_.
    BigInt scaled_magnitude(int target) const;

    // The exact integer magnitude, for the random sampler. False when there is
    // a fractional part, or when the value needs more than `max_digits` decimal
    // digits — which is checked BEFORE the expansion, so `1e2000000000` is
    // refused rather than attempted.
    //
    // scaled_magnitude(0) is not this: it multiplies by 10^exp_ unsigned, so a
    // value stored as 1000e-1 — which is the integer 100 — would send it a
    // negative count cast to unsigned.
    bool integer_magnitude(BigInt &out, int max_digits) const;

    // Shrinks back to the small representation when the magnitude fits.
    static Number make(int sign, const BigInt &magnitude, long long exponent);

    // Trailing zeros stripped and the exponent raised to match, which is what
    // makes 2.50 print as 2.5 and 5.0 as 5.
    void normalized(std::string &digits, long long &exponent) const;

    // value = significand * 10^exp_, where the significand is `sig_` when big_
    // is null, and otherwise (sig_ < 0 ? -1 : +1) times *big_. A null big_ is
    // the fast path and the common one: it holds every value a loop counter,
    // an index or a literal takes, and costs no allocation and no atomic.
    long long sig_ = 0;
    int exp_ = 0;
    std::shared_ptr<const BigInt> big_;
};

} // namespace satellite
