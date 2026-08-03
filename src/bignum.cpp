#include "satellite/container.hpp"

#include <algorithm>
#include <limits>

namespace satellite {

// ===========================================================================
// BigInt
// ===========================================================================
namespace big {

using Limbs = std::vector<uint64_t>;

static void trim(Limbs& v) { while (!v.empty() && v.back() == 0) v.pop_back(); }

static int cmp_limbs(const Limbs& a, const Limbs& b) {
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    for (size_t k = a.size(); k-- > 0;)
        if (a[k] != b[k]) return a[k] < b[k] ? -1 : 1;
    return 0;
}

// __builtin_addcll RETURNS the sum and writes the carry-out through the
// pointer -- not the other way round.
static Limbs add_limbs(const Limbs& a, const Limbs& b) {
    Limbs r;
    r.resize(std::max(a.size(), b.size()) + 1, 0);
    unsigned long long carry = 0;
    for (size_t k = 0; k < r.size(); ++k) {
        unsigned long long x = k < a.size() ? a[k] : 0;
        unsigned long long y = k < b.size() ? b[k] : 0;
        unsigned long long carry_out;
        r[k]  = __builtin_addcll(x, y, carry, &carry_out);
        carry = carry_out;
    }
    trim(r);
    return r;
}

// requires |a| >= |b|
static Limbs sub_limbs(const Limbs& a, const Limbs& b) {
    Limbs r(a.size(), 0);
    unsigned long long borrow = 0;
    for (size_t k = 0; k < a.size(); ++k) {
        unsigned long long y = k < b.size() ? b[k] : 0;
        unsigned long long borrow_out;
        r[k]   = __builtin_subcll(a[k], y, borrow, &borrow_out);
        borrow = borrow_out;
    }
    trim(r);
    return r;
}

static Limbs mul_limbs(const Limbs& a, const Limbs& b) {
    if (a.empty() || b.empty()) return {};
    Limbs r(a.size() + b.size(), 0);
    for (size_t k = 0; k < a.size(); ++k) {
        __uint128_t carry = 0;
        for (size_t j = 0; j < b.size(); ++j) {
            __uint128_t cur = (__uint128_t)a[k] * b[j] + r[k + j] + carry;
            r[k + j] = (uint64_t)cur;
            carry    = cur >> 64;
        }
        size_t idx = k + b.size();
        while (carry) {
            __uint128_t cur = (__uint128_t)r[idx] + carry;
            r[idx] = (uint64_t)cur;
            carry  = cur >> 64;
            ++idx;
        }
    }
    trim(r);
    return r;
}

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
