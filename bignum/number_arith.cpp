// Number: the arithmetic surface — add, sub, mul, divide, modulo.
//
// Part of bignum/, split from an 855-line bignum.cpp.

#include "bignum_internal.hpp"

namespace satellite {

Number Number::add(const Number &a, const Number &b)
{
    // The hot path, and the reason the small representation exists: two
    // long longs and an overflow check, with no allocation and no atomic.
    if (!a.big_ && !b.big_) {
        const int target = std::min(a.exp_, b.exp_);
        long long left = 0;
        long long right = 0;
        long long sum = 0;
        if (scale_ll(a.sig_, a.exp_ - target, left) &&
            scale_ll(b.sig_, b.exp_ - target, right) &&
            !__builtin_add_overflow(left, right, &sum)) {
            Number out;
            out.sig_ = sum;
            out.exp_ = sum == 0 ? 0 : target;
            return out;
        }
    }

    const int sa = a.sign();
    const int sb = b.sign();
    if (sa == 0)
        return b;
    if (sb == 0)
        return a;

    // Align to the LOWER exponent, so no digit is ever discarded: this is what
    // makes 1e20 + 1 exact where a double loses the 1 entirely.
    const int target = std::min(a.exp_, b.exp_);
    const BigInt ma = a.scaled_magnitude(target);
    const BigInt mb = b.scaled_magnitude(target);

    if (sa == sb)
        return make(sa, BigInt::add(ma, mb), target);

    const int c = BigInt::compare(ma, mb);
    if (c == 0)
        return Number();
    if (c > 0)
        return make(sa, BigInt::sub(ma, mb), target);
    return make(sb, BigInt::sub(mb, ma), target);
}

Number Number::sub(const Number &a, const Number &b)
{
    return add(a, b.negated());
}

Number Number::mul(const Number &a, const Number &b)
{
    if (!a.big_ && !b.big_) {
        long long product = 0;
        const long long exponent = static_cast<long long>(a.exp_) + b.exp_;
        if (!__builtin_mul_overflow(a.sig_, b.sig_, &product) &&
            exponent >= INT_MIN && exponent <= INT_MAX) {
            Number out;
            out.sig_ = product;
            out.exp_ = product == 0 ? 0 : static_cast<int>(exponent);
            return out;
        }
    }
    return make(a.sign() * b.sign(), BigInt::mul(a.magnitude(), b.magnitude()),
                static_cast<long long>(a.exp_) + b.exp_);
}

namespace {

// mb with every factor of two and five taken out, and how many there were.
//
// This is the whole of "does this division terminate". A decimal expansion
// stops if and only if what remains of the divisor once the twos and fives are
// removed divides the dividend — because 10^k supplies twos and fives and
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

Number Number::divide(const Number &a, const Number &b, int digits)
{
    // A zero divisor is a satellite error with a span attached, so the caller
    // has already rejected it; returning zero here is what happens if it did
    // not, and it is better than dividing by nothing.
    if (a.is_zero() || b.is_zero())
        return Number();

    if (digits < 1)
        digits = 1;
    if (digits > MAX_DIVISION_DIGITS)
        digits = MAX_DIVISION_DIGITS;

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
    //     1 / 2^100              — terminates at 100 places, came back as
    //                              7.888...e-31 rounded to 34
    //     (1e40 + 1) / 100       — terminates at 2 places, came back as 1e+38,
    //                              because `scale` clamps to zero once the
    //                              dividend is longer than `digits`
    //
    // Both violate §8.1's "division whose result terminates is exact", and both
    // are invisible until a dividend gets longer than `division_digits`.
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

    // Exact divisions — every division by a power of ten, which is every
    // nanoseconds-to-seconds conversion — leave a zero remainder and keep every
    // digit. Only a non-terminating one is cut.
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

    return make(a.sign() * b.sign(),
                BigInt::from_digits(qd.data(), qd.size()), exponent);
}

Number Number::modulo(const Number &a, const Number &b)
{
    if (a.is_zero() || b.is_zero())
        return Number();

    // Truncating remainder, so the sign follows the DIVIDEND — the same rule
    // fmod has, and the one the evaluator's % already documented.
    const int target = std::min(a.exp_, b.exp_);
    BigInt quotient;
    BigInt remainder;
    BigInt::divmod(a.scaled_magnitude(target), b.scaled_magnitude(target),
                   quotient, remainder);
    return make(a.sign(), remainder, target);
}

} // namespace satellite
