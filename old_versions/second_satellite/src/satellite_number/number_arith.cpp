// Number: the arithmetic surface -- add, sub, mul, divide, modulo.
//
// Part of src/satellite_number/, split from the first satellite's 855-line
// bignum.cpp. Every function here decides a sign, so every one of them changed
// shape at M8 even though none of them changed what it computes.

#include "satellite_number/bignum_internal.hpp"

namespace satellite {

Number Number::add(const Number &a, const Number &b)
{
    // THE HOT PATH, AND SIGN-MAGNITUDE MADE IT WIDER RATHER THAN NARROWER.
    // v1 added two signed long longs under one overflow check. Here the signs
    // are consulted first, which is DESIGN §8.6's rule -- same sign, add;
    // opposite signs, subtract the smaller magnitude from the larger and take
    // the larger's sign -- and §8.6 calls the comparison "the one cost of
    // sign-magnitude".
    //
    // THE HALF §8.6 DOES NOT MENTION IS THAT THE OPPOSITE-SIGN CASE CANNOT
    // OVERFLOW. Subtracting the smaller unsigned magnitude from the larger
    // always fits, so it never falls through to the BigInt path -- where v1's
    // signed add fell through whenever the sum left the range, allocating for
    // `1e18 + -1e18`. There is no overflow check on that branch because there
    // is nothing it could catch.
    if (!a.big_ && !b.big_) {
        const int target = std::min(a.exp_, b.exp_);
        unsigned long long left = 0;
        unsigned long long right = 0;
        if (scale_u64(a.sig_, a.exp_ - target, left) &&
            scale_u64(b.sig_, b.exp_ - target, right)) {
            if (a.positive_ == b.positive_) {
                unsigned long long sum = 0;
                if (!__builtin_add_overflow(left, right, &sum))
                    return small(a.positive_, sum, target);
            } else if (left >= right) {
                // 5 + -5 lands here with a zero magnitude, and small() answers
                // it with a positive zero. DESIGN §8.6 invariant 3 is not
                // checked on this path; it is unreachable from it.
                return small(a.positive_, left - right, target);
            } else {
                return small(b.positive_, right - left, target);
            }
        }
    }

    if (a.is_zero())
        return b;
    if (b.is_zero())
        return a;

    // Align to the LOWER exponent, so no digit is ever discarded: this is what
    // makes 1e20 + 1 exact where a double loses the 1 entirely.
    const int target = std::min(a.exp_, b.exp_);
    const BigInt ma = a.scaled_magnitude(target);
    const BigInt mb = b.scaled_magnitude(target);

    if (a.positive_ == b.positive_)
        return make(a.positive_, BigInt::add(ma, mb), target);

    const int c = BigInt::compare(ma, mb);
    if (c == 0)
        return Number();
    if (c > 0)
        return make(a.positive_, BigInt::sub(ma, mb), target);
    return make(b.positive_, BigInt::sub(mb, ma), target);
}

Number Number::sub(const Number &a, const Number &b)
{
    // DESIGN §8.6: "subtraction is addition of the negation", written once.
    return add(a, b.negated());
}

Number Number::mul(const Number &a, const Number &b)
{
    // THE SIGN IS DECIDED BEFORE ANY ARITHMETIC RUNS AND IS NEVER REVISITED --
    // DESIGN §8.6's "multiplication and division get their sign for free". The
    // result is positive exactly when the flags agree; only the magnitudes are
    // multiplied. v1 computed sign(a) * sign(b), which had to ask each operand
    // whether it was zero first.
    const bool positive = a.positive_ == b.positive_;

    if (!a.big_ && !b.big_) {
        unsigned long long product = 0;
        const long long exponent = static_cast<long long>(a.exp_) + b.exp_;
        if (!__builtin_mul_overflow(a.sig_, b.sig_, &product) &&
            exponent >= INT_MIN && exponent <= INT_MAX)
            return small(positive, product, static_cast<int>(exponent));
    }
    return make(positive, BigInt::mul(a.magnitude(), b.magnitude()),
                static_cast<long long>(a.exp_) + b.exp_);
}

namespace {

// mb with every factor of two and five taken out, and how many there were.
//
// This is the whole of "does this division terminate". A decimal expansion
// stops if and only if what remains of the divisor once the twos and fives are
// removed divides the dividend -- because 10^k supplies twos and fives and
// nothing else. Scaling the dividend by 10^max(twos, fives) then makes the
// division come out even, since that supplies enough of both at once.
BigInt strip_twos_and_fives(const BigInt &mb, unsigned &twos, unsigned &fives)
{
    twos = 0;
    fives = 0;

    const BigInt two = BigInt::from_u64(2);
    const BigInt five = BigInt::from_u64(5);
    BigInt rest = mb;
    BigInt quotient;
    BigInt remainder;

    for (;;) {
        BigInt::divmod(rest, two, quotient, remainder);
        if (!remainder.is_zero())
            break;
        rest = quotient;
        twos++;
    }
    for (;;) {
        BigInt::divmod(rest, five, quotient, remainder);
        if (!remainder.is_zero())
            break;
        rest = quotient;
        fives++;
    }
    return rest;
}

} // namespace

Number Number::divide(const Number &a, const Number &b, unsigned digits)
{
    // A zero divisor is a satellite error with a span attached, so the caller
    // has already rejected it -- S0601, and programs/number_command.cpp is the
    // first site to raise it. Returning zero here is what happens if a caller
    // did not, and it is better than dividing by nothing.
    //
    // A ZERO COUNT IS THE SAME KIND OF NON-ANSWER AND GETS THE SAME LINE.
    // Keeping no significant digits produces no value to round, so there is
    // nothing for this function to return that is not a guess. It is refused
    // where it can be pointed at instead: `division_digits=0` in a
    // satellite_config.ini is S0807 with a caret under the `0`, which is
    // machine_limits/config.cpp, and until 2026-08-31 it was silently 34.
    if (a.is_zero() || b.is_zero() || digits == 0)
        return Number();

    // THE COUNT IS THE CALLER'S, THIS NEVER INVENTS ONE, AND SINCE 2026-08-31
    // IT NEVER OVERRULES ONE EITHER. It comes from
    // satellite.library.system.division_digits `1 14 2 1`, whose default lives
    // beside the dial in machine_limits/limits.hpp. There was a clamp to
    // kMaxDivisionDigits here; bignum_number.hpp has what it was and why
    // DESIGN §7.5 does not allow it.

    const BigInt ma = a.magnitude();
    const BigInt mb = b.magnitude();

    // Scale the dividend until the quotient is guaranteed at least `digits` + 1
    // significant digits. The extra one is what the rounding decision is made
    // on, and it is why this is +1 rather than exact.
    long long scale = static_cast<long long>(digits) + 1 +
                      static_cast<long long>(mb.digit_count()) -
                      static_cast<long long>(ma.digit_count());
    if (scale < 0)
        scale = 0;

    BigInt quotient;
    BigInt remainder;
    BigInt::divmod(scale ? BigInt::mul_pow10(ma, static_cast<unsigned>(scale))
                         : ma,
                   mb, quotient, remainder);

    // A non-zero remainder here does NOT mean the division fails to terminate.
    // It means it did not terminate WITHIN THE SCALE CHOSEN ABOVE, which is a
    // different claim, and treating the two as the same was a bug: `scale` is
    // sized to give `digits` + 1 significant digits, so any terminating division
    // that needs more than that was being rounded away.
    //
    //     1 / 2^100              -- terminates at 100 places, came back as
    //                               7.888...e-31 rounded to 34
    //     (1e40 + 1) / 100       -- terminates at 2 places, came back as 1e+38,
    //                               because `scale` clamps to zero once the
    //                               dividend is longer than `digits`
    //
    // Both violate DESIGN §8.1's "division whose result terminates is exact",
    // and both are invisible until a dividend gets longer than division_digits.
    //
    // So: when the remainder is non-zero, ask whether the division terminates at
    // all, and if it does, redo it at a scale that reaches the end. This runs
    // only on divisions that were about to be rounded, so nothing that already
    // came out exact pays for it.
    if (!remainder.is_zero()) {
        unsigned twos = 0;
        unsigned fives = 0;
        const BigInt rest = strip_twos_and_fives(mb, twos, fives);

        BigInt reduced;
        BigInt leftover;
        BigInt::divmod(ma, rest, reduced, leftover);

        if (leftover.is_zero()) {
            const long long need = static_cast<long long>(twos > fives ? twos : fives);
            if (need > scale) {
                scale = need;
                BigInt::divmod(BigInt::mul_pow10(ma, static_cast<unsigned>(scale)),
                               mb, quotient, remainder);
            }
        }
    }

    long long exponent = static_cast<long long>(a.exp_) - b.exp_ - scale;
    std::string qd = quotient.to_digits();

    // Exact divisions -- every division by a power of ten, which is every
    // nanoseconds-to-seconds conversion -- leave a zero remainder and keep every
    // digit. Only a non-terminating one is cut.
    //
    // AND THE CUT ROUNDS HALF AWAY FROM ZERO, WHICH IS THE LANGUAGE'S ONE
    // RULE SINCE M15 AND WAS ONLY AN INHERITANCE BEFORE IT. The `>= '5'` bump
    // below is v1's behaviour ported at M8, and DESIGN §8.6 spent two
    // milestones calling it "a default arrived by porting rather than by
    // decision"; M15 chose the float's rule and RATIFIED this site rather
    // than overruling it, so the number's division and the float's agree by
    // construction. This runs on a magnitude -- the sign came off with abs()
    // above -- which is why half-up here IS half away from zero.
    if (!remainder.is_zero() && qd.size() > static_cast<size_t>(digits)) {
        const size_t keep = static_cast<size_t>(digits);
        const bool up = qd[keep] >= '5';
        exponent += static_cast<long long>(qd.size() - keep);
        qd.resize(keep);

        BigInt kept = BigInt::from_digits(qd.data(), qd.size());
        if (up)
            kept = BigInt::add(kept, BigInt::from_u64(1));
        qd = kept.to_digits();
    }

    // Strip trailing zeros before building the result, so an exact division
    // comes back in the small representation instead of carrying the scaling
    // around as a bignum for the rest of the program.
    size_t end = qd.size();
    while (end > 1 && qd[end - 1] == '0') {
        end--;
        exponent++;
    }
    qd.resize(end);

    return make(a.positive_ == b.positive_,
                BigInt::from_digits(qd.data(), qd.size()), exponent);
}

Number Number::modulo(const Number &a, const Number &b)
{
    if (a.is_zero() || b.is_zero())
        return Number();

    // Truncated and not floored, so the result takes the sign of the DIVIDEND
    // -- C's rule rather than Python's, and DESIGN §8.6 says why: under
    // sign-magnitude, trunc is the magnitude with `a`'s sign, so dropping the
    // fraction is the whole implementation.
    //
    // THE C++ IS HERE AND THE PATH IS NOT. `satellite.variable.number.modulus`
    // `1 6 4 12` is M15's, because DESIGN §8.6 is what it cites and the
    // rounding rule that section leaves open is M15's to settle. This function
    // is one of the ten files' contents and ports with them; nothing in the
    // language can reach it yet.
    const int target = std::min(a.exp_, b.exp_);
    BigInt quotient;
    BigInt remainder;
    BigInt::divmod(a.scaled_magnitude(target), b.scaled_magnitude(target),
                   quotient, remainder);
    return make(a.positive_, remainder, target);
}

namespace {

// `base` to the `n`, by squaring, for the two shifts and nothing else. Linear
// in the bits of `n` rather than in `n`, which matters because nothing bounds
// what a program may shift by -- DESIGN §7.5 again -- so the loop has to be
// paid for by the size of the answer and not by the size of the count.
BigInt power_of(unsigned long long base, unsigned n)
{
    BigInt result = BigInt::from_u64(1);
    BigInt square = BigInt::from_u64(base);
    for (unsigned bits = n; bits != 0; bits >>= 1) {
        if ((bits & 1u) != 0)
            result = BigInt::mul(result, square);
        if ((bits >> 1) != 0)
            square = BigInt::mul(square, square);
    }
    return result;
}

} // namespace

// TWO MULTIPLICATIONS AND NO DIVISION, WHICH IS WHY NEITHER OF THESE ROUNDS.
// bignum_number.hpp has the decision and DESIGN §5.5 is where it is written
// down: a shift is multiplication and division by a power of two. Left is the
// multiplication. Right is the same multiplication by 5^n with the exponent
// dropped by n, because a / 2^n is a * 5^n / 10^n and dividing by a power of
// ten is an adjustment to `exp_` rather than any arithmetic at all -- so the
// direction that could in principle not terminate never runs a long division.
Number Number::shift_left(const Number &a, unsigned n)
{
    if (a.is_zero())
        return Number();
    if (n == 0)
        return a;
    return make(a.positive_, BigInt::mul(a.magnitude(), power_of(2, n)), a.exp_);
}

Number Number::shift_right(const Number &a, unsigned n)
{
    if (a.is_zero())
        return Number();
    if (n == 0)
        return a;
    return make(a.positive_, BigInt::mul(a.magnitude(), power_of(5, n)),
                static_cast<long long>(a.exp_) - static_cast<long long>(n));
}

} // namespace satellite
