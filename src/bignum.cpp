#include "satellite/container.hpp"

#include <algorithm>
#include <limits>

#include "big_limbs.hpp"

namespace satellite {

// ===========================================================================
// BigInt
//
// The limb-level primitives (trim, cmp_limbs, add_limbs, sub_limbs, mul_limbs)
// live in big_limbs.hpp so that bigdiv.cpp can share them rather than keep a
// second copy that drifts.  That header is in src/ on purpose: everything under
// include/ gets preprocessed by every satellite.cxx block at runtime.
// ===========================================================================
namespace big {

int cmp_mag(const BigInt& a, const BigInt& b) { return cmp_limbs(a.limbs, b.limbs); }

// Shrink back to a plain Int whenever the value fits -- keeps the fast path
// fast after a temporary excursion into bignum territory.
Container normalize(BigInt* p) {
    p->trim();
    if (p->limbs.empty()) { delete p; return Container::integer(0); }
    if (p->limbs.size() == 1) {
        uint64_t m = p->limbs[0];
        constexpr uint64_t MAXPOS = (uint64_t)std::numeric_limits<int64_t>::max();
        if (!p->neg && m <= MAXPOS) { delete p; return Container::integer((int64_t)m); }
        if (p->neg && m <= MAXPOS + 1) {
            int64_t v = (m == MAXPOS + 1) ? std::numeric_limits<int64_t>::min()
                                          : -(int64_t)m;
            delete p;
            return Container::integer(v);
        }
    }
    Container c;
    c.type = Type::Big;
    c.obj  = p;
    return c;
}

Container from_i64(int64_t v) {
    auto* p = new BigInt();
    p->neg = v < 0;
    uint64_t mag = p->neg ? (uint64_t)(-(v + 1)) + 1 : (uint64_t)v;
    if (mag) p->limbs.push_back(mag);
    Container c;
    c.type = Type::Big;
    c.obj  = p;
    return c;
}

Container add(const BigInt& a, const BigInt& b) {
    auto* r = new BigInt();
    if (a.neg == b.neg) {
        r->limbs = add_limbs(a.limbs, b.limbs);
        r->neg   = a.neg;
    } else {
        int c = cmp_limbs(a.limbs, b.limbs);
        if (c == 0) return normalize(r);
        if (c > 0) { r->limbs = sub_limbs(a.limbs, b.limbs); r->neg = a.neg; }
        else       { r->limbs = sub_limbs(b.limbs, a.limbs); r->neg = b.neg; }
    }
    return normalize(r);
}

Container sub(const BigInt& a, const BigInt& b) {
    BigInt nb;
    nb.limbs = b.limbs;
    nb.neg   = b.limbs.empty() ? false : !b.neg;
    return add(a, nb);
}

Container mul(const BigInt& a, const BigInt& b) {
    auto* r = new BigInt();
    r->limbs = mul_limbs(a.limbs, b.limbs);
    r->neg   = !r->limbs.empty() && (a.neg != b.neg);
    return normalize(r);
}

std::string to_string(const BigInt& a) {
    if (a.limbs.empty()) return "0";
    constexpr uint64_t CHUNK = 10000000000000000000ull;   // 10^19
    Limbs w = a.limbs;
    std::string out;
    while (!w.empty()) {
        __uint128_t rem = 0;
        for (size_t k = w.size(); k-- > 0;) {
            __uint128_t cur = (rem << 64) | w[k];
            w[k] = (uint64_t)(cur / CHUNK);
            rem  = cur % CHUNK;
        }
        trim(w);
        std::string piece = std::to_string((uint64_t)rem);
        if (!w.empty()) piece.insert(0, 19 - piece.size(), '0');
        out.insert(0, piece);
    }
    if (a.neg) out.insert(0, 1, '-');
    return out;
}

}  // namespace big

}  // namespace satellite
