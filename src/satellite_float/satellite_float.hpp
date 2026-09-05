#pragma once

// satellite.variable.float `1 6 10` -- DESIGN §8.6's type: a bool and two
// satellite_numbers. PLAN M15, and MILESTONES/M15.md is the review.
//
//     float = (positive, L, R)      value = (positive ? +1 : -1) x (L + R)
//
// THE THREE INVARIANTS ARE §8.6's AND CONSTRUCTION IS WHERE THEY HOLD. `L` and
// `R` are magnitudes and never carry a sign; `R < 1`; and `positive` is true
// when both halves are zero, so there is no negative zero for QUAD's seven
// sort comparators to trip on. Every path into a Float runs through
// from_number() or assemble(), and both normalize -- the way §8.1's Number
// guards its own construction. The default constructor is a positive zero,
// which is why the flag has a default member initialiser: §8.6 warns that a
// ZERO-INITIALISED bare struct would read as negative zero, and giving the
// compiler the true default is what makes that state unreachable through any
// constructor. (`memset` over one of these is still detectable, not silent --
// the warning stands for anyone holding raw bytes.)
//
// THE LEFT HALF IS NEVER ROUNDED AND THE RIGHT HALF ALWAYS MAY BE, which is
// the entire reason the type is two numbers rather than one. §8.1's Number
// bounds significant digits across the whole value; QUAD's `activation *=
// keep` grows digits DOWNWARD every tick, so the bound belongs downward and
// only downward. `R`'s digit count IS the value's precision -- places() --
// so precision travels with the value, and
// `satellite.library.system.float_digits` `1 14 2 4` is only the default for
// a result that would otherwise have less.
//
// THE ROUNDING RULE IS ROUND HALF AWAY FROM ZERO, DECIDED AT M15 (2026-09-04,
// delegated). The tree had already taken it three times -- Number::divide
// bumps its guard digit at `5`, Number::round() documents "half away from
// zero", and both are v1's behaviour ported with tests asserting it -- so the
// milestone's job, as at M12, was to notice rather than invent.
// MILESTONES/M15.md §2 carries the full argument and the two rules declined;
// DESIGN §8.6 no longer says "still open". Because the halves are magnitudes,
// half away from zero and half up are the same motion here: the sign is
// decided before any rounding runs and is never revisited.
//
// THE SIGN IS INHERITED AND NOT REDEFINED. add() and sub() below hand the
// whole value to Number's own signed arithmetic and split the answer, so the
// opposite-signs compare-and-swap §8.6 spells out runs in satellite_number/
// where M8 wrote it once -- "negation, abs, the sign of a product and the
// ordering of negatives are written once ... and are true of both types" is
// PLAN §8's sentence and this file is what makes it stay true.

#include "satellite_number/bignum.hpp"

#include <string>

namespace satellite {

class Float {
public:
    // A positive zero. The member initialisers ARE invariant 3.
    Float() = default;

    // DESIGN §8.6's conversion: exact and it always succeeds. `(n, 0)` for a
    // whole number; a fractional number splits at the point, also exactly.
    static Float from_number(const Number &value);

    // Sign, whole part, fractional part -- normalized on the way in: the
    // halves are taken as magnitudes, `R >= 1` carries into `L`, and a zero
    // comes out positive. This is §8.6's `normalize` and it cannot round.
    static Float assemble(bool positive, const Number &whole,
                          const Number &fraction);

    bool positive() const { return positive_; }
    const Number &left() const { return left_; }
    const Number &right() const { return right_; }

    bool is_zero() const { return left_.is_zero() && right_.is_zero(); }

    // Digits right of the decimal point -- THE VALUE'S PRECISION, §8.6's "R's
    // digit count is the float's precision". 3.14 has 2; a whole number has 0.
    // Trailing zeros do not survive Number's canonical form, so 0.50 is one
    // place, which the floor at float_digits absorbs on the next operation.
    unsigned places() const;

    // The whole value as one Number -- exact, because L + R is a finite
    // decimal by construction. This is how the four operations below reach
    // M8's arithmetic, and it is the one direction of conversion that needs
    // no rounding rule.
    Number to_number() const;

    // sign, L, '.', then R's digits -- "3.14", "-0.14", and "4.0" for a whole
    // float, one fractional digit minimum so a float never prints as a
    // number. satellite_value/render.cpp forwards here.
    std::string to_string() const;

    Float negated() const; // flip `positive`, unless the value is zero
    Float abs() const;     // one bool -- §8.6's "nearly free"

    // The right half rounded to `n` places, half away from zero, then the
    // carry (0.99 rounded to one place is 1.0). Fewer places than asked for
    // is left alone: rounding never manufactures digits.
    Float rounded_to(unsigned n) const;

    // Total and exact: -1, 0, +1. Invariant 3 is what makes it total -- there
    // is no -0 to be unequal to 0 (QUAD.md §3.3's comparators lean on this).
    static int compare(const Float &a, const Float &b);

    // THE FOUR OPERATIONS, §8.6. Addition and subtraction are EXACT and never
    // round -- the property `double` does not have -- so they take no digit
    // count. Multiplication and division round the right half to the result's
    // precision: max of the operands', floored at `float_digits`.
    static Float add(const Float &a, const Float &b);
    static Float sub(const Float &a, const Float &b);
    static Float mul(const Float &a, const Float &b, unsigned float_digits);

    // Correctly rounded, not merely guard-digited: the last place is settled
    // by exact cross-multiplication (float_arith.cpp), so a tie is a proved
    // tie. `b` must be non-zero; the caller checks and raises S0601, because
    // only the caller has the span to attach.
    static Float divide(const Float &a, const Float &b, unsigned float_digits);

private:
    bool positive_ = true;
    Number left_;
    Number right_;
};

// power and sqrt -- DESIGN §8.6's class-3 answers for the three
// `satellite.variable.number` methods that waited on the rounding rule.
// They answer a FLOAT: §8.6, "a result type that depended on the VALUE of an
// argument would make `a ^ b` mean two different things".
//
// AN OUTCOME AND NOT A THROW, dispatch.hpp's argument one seam further down:
// the caller turns an outcome into the S06xx row with the span only it holds.
enum class PowerOutcome {
    Answered,
    DividedByZero, // 0 to a negative exponent is 1 over 0 -- S0601
    NoRealAnswer,  // sqrt of a negative; a negative base to a fractional
                   // exponent -- S0602 / S0603
};

// base^exponent. Integer exponent >= 0 is exact repeated multiplication (the
// table's own word -- growth is the float multiplication's to bound, not
// this); negative is the reciprocal, which rounds; fractional peels the
// exponent a decimal digit at a time through 10th roots. 0^0 is 1.
PowerOutcome power_of(const Number &base, const Number &exponent,
                      unsigned float_digits, Float &out);

// Newton's method, then the last place settled by exact squaring -- so
// sqrt(6.25) is exactly 2.5 and a tie is a proved tie, same contract as
// divide. Negative is NoRealAnswer.
PowerOutcome sqrt_of(const Number &value, unsigned float_digits, Float &out);

} // namespace satellite
