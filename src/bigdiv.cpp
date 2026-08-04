// Division, modulo, powers, shifts and decimal parsing for satellite_number's
// big form.
//
// Split out from bignum.cpp because Knuth's Algorithm D is the one piece of
// bignum arithmetic that is genuinely intricate, and it earns the right to be
// read on its own rather than buried under add and multiply.
#include "satellite/container.hpp"

#include <limits>

#include "big_limbs.hpp"

namespace satellite {
namespace big {

// Wrap limbs and a sign back into a Container, demoting to a plain Int whenever
// the value fits.  normalize() owns the allocation from here, including the
// delete on the demotion path.
static Container make(Limbs limbs, bool neg) {
    auto* p  = new BigInt();
    p->limbs = std::move(limbs);
    p->neg   = neg;
    return normalize(p);
}

// ---------------------------------------------------------------------------
// Knuth, TAOCP vol 2, 4.3.1, Algorithm D -- in base 2^64.
//
// The whole difficulty is step D3: guessing each quotient limb from only the
// top two limbs of the running remainder.  The guess qhat is provably at most 2
// too large; the correction loop brings it to at most 1 too large; step D6
// repairs that last one by adding the divisor back.
//
// D6 fires roughly once in 2^64 random divisions.  It is written out rather
// than skipped precisely because no test built from small numbers will ever
// reach it -- omitting it produces code that passes every casual check and is
// silently wrong forever after.
//
// Requires: u.size() >= v.size() >= 1, v is non-zero, both trimmed.
// ---------------------------------------------------------------------------
static void divmnu(const Limbs& u_in, const Limbs& v_in, Limbs& q, Limbs& r) {
    const size_t m = u_in.size();
    const size_t n = v_in.size();

    // ---- n == 1: short division.  No estimation needed at all, because a
    // 128-by-64 divide is a single instruction on x86-64. ----
    if (n == 1) {
        const uint64_t d = v_in[0];
        q.assign(m, 0);
        __uint128_t rem = 0;
        for (size_t i = m; i-- > 0;) {
            __uint128_t cur = (rem << 64) | u_in[i];
            q[i] = (uint64_t)(cur / d);
            rem  = cur % d;
        }
        r.assign(1, (uint64_t)rem);
        trim(q);
        trim(r);
        return;
    }

    // ---- D1: normalize so the divisor's top limb has its high bit set. ----
    // This is what bounds the error in the D3 estimate.  Shifting BOTH operands
    // left by the same amount leaves the quotient unchanged, so only the
    // remainder has to be shifted back at the end.
    const int s = __builtin_clzll(v_in[n - 1]);

    // Every `s ? x >> (64 - s) : 0` below guards a shift by 64, which is
    // undefined behaviour on a uint64_t rather than the zero it looks like.
    Limbs v(n);
    for (size_t i = n; i-- > 1;)
        v[i] = (v_in[i] << s) | (s ? (v_in[i - 1] >> (64 - s)) : 0);
    v[0] = v_in[0] << s;

    Limbs u(m + 1);
    u[m] = s ? (u_in[m - 1] >> (64 - s)) : 0;
    for (size_t i = m; i-- > 1;)
        u[i] = (u_in[i] << s) | (s ? (u_in[i - 1] >> (64 - s)) : 0);
    u[0] = u_in[0] << s;

    q.assign(m - n + 1, 0);

    constexpr __uint128_t B = (__uint128_t)1 << 64;      // the limb base

    for (size_t j = m - n + 1; j-- > 0;) {
        // ---- D3: estimate one quotient limb from the top two. ----
        const __uint128_t top = ((__uint128_t)u[j + n] << 64) | u[j + n - 1];
        __uint128_t qhat = top / v[n - 1];
        __uint128_t rhat = top % v[n - 1];

        // qhat starts at no more than B, because the invariant u[j+n] <= v[n-1]
        // holds throughout.  Testing `qhat >= B` FIRST matters: short-circuit is
        // what keeps the multiply on the right from overflowing 128 bits.
        while (qhat >= B || qhat * v[n - 2] > (rhat << 64) + u[j + n - 2]) {
            --qhat;
            rhat += v[n - 1];
            if (rhat >= B) break;      // rhat no longer fits; the guess is good
        }

        // ---- D4: u[j .. j+n] -= qhat * v ----
        __uint128_t carry  = 0;
        int64_t     borrow = 0;
        for (size_t i = 0; i < n; ++i) {
            const __uint128_t p = qhat * v[i] + carry;
            carry = p >> 64;
            const __int128_t t =
                (__int128_t)u[i + j] - (__int128_t)(uint64_t)p - borrow;
            u[i + j] = (uint64_t)t;
            borrow   = t < 0 ? 1 : 0;
        }
        const __int128_t top_t =
            (__int128_t)u[j + n] - (__int128_t)carry - borrow;
        u[j + n] = (uint64_t)top_t;

        q[j] = (uint64_t)qhat;

        // ---- D6: the subtraction went negative, so qhat was one too big. ----
        if (top_t < 0) {
            --q[j];
            __uint128_t c = 0;
            for (size_t i = 0; i < n; ++i) {
                const __uint128_t sum = (__uint128_t)u[i + j] + v[i] + c;
                u[i + j] = (uint64_t)sum;
                c = sum >> 64;
            }
            // The final carry out is discarded on purpose: it cancels the
            // borrow that D4 left behind.
            u[j + n] += (uint64_t)c;
        }
    }

    // ---- D8: undo the D1 shift, on the remainder only. ----
    r.assign(n, 0);
    for (size_t i = 0; i + 1 < n; ++i)
        r[i] = (u[i] >> s) | (s ? (u[i + 1] << (64 - s)) : 0);
    r[n - 1] = u[n - 1] >> s;

    trim(q);
    trim(r);
}

// ---------------------------------------------------------------------------
bool divmod(const BigInt& a, const BigInt& b, Container* q_out, Container* r_out) {
    if (b.limbs.empty()) return false;                  // division by zero

    // |a| < |b|: the quotient is zero and the remainder is a unchanged.  Worth
    // special-casing because Algorithm D assumes m >= n.
    if (cmp_limbs(a.limbs, b.limbs) < 0) {
        if (q_out) *q_out = Container::integer(0);
        if (r_out) *r_out = make(a.limbs, a.neg);
        return true;
    }

    Limbs q, r;
    divmnu(a.limbs, b.limbs, q, r);

    // Truncated division: the quotient's sign is the xor of the operands', and
    // the remainder keeps the DIVIDEND's sign.  make() drops the sign when the
    // magnitude is zero, so -6 / 3 gives a remainder of 0 rather than -0.
    if (q_out) *q_out = make(std::move(q), a.neg != b.neg);
    if (r_out) *r_out = make(std::move(r), a.neg);
    return true;
}

// ---------------------------------------------------------------------------
// A ceiling on pow, because `2 ** 10000000000` is a request to fill the
// machine's memory.  Refusing costs one comparison; discovering the limit by
// being killed by the OOM reaper costs the user's whole program.
static constexpr size_t kMaxPowLimbs = 1u << 20;        // ~8 MB per value

Container pow_u64(const BigInt& base, uint64_t exp) {
    if (exp == 0)            return Container::integer(1);
    if (base.limbs.empty())  return Container::integer(0);

    // Decide before allocating, not after: the result needs about
    // bits(base) * exp bits, and that is knowable up front.
    const size_t base_bits = base.limbs.size() * 64;
    if (exp > (uint64_t)kMaxPowLimbs * 64 / base_bits) return Container::nil();

    // Square-and-multiply.  The sign is settled once at the end -- a negative
    // base is negative only for an odd exponent -- because carrying it through
    // the squaring loop is a second chance to get it wrong.
    Limbs acc{1};
    Limbs cur = base.limbs;
    uint64_t e = exp;
    while (e) {
        if (e & 1) acc = mul_limbs(acc, cur);
        e >>= 1;
        if (e) cur = mul_limbs(cur, cur);
        if (acc.size() > kMaxPowLimbs || cur.size() > kMaxPowLimbs)
            return Container::nil();
    }
    return make(std::move(acc), base.neg && (exp & 1));
}

// ---------------------------------------------------------------------------
Container shl(const BigInt& a, uint64_t bits) {
    if (a.limbs.empty()) return Container::integer(0);
    const size_t whole = (size_t)(bits / 64);
    const int    part  = (int)(bits % 64);
    if (whole > kMaxPowLimbs) return Container::nil();

    Limbs r(a.limbs.size() + whole + 1, 0);
    for (size_t i = 0; i < a.limbs.size(); ++i) {
        r[i + whole] |= part ? (a.limbs[i] << part) : a.limbs[i];
        if (part) r[i + whole + 1] |= a.limbs[i] >> (64 - part);
    }
    return make(std::move(r), a.neg);
}

Container shr(const BigInt& a, uint64_t bits) {
    const size_t whole = (size_t)(bits / 64);
    if (whole >= a.limbs.size()) return Container::integer(0);
    const int part = (int)(bits % 64);

    Limbs r(a.limbs.size() - whole, 0);
    for (size_t i = 0; i < r.size(); ++i) {
        r[i] = part ? (a.limbs[i + whole] >> part) : a.limbs[i + whole];
        if (part && i + whole + 1 < a.limbs.size())
            r[i] |= a.limbs[i + whole + 1] << (64 - part);
    }
    return make(std::move(r), a.neg);
}

// ---------------------------------------------------------------------------
// Decimal text -> a number.
//
// Folded 19 digits at a time rather than one, because 10^19 is the largest
// power of ten that fits a limb: each pass is then one multiply-and-add over
// the accumulator instead of nineteen.
Container from_string(const std::string& text, bool* ok) {
    if (ok) *ok = false;

    size_t i   = 0;
    bool   neg = false;
    if (i < text.size() && (text[i] == '-' || text[i] == '+'))
        neg = (text[i++] == '-');

    Limbs acc;
    uint64_t chunk = 0, scale = 1;
    size_t   digits = 0;

    for (; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '_') continue;                    // 1_000_000 is one number
        if (c < '0' || c > '9') return Container::nil();
        chunk = chunk * 10 + (uint64_t)(c - '0');
        scale *= 10;
        ++digits;
        if (scale == 10000000000000000000ull) {    // 10^19, a full limb's worth
            mul_add_small(acc, scale, chunk);
            chunk = 0;
            scale = 1;
        }
    }
    if (digits == 0) return Container::nil();      // "", "-", "+" are not numbers
    if (scale > 1) mul_add_small(acc, scale, chunk);

    if (ok) *ok = true;
    return make(std::move(acc), neg);
}

}  // namespace big
}  // namespace satellite
