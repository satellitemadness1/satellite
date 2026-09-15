#pragma once

// Part of satellite_number/bignum.hpp -- include that, which includes this.
//
// THE SIGN IS AN EXPLICIT bool AND THAT IS THE WHOLE OF WHAT M8 CHANGED ABOUT
// THIS PORT. DESIGN §8.1: "A number's sign is an explicit
// satellite.variable.bool named `positive`, defaulting to true -- a number with
// no sign written is positive, and something has to flip the flag for it to be
// otherwise. The magnitude never carries a sign of its own." The first
// satellite did the opposite: it packed the sign into the significand, and when
// the magnitude was boxed `sig_` held 1 or -1 instead of a value.
//
// IT IS THE SAME MECHANISM DESIGN §8.6 GIVES A FLOAT, NOT A PARALLEL ONE, and
// that is why this milestone builds it rather than M15. A float is
// (positive, L, R) over two of these magnitudes; negation, abs, the sign of a
// product and the ordering of negatives are then written once and true of both.
// PLAN §8: "M15's float inherits it rather than defining a second one, which is
// the whole reason the two can be milestones apart instead of one large one."
//
// WHAT IT COST AND WHAT IT PAID FOR, because neither is obvious.
//
//   * add() has to compare and conditionally swap when the signs differ, where
//     v1 added two signed long longs and checked for overflow. DESIGN §8.6
//     names that as "the one cost of sign-magnitude" and takes the trade.
//   * AND THE OPPOSITE-SIGN CASE CAN NO LONGER OVERFLOW AT ALL, which is the
//     half that argument does not mention. Subtracting the smaller unsigned
//     magnitude from the larger always fits, so `1e18 + -1e18` stays in the
//     small form where v1's signed add had to fall through to the bignum path
//     whenever the sum left the range. More additions are allocation-free after
//     this change than before it, not fewer.
//   * LLONG_MIN STOPS BEING A SPECIAL CASE. v1 carried three separate blocks --
//     in the constructor, in from_small() and in magnitude() -- because
//     -LLONG_MIN has no long long to land in, so "sig_ can be negated" was an
//     invariant every other operation leaned on. An unsigned magnitude holds
//     2^63 and the whole argument dissolves; the code is shorter than what it
//     replaces.
//   * THE POWERS OF TEN GAIN A ROW. bignum_internal.hpp's table ran to 10^18,
//     the largest that fits a long long. Unsigned reaches 10^19, so one more
//     scale stays in the allocation-free path.
//
// THERE IS NO NEGATIVE ZERO AND IT IS STRUCTURAL RATHER THAN CHECKED. make()
// returns a default-constructed Number the moment the magnitude is zero, and a
// default Number is positive -- so no path in this module can build one, and
// DESIGN §8.6's invariant 3 is a thing the representation makes unsayable
// rather than a thing a comparison has to repair. Note the contrast the float
// will NOT inherit: §8.6 warns that a zero-initialised float struct reads as
// negative zero, because its `positive` has to be set by construction. Here
// `positive_` defaults to true, so a zero-initialised Number is the number 0.

#include "satellite_number/bignum_bigint.hpp"

#include <climits>
#include <memory>
#include <string>
#include <type_traits>

namespace satellite {

class Number {
public:
    // THERE IS NO CEILING ON A DIVISION AND THERE WAS ONE UNTIL 2026-08-31.
    // `kMaxDivisionDigits = 10000` sat here and divide() clamped to it, so a
    // file asking for 50,000 digits got 10,000 and nothing said so. DESIGN §7.5
    // is what that broke -- "no constant in a header deciding how big a thing
    // the user may write" -- and it is the same sentence errors.def's S06xx
    // block makes about there being no overflow row: a number's size is bounded
    // by memory, and a division to N digits is a number of size N. What ends a
    // runaway is M6's watchdog at MEMORY_MAX.
    //
    // THE DEFAULT IS NOT HERE EITHER, AND THAT IS THE SEAM M6 DREW. v1 kept a
    // DEFAULT_DIVISION_DIGITS beside this because its evaluator read the library
    // namespace itself. Here the count comes from
    // satellite.library.system.division_digits `1 14 2 1`, and
    // machine_limits/limits.hpp's division_digits() is where 34 lives and why.
    // divide() takes a count, never invents one, and no longer overrules one.

    Number() = default;

