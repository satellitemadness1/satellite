// power and sqrt -- the class-3 answers. DESIGN §8.6: "Rounding is required
// for an answer to exist at all", and this file is where the answers come
// from once M15's rule says how the last place falls.
//
// TWO DIFFERENT PROMISES ABOUT THE LAST PLACE, AND THE DIFFERENCE IS WRITTEN
// RATHER THAN DISCOVERED. sqrt settles its final digit the way divide does --
// exact comparisons against the operand, so a tie is a proved tie. power at a
// fractional exponent is a CHAIN of roots and multiplications and has no
// single cross-check to settle against, so its answer is computed through
// guard places and rounded once: deterministic -- QUAD's invariant, the same
// digits every run -- but the last place is argued from guard width, not
// proved. The width is kGuardPlaces below, and widening it is the whole fix
// the day a misround is found.

#include "satellite_float/float_internal.hpp"
#include "satellite_float/satellite_float.hpp"

namespace satellite {

namespace {

using floats::floor_log10;
using floats::place_step;
using floats::truncate_places;

// Guard places for the fractional-power chain, over and above the answer's.
// Each intermediate is truncated at working width, each truncation costs at
// most one ulp there, and a run of the chain is some tens of operations per
// exponent digit -- so double digits of headroom per thousand operations.
constexpr unsigned kGuardPlaces = 12;

// a^e for a whole non-negative e, EXACT -- §8.6's table row, "repeated
// multiplication ... exact, and grows". Squaring driven by the exponent's own
// bits, so an exponent no machine word holds still only costs its log2 in
// squarings; what ends an answer too wide for the machine is M6's watchdog,
// which is DESIGN §7.5's one and only ceiling.
Number whole_power(const Number &base, const Number &exponent)
{
    Number result(1);
    Number square = base;
    Number remaining = exponent;
    const Number two(2);
    while (!remaining.is_zero()) {
        if (!Number::modulo(remaining, two).is_zero())
            result = Number::mul(result, square);
        remaining = Number::shift_right(remaining, 1).floor();
        if (!remaining.is_zero())
            square = Number::mul(square, square);
    }
    return result;
}

// x^(1/10) for a positive x, to `working` places past the point. Newton on
// x' = (9x + A/x^9) / 10, started at a power of ten at or above the root, so
// the sequence descends monotonically -- which is also the stop rule: the
// first step that fails to descend is quantization at the working width, and
// the value in hand is the converged one.
Number tenth_root(const Number &value, unsigned working)
{
    // value < 10^(f+1) puts the root under 10^ceil((f+1)/10); floor division
    // spelled out because C++ truncates negatives toward zero.
    const long long f = floor_log10(value) + 1;
    const long long up = f >= 0 ? (f + 9) / 10 : -((-f) / 10);
    Number guess = Number::from_small(true, 1, static_cast<int>(up));

    const unsigned digits =
        static_cast<unsigned>((up > 0 ? up : 1) + working + 4);
    for (;;) {
        const Number x2 = Number::mul(guess, guess);
        const Number x4 = Number::mul(x2, x2);
        const Number x9 =
            Number::mul(Number::mul(x4, x4), guess);
        Number next = Number::add(
            Number::mul(guess, Number(9)),
            Number::divide(value, x9, digits));
        // Divide by ten exactly: exponent arithmetic, never a rounding.
        next = Number::mul(next, Number::from_small(true, 1, -1));
        next = truncate_places(next, working);
        if (Number::compare(next, guess) >= 0)
            return guess;
        guess = next;
    }
}

} // namespace

PowerOutcome sqrt_of(const Number &value, unsigned float_digits, Float &out)
{
    if (value.is_negative())
        return PowerOutcome::NoRealAnswer;
    if (value.is_zero()) {
        out = Float();
        return PowerOutcome::Answered;
    }

    // Newton on x' = (x + A/x) / 2, from a power of ten at or above the root,
    // stopping the way tenth_root does. The halving is a shift and exact.
    const long long f = floor_log10(value);
    Number guess =
        Number::from_small(true, 1, static_cast<int>(f / 2 + 1));
    const unsigned working = float_digits + 6;
    const unsigned digits = static_cast<unsigned>(
        (f > 0 ? f / 2 + 1 : 1) + working + 4);
    for (;;) {
        Number next = Number::shift_right(
            Number::add(guess, Number::divide(value, guess, digits)), 1);
        next = truncate_places(next, working);
        if (Number::compare(next, guess) >= 0)
            break;
        guess = next;
    }

    // Settle the last place exactly, divide's contract: kept <= root <
    // kept + step by exact squaring, then the midpoint proves or refutes the
    // tie, and at the midpoint the rule says away from zero. sqrt(6.25) ends
    // here as exactly 2.5.
    const Number step = place_step(float_digits);
    Number kept = truncate_places(guess, float_digits);
    while (Number::compare(Number::mul(kept, kept), value) > 0)
        kept = Number::sub(kept, step);
    while (true) {
        const Number above = Number::add(kept, step);
        if (Number::compare(Number::mul(above, above), value) > 0)
            break;
        kept = above;
    }
    const Number midpoint = Number::add(kept, Number::shift_right(step, 1));
    if (Number::compare(Number::mul(midpoint, midpoint), value) <= 0)
        kept = Number::add(kept, step);

    out = Float::assemble(true, kept.floor(), Number::sub(kept, kept.floor()));
    return PowerOutcome::Answered;
}

PowerOutcome power_of(const Number &base, const Number &exponent,
                      unsigned float_digits, Float &out)
{
    // 0^0 IS 1, SAID RATHER THAN INHERITED: the empty product, x^0 = 1 with
    // no exception carved into it, which is the convention every discrete
    // context uses and MILESTONES/M15.md §2 records as decided.
    if (base.is_zero()) {
        if (exponent.is_zero()) {
            out = Float::from_number(Number(1));
            return PowerOutcome::Answered;
        }
        if (exponent.is_negative())
            return PowerOutcome::DividedByZero; // 1 over 0^|e|
        out = Float();
        return PowerOutcome::Answered;
    }

    if (exponent.is_integer()) {
        // §8.6's first two table rows. The magnitude is exact repeated
        // multiplication; the sign is the parity of the exponent, decided
        // before any arithmetic runs; a negative exponent is the reciprocal,
        // which rounds -- through Float::divide, so it rounds CORRECTLY.
        const Number magnitude =
            whole_power(base.abs(), exponent.abs());
        const bool odd =
            !Number::modulo(exponent.abs(), Number(2)).is_zero();
        const bool positive = base.positive() || !odd;
        const Number signed_power =
            positive ? magnitude : magnitude.negated();
        if (exponent.is_negative()) {
            out = Float::divide(Float::from_number(Number(1)),
                                Float::from_number(signed_power),
                                float_digits);
            return PowerOutcome::Answered;
        }
        out = Float::from_number(signed_power);
        return PowerOutcome::Answered;
    }

    // A FRACTIONAL EXPONENT ON A NEGATIVE BASE IS REFUSED WHOLE. Every
    // fractional exponent here is p/10^k, and some of those roots exist on
    // the real line for a negative base ((-32)^0.2 is -2) while most do not;
    // satellite does not pick out the cases that are, because an answer that
    // appears and vanishes with the reduced form of the exponent's fraction
    // is exactly the kind of cleverness §1.1 spends on the implementation
    // instead of the reader. MILESTONES/M15.md §2 records the line.
    if (base.is_negative())
        return PowerOutcome::NoRealAnswer;

    // base^(I + F) = base^I x base^F, and base^F is peeled a decimal digit at
    // a time: r1 = base^(1/10), each r that follows the tenth root of the one
    // before, so digit d at place i contributes r_i^d. The exponent is a
    // finite decimal by construction -- there is nothing else it can be --
    // so the peel terminates at its written places.
    const Number magnitude = exponent.abs();
    const Number whole = magnitude.floor();
    const Number fraction = Number::sub(magnitude, whole);

    const floats::Decimal peel = floats::decompose(fraction);
    const unsigned k = floats::places_of(peel);
    std::string digits(static_cast<size_t>(-peel.point), '0');
    digits += peel.digits;

    const unsigned working = float_digits + k + kGuardPlaces;
    Number result = whole_power(base, whole);
    Number root = base;
    for (unsigned i = 0; i < k; ++i) {
        root = tenth_root(root, working);
        const int digit = digits[i] - '0';
        for (int d = 0; d < digit; ++d)
            result = truncate_places(Number::mul(result, root), working);
    }

    Float answer = Float::from_number(result).rounded_to(float_digits);
    if (exponent.is_negative())
        answer = Float::divide(Float::from_number(Number(1)), answer,
                               float_digits);
    out = answer;
    return PowerOutcome::Answered;
}

} // namespace satellite
