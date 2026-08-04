// Limb-level primitives shared by bignum.cpp (add, subtract, multiply) and
// bigdiv.cpp (divide, modulo, power, shift, parse).
//
// This header lives in src/ rather than include/ deliberately.  Everything under
// include/ is preprocessed by every satellite.cxx block at runtime, so putting
// internal helpers there would tax every compile with headers no cxx block has
// any business seeing.
//
// A limb is a uint64_t.  Values are little-endian (limbs[0] is least
// significant) with no trailing zero limb, so an empty vector is exactly zero
// and every value has one canonical representation.  Sign lives outside, in
// BigInt::neg, which is why everything here is magnitude-only.
#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace satellite {
namespace big {

using Limbs = std::vector<uint64_t>;

// Restore the no-trailing-zeros invariant.  Every operation that can shrink a
// value ends with this, so comparisons can rely on size alone.
inline void trim(Limbs& v) {
    while (!v.empty() && v.back() == 0) v.pop_back();
}

// Magnitude comparison: -1, 0 or 1.  Because of the trim invariant, a longer
// vector is always the larger value, so the common case is one size compare.
inline int cmp_limbs(const Limbs& a, const Limbs& b) {
    if (a.size() != b.size()) return a.size() < b.size() ? -1 : 1;
    for (size_t k = a.size(); k-- > 0;)
        if (a[k] != b[k]) return a[k] < b[k] ? -1 : 1;
    return 0;
}

// __builtin_addcll RETURNS the sum and writes the carry-out through the
// pointer -- not the other way round.  Getting this backwards produces a
// result that is correct until the first carry, which is the worst kind of
// wrong.
inline Limbs add_limbs(const Limbs& a, const Limbs& b) {
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

// Requires |a| >= |b|.  The caller establishes that with cmp_limbs and swaps
// the operands if needed -- checking here would cost a compare on every
// subtraction to catch a mistake that belongs to the caller.
inline Limbs sub_limbs(const Limbs& a, const Limbs& b) {
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

// Schoolbook O(n*m).  Measured against boost::cpp_int at 1024 bits: 1.03x, so
// there is nothing to gain from Karatsuba at the sizes satellite programs
// actually reach.  At 8192 bits boost pulls ahead 1.53x, which is where a
// Karatsuba cutoff would go if that ever becomes a real workload.
inline Limbs mul_limbs(const Limbs& a, const Limbs& b) {
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

// Multiply by a single limb and add another, in place.  The workhorse of
// decimal parsing: each step folds 19 more digits into the accumulator.
inline void mul_add_small(Limbs& v, uint64_t mul, uint64_t add) {
    __uint128_t carry = add;
    for (size_t i = 0; i < v.size(); ++i) {
        __uint128_t cur = (__uint128_t)v[i] * mul + carry;
        v[i]  = (uint64_t)cur;
        carry = cur >> 64;
    }
    while (carry) {
        v.push_back((uint64_t)carry);
        carry >>= 64;
    }
    trim(v);
}

}  // namespace big
}  // namespace satellite