    // Integral only -- and the float overloads are DELETED rather than simply
    // absent. DESIGN §8.1 verified both halves of that by compiling them:
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
            positive_ = narrow >= 0;
            // Taken in unsigned so that LLONG_MIN has somewhere to land. v1
            // had to send this value down the BigInt path instead; the
            // magnitude being unsigned is what removes the special case.
            sig_ = narrow < 0 ? 0ULL - static_cast<unsigned long long>(narrow)
                              : static_cast<unsigned long long>(narrow);
        } else {
            sig_ = static_cast<unsigned long long>(value);
        }
    }

    Number(float) = delete;
    Number(double) = delete;
    Number(long double) = delete;

    static Number from_u64(unsigned long long value);

    // digits [ '.' digits ] [ ('e'|'E') ['+'|'-'] digits ]. False, with `out`
    // untouched, on anything else.
    static bool parse(const std::string &text, Number &out);

    // Print the VALUE, never N significant digits. Fixed notation until the
    // PADDING would outnumber the information; render.cpp has the rule and why
    // it is phrased that way rather than as a fixed range.
    std::string to_string() const;

    // THE SIGN, READ DIRECTLY. DESIGN §8.1 names the field, so this names it
    // too -- `positive: true` is one thought and `negative: false` is two, and
    // §1.1 spends the language's cleverness on the reader.
    //
    // Zero is positive, always, and callers may rely on it: there is no
    // negative zero to test for.
    bool positive() const { return positive_; }

    bool is_zero() const { return !big_ && sig_ == 0; }
    bool is_negative() const { return !positive_ && !is_zero(); }

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
    // `1 6 4 15`, WHICH WAS APPENDED FOR IT ON 2026-08-31. v1 exposes this as
    // `.digits()` and WORD_NUMBERS §2.2 had no row for it, so it was a C++
    // method the language could not call -- the mirror image of the two shifts,
    // which had numbers and no meaning. Appending is what §1.2 permits and
    // renumbering is what it forbids, so the row went on the end.
    size_t digit_count() const;

    // Bytes this number owns BEYOND its 40-byte Value node. Zero for the small
    // form -- a significand and an exponent fit in the node, which is the whole
    // point of that form -- and four per limb once the magnitude is boxed.
    size_t payload_bytes() const;

    // Identity of the boxed magnitude, or null when there is none. The ONLY
    // thing a caller may do with this is compare it: shared storage is counted
    // once, and two numbers copied from one another share exactly this pointer.
    // It is not a handle to read through, and the const void * says so.
    const void *payload_id() const { return big_.get(); }

    // The small form, and the ONLY sanctioned window onto it. DESIGN §8.2's
    // inline value holds a number as sign-significand-and-exponent, and these
    // two are how it gets in and out.
    //
    // They exist as members rather than as a caller reading the fields because
    // reading them directly is wrong in a way that type-checks: when big_ is
    // set, `sig_` is not the magnitude and holds zero. A caller who forgot
    // would read every large number as 0 -- silently, and only for numbers big
    // enough that a test with small inputs would never show it.
    //
    // ZERO RATHER THAN v1's SIGN, AND THE DIFFERENCE IS WORTH ONE LINE. v1 put
    // 1 or -1 in the significand when it boxed, so the same mistake read every
    // large number as 1. Both are wrong; this one is wrong the same way every
    // time and cannot be mistaken for a plausible small value.
    //
    // small_parts returns false -- leaving its outputs untouched -- for exactly
    // the numbers where the small form does not exist.
    bool small_parts(bool &positive, unsigned long long &sig, int &exp) const;

    // The inverse. Not a constructor, because it is not a conversion anyone
    // should reach for by accident: it takes the internal representation, and
    // the only caller that should have it is one that got it from small_parts.
    static Number from_small(bool positive, unsigned long long sig, int exp);

    Number abs() const;
    Number negated() const;
    Number floor() const;
    Number ceil() const;
    Number round() const;     // half away from zero

    // `1 6 4 2`, `1 6 4 3` and `1 6 4 5`. NEW AT M8 AND NOT PORTED: the first
    // satellite's number surface was abs, ceil, floor, round, to_string,
    // digits and negate, and it had none of these three. They are one
    // comparison each and they are here rather than at a call site so that the
    // three paths cannot disagree about what a tie does.
    //
    // BY VALUE, DELIBERATELY, AND number_query.cpp CARRIES THE ARGUMENT: a
    // reference-returning max is the standard library's oldest dangling
    // temporary and this is not a hot path.
    static Number max(const Number &a, const Number &b);
    static Number min(const Number &a, const Number &b);
    static Number clamp(const Number &a, const Number &low, const Number &high);

    static int compare(const Number &a, const Number &b);

    static Number add(const Number &a, const Number &b);
    static Number sub(const Number &a, const Number &b);
    static Number mul(const Number &a, const Number &b);

    // Both require a non-zero `b`; the caller checks and raises the satellite
    // error, because only the caller has the span to attach it to.
    //
    // `digits` IS THE CALLER'S AND IS NOT SECOND-GUESSED -- unsigned, with no
    // upper bound at all, per the note where kMaxDivisionDigits used to be. A
    // zero is the one count divide() cannot answer, and it is treated exactly as
    // a zero divisor is: machine_limits refuses it at the file, where a caret
    // can land under it.
    static Number divide(const Number &a, const Number &b, unsigned digits);

    // `1 6 4 12`. EXACT AND IT NEVER ROUNDS -- DESIGN §8.6's own sentence, and
    // why this is M8's rather than M15's. `a % b` is `a - b * trunc(a/b)`; the
    // quotient is only ever wanted as an integer, so the inexact tail of the
    // division is discarded before it can matter. Truncated and not floored, so
    // the result takes the sign of `a`.
    static Number modulo(const Number &a, const Number &b);

    // `1 6 4 1` and `1 6 4 11`. A shift is MULTIPLICATION AND DIVISION BY A
    // POWER OF TWO, decided 2026-08-31 and specified in DESIGN §5.5: 2 divides
    // 10, so both directions terminate and neither rounds, and a fractional
    // receiver needs no rule of its own -- 7.5 shifted left once is exactly 15
    // and right once is exactly 3.75.
    static Number shift_left(const Number &a, unsigned n);
    static Number shift_right(const Number &a, unsigned n);

    // Uniform over [0, bound). False, leaving `out` untouched, when `bound` is
    // not a positive integer or is wider than MAX_RANDOM_DIGITS -- the caller
    // turns that into a satellite error, because only the caller has a span to
    // hang one on.
    //
    // This is the whole of satellite.random's arbitrary precision: the 32-bit
    // bound a generator offers cannot express 10^40, let alone a range between
    // two numbers a program wrote down, so the draw has to happen out here
    // where the digits live. satellite_random/random.hpp owns the ceiling and
    // the seam; this owns the digits.
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

    // -1, 0 or +1, DERIVED and never stored. The sign is `positive_` and the
    // zero test is the fields; this is the two of them in the shape an ordering
    // wants. Keeping it as a function is what stops the sign having a second
    // home.
    int sign() const { return is_zero() ? 0 : (positive_ ? 1 : -1); }

    BigInt magnitude() const;

    // The magnitude scaled up to `target`, which must not exceed exp_.
    BigInt scaled_magnitude(int target) const;

    // The exact integer magnitude, for the random sampler. False when there is
    // a fractional part, or when the value needs more than `max_digits` decimal
    // digits -- which is checked BEFORE the expansion, so `1e2000000000` is
    // refused rather than attempted.
    //
    // scaled_magnitude(0) is not this: it multiplies by 10^exp_ unsigned, so a
    // value stored as 1000e-1 -- which is the integer 100 -- would send it a
    // negative count cast to unsigned.
    bool integer_magnitude(BigInt &out, long long max_digits) const;

    // Shrinks back to the small representation when the magnitude fits, and the
    // one place "zero is positive" is enforced.
    static Number make(bool positive, const BigInt &magnitude,
                       long long exponent);

    // The small form, built directly. Same two rules make() applies -- zero
    // carries no sign and no exponent -- without going through a BigInt.
    static Number small(bool positive, unsigned long long sig, int exp);

    // Trailing zeros stripped and the exponent raised to match, which is what
    // makes 2.50 print as 2.5 and 5.0 as 5.
    void normalized(std::string &digits, long long &exponent) const;

    // value = (positive_ ? +1 : -1) * significand * 10^exp_, where the
    // significand is `sig_` when big_ is null and *big_ otherwise. A null big_
    // is the fast path and the common one: it holds every value a loop counter,
    // an index or a literal takes, and costs no allocation and no atomic.
    //
    // THE FIELD ORDER IS LOAD-BEARING AND bignum.hpp HAS THE MEASUREMENT.
    // `positive_` sits after `exp_` so that it lands in the padding an int
    // leaves before an eight-byte pointer. Moved anywhere else this class is 40
    // bytes and DESIGN §8.2's Value budget is 48.
    unsigned long long sig_ = 0;
    int exp_ = 0;
    bool positive_ = true;
    std::shared_ptr<const BigInt> big_;
};

// DESIGN §8.2 budgets a Value at 40 bytes and PLAN §6.1 calls this the one
// number that could make the port not fit. It is checked here rather than
// described, because FORMAT/CXX.md §1's third rule is that a fact anything
// breaks on has to be somewhere a static_assert can see it.
static_assert(sizeof(Number) == 32,
              "Number must stay 32 bytes so a Value fits DESIGN §8.2's 40");

} // namespace satellite
