// The uniform draw, at arbitrary precision — the bignum half of §18.
//
// Part of src/satellite_number/. It is here rather than beside the generator because it is
// bignum work: a 32-bit generator's own bounded draw caps at a 2^32 span, and
// satellite.random.ultra(40) needs 10^40 of them. Nothing in this file knows
// what a PCG is; it asks a Bits32 for 32 bits at a time and does the rest in
// base 10^9.

#include "satellite_number/bignum_internal.hpp"

namespace satellite {

namespace {

// Uniform over [0, bound), unbiased, from 32-bit draws. `bound` is at least 1.
//
// Rejection, never `%` on its own. The modulo alone is skewed whenever `bound`
// does not divide 2^32: the low (2^32 mod bound) residues get one extra
// preimage each. Measured over three buckets in the §18 session, `%` gave
// 1398410 / 904307 / 697283 against the 998574 / 1001152 / 1000274 this path
// gives — a 2:1 skew, which is not a subtlety anyone would notice in a test
// that only checked the range.
//
// `threshold` is 2^32 mod bound, computed as (-bound) % bound in unsigned
// arithmetic because 2^32 is one past what the type holds. Draws below it are
// exactly the ones that would have made the last, short residue class.
unsigned draw_below(Bits32 &bits, unsigned bound)
{
    if (bound <= 1)
        return 0;
    const unsigned threshold = static_cast<unsigned>(0u - bound) % bound;
    for (;;) {
        const unsigned r = bits.next();
        if (r >= threshold)
            return r % bound;
    }
}

} // namespace

BigInt BigInt::random_below(const BigInt &bound, Bits32 &bits)
{
    // [0, 0) has nothing in it. The callers all check, so this is the same kind
    // of belt-and-braces divmod's non-zero requirement gets.
    if (bound.is_zero())
        return BigInt();

    // trim() guarantees the top limb is non-zero, which is what makes the
    // acceptance argument below hold: `top` is at least 1, so the sampling
    // space is at most twice the bound.
    const size_t count = bound.limbs_.size();
    const unsigned top = bound.limbs_.back();

    BigInt out;
    out.limbs_.resize(count);
    for (;;) {
        // The top limb over [0, top], every other over the whole base. The
        // values this produces are exactly [0, (top+1) * BASE^(count-1)), each
        // once, so the draw is uniform on that interval — and `bound` lies
        // inside it, since bound = top * BASE^(count-1) + (something under
        // BASE^(count-1)).
        for (size_t i = 0; i + 1 < count; i++)
            out.limbs_[i] = draw_below(bits, BASE);
        out.limbs_[count - 1] = draw_below(bits, top + 1);

        // Rejection is what keeps it uniform: the values at or above `bound`
        // are simply redrawn, never folded back in. It happens with probability
        // 1 - bound/((top+1) * BASE^(count-1)), which is at most 1/(top+1) and
        // so at most half — the same guarantee §18 wanted from a bit-length
        // draw, without needing a bit length.
        if (compare(out, bound) < 0) {
            out.trim();   // leading zero limbs are spelling, not value
            return out;
        }
    }
}

bool Number::integer_magnitude(BigInt &out, int max_digits) const
{
    std::string digits;
    long long exponent = 0;
    normalized(digits, exponent);

    if (digits == "0") {
        out = BigInt();
        return true;
    }
    if (exponent < 0)
        return false;   // a fractional part, after trailing zeros went

    // Checked on the COUNT, before anything is built. normalized() has stripped
    // the trailing zeros into `exponent`, so this is the true width of the
    // integer: `1e2000000000` is twelve characters of source and two billion
    // digits of value, and mul_pow10 would happily try.
    if (static_cast<long long>(digits.size()) + exponent > max_digits)
        return false;

    out = BigInt::from_digits(digits.data(), digits.size());
    if (exponent > 0)
        out = BigInt::mul_pow10(out, static_cast<unsigned>(exponent));
    return true;
}

bool Number::random_below(const Number &bound, Bits32 &bits, Number &out)
{
    if (bound.sign() <= 0)
        return false;

    // MAX_RANDOM_DIGITS + 1, and the +1 is not slack. The ceiling is on the
    // DRAW, and a draw below `bound` has at most as many digits as bound - 1:
    // 10^100000 is 100001 digits wide and every value under it is at most
    // 100000, which is exactly what satellite.random.ultra(100000) asks for.
    BigInt limit;
    if (!bound.integer_magnitude(limit, MAX_RANDOM_DIGITS + 1))
        return false;

    out = make(1, BigInt::random_below(limit, bits), 0);
    return true;
}

} // namespace satellite
