// The four operations -- DESIGN §8.6, with the result-precision rule and the
// one place division's last digit is settled exactly.
//
// EVERY OPERATION IS COMPOSITION OVER THE EXACT JOIN. to_number() loses
// nothing, Number's arithmetic is M8's -- signed, exact where §8.1 promises
// exact -- and from_number() splits the answer back. So the same-sign and
// opposite-sign case analysis §8.6 spells out for addition, and the
// signs-agree rule it gives multiplication, run in satellite_number/ where
// they were written once; what this file owns is WHERE the rounding bites,
// which is §8.6's three classes made code. The four-products decomposition in
// the document is the argument for why growth is bounded, not a required
// shape for the arithmetic: an exact product rounded once is digit-for-digit
// the same answer.

#include "satellite_float/float_internal.hpp"
#include "satellite_float/satellite_float.hpp"

namespace satellite {

namespace {

// "The result's precision is `max` of the operands', floored at
// `float_digits`. So precision never silently shrinks and never grows without
// bound." -- §8.6. Only multiplication and division consult this; addition
// and subtraction are exact and keep what falls out.
unsigned result_places(const Float &a, const Float &b, unsigned float_digits)
{
    const unsigned wider = a.places() > b.places() ? a.places() : b.places();
    return wider > float_digits ? wider : float_digits;
}

} // namespace

Float Float::add(const Float &a, const Float &b)
{
    // EXACT AND IT NEVER ROUNDS -- the property `double` does not have, free
    // here because two n-place fractions sum to at most n places plus a carry
    // that normalize moves into the unbounded half.
    return from_number(Number::add(a.to_number(), b.to_number()));
}

Float Float::sub(const Float &a, const Float &b)
{
    return from_number(Number::sub(a.to_number(), b.to_number()));
}

Float Float::mul(const Float &a, const Float &b, unsigned float_digits)
{
    // The exact product first -- 2n places from n-place operands, §8.6's "this
    // is what grows" -- then one rounding, half away from zero, at the
    // result's precision. Rounding an EXACT value cannot double-round, which
    // is why mul needs none of divide's cross-multiplication below.
    return from_number(Number::mul(a.to_number(), b.to_number()))
        .rounded_to(result_places(a, b, float_digits));
}

Float Float::divide(const Float &a, const Float &b, unsigned float_digits)
{
    // "Division always rounds, because its answer is usually not finite."
    // The quotient itself is approximated with guard digits, and then the
    // LAST KEPT PLACE is settled by exact cross-multiplication against the
    // operands -- so the answer is correctly rounded, not guard-digited, and
    // a tie is a proved tie rather than a pattern of nines trusted at sight.
    //
    // The sign is §8.6's free half: positive exactly when the flags agree,
    // decided here and never revisited. Only magnitudes divide.
    const bool positive = a.positive() == b.positive();
    const Number dividend = a.to_number().abs();
    const Number divisor = b.to_number().abs();
    if (dividend.is_zero())
        return Float(); // 0 / b is a positive zero; b == 0 was the caller's

    const unsigned target = result_places(a, b, float_digits);

    // Number::divide counts SIGNIFICANT digits and this rule counts PLACES,
    // so the budget bridges the two: enough leading digits to cover the
    // integer part, then the target places, then guards for the adjustment
    // loops below (which measurement says almost never move -- the quotient
    // is already good to the guards).
    const long long lead =
        floats::floor_log10(dividend) - floats::floor_log10(divisor) + 1;
    const unsigned digits =
        static_cast<unsigned>((lead > 0 ? lead : 0) + target + 4);

    const Number quotient = Number::divide(dividend, divisor, digits);
    Number kept = floats::truncate_places(quotient, target);
    const Number step = floats::place_step(target);

    // Settle the floor exactly: kept <= true quotient < kept + step, checked
    // by cross-multiplication because comparing q against a/b IS comparing
    // q*b against a, and the right-hand side never rounded.
    while (Number::compare(Number::mul(kept, divisor), dividend) > 0)
        kept = Number::sub(kept, step);
    while (Number::compare(
               Number::mul(Number::add(kept, step), divisor), dividend) <= 0)
        kept = Number::add(kept, step);

    // The tie point, proved rather than pattern-matched: the true quotient
    // equals kept + step/2 only when (kept + step/2) x divisor lands exactly
    // on the dividend. At or above the midpoint goes away from zero.
    const Number midpoint = Number::add(kept, Number::shift_right(step, 1));
    if (Number::compare(Number::mul(midpoint, divisor), dividend) <= 0)
        kept = Number::add(kept, step);

    return assemble(positive, kept.floor(), Number::sub(kept, kept.floor()));
}

} // namespace satellite
