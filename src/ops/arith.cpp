// Arithmetic: + - * / % over Int, Big and Real.
//
// The shape every integer operation here shares: try it in int64, and promote
// to BigInt only when the hardware says it overflowed.  __builtin_*_overflow
// compiles to the add/sub/mul the CPU was going to do anyway plus a branch on
// the overflow flag, so the common case costs a correctly-predicted branch and
// the arbitrary-precision path is never taken speculatively.
#include "ops_internal.hpp"

#include <cmath>
#include <limits>

namespace satellite {
namespace ops {

// --- integer, fast path with promotion on overflow -------------------------
Container add_int_int(const Container& a, const Container& b) {
    int64_t r;
    if (!__builtin_add_overflow(a.i, b.i, &r)) return Container::integer(r);
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::add(x, y);
}
Container sub_int_int(const Container& a, const Container& b) {
    int64_t r;
    if (!__builtin_sub_overflow(a.i, b.i, &r)) return Container::integer(r);
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::sub(x, y);
}
Container mul_int_int(const Container& a, const Container& b) {
    int64_t r;
    if (!__builtin_mul_overflow(a.i, b.i, &r)) return Container::integer(r);
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::mul(x, y);
}

// --- at least one side is already Big --------------------------------------
Container add_big_any(const Container& a, const Container& b) {
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::add(x, y);
}
Container sub_big_any(const Container& a, const Container& b) {
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::sub(x, y);
}
Container mul_big_any(const Container& a, const Container& b) {
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::mul(x, y);
}

// big::divmod computes both halves in one pass of Algorithm D, so asking for
// only one costs nothing extra -- the null out-pointer just discards it.
//
// It returns false for a zero divisor and writes nothing.  Nil is satellite's
// answer for that, matching div_int below: there is no integer that means
// "undefined", and inventing one would let it propagate silently through the
// next twenty operations before surfacing somewhere unrelated.
Container div_big_any(const Container& a, const Container& b) {
    BigInt x = as_bigint(a), y = as_bigint(b);
    Container q;
    if (!big::divmod(x, y, &q, nullptr)) return Container::nil();
    return q;
}
Container mod_big_any(const Container& a, const Container& b) {
    BigInt x = as_bigint(a), y = as_bigint(b);
    Container r;
    if (!big::divmod(x, y, nullptr, &r)) return Container::nil();
    return r;
}

// --- real -------------------------------------------------------------------
Container add_real(const Container& a, const Container& b) {
    return Container::real(to_double(a) + to_double(b));
}
Container sub_real(const Container& a, const Container& b) {
    return Container::real(to_double(a) - to_double(b));
}
Container mul_real(const Container& a, const Container& b) {
    return Container::real(to_double(a) * to_double(b));
}
Container div_real(const Container& a, const Container& b) {
    double r = to_double(b);
    if (r == 0.0) return Container::nil();
    return Container::real(to_double(a) / r);
}

// fmod, not remainder: fmod truncates toward zero and takes the sign of the
// dividend, which is what % does for integers here and in C++.  std::remainder
// rounds to nearest instead, so fmod(-7.5, 2) is -1.5 while remainder(-7.5, 2)
// is 0.5 -- the same expression changing sign as a value crosses from Int to
// Real would be indefensible.
Container mod_real(const Container& a, const Container& b) {
    double r = to_double(b);
    if (r == 0.0) return Container::nil();
    return Container::real(std::fmod(to_double(a), r));
}

// --- integer division and modulo -------------------------------------------
//
// Truncated, matching C++ and every language satellite will be compared
// against: the quotient rounds TOWARD ZERO and the remainder takes the sign of
// the DIVIDEND.  So -7/2 == -3 and -7%2 == -1.  big::divmod agrees, which is
// what keeps the answer stable as a value grows past int64.
Container div_int(const Container& a, const Container& b) {
    if (b.i == 0) return Container::nil();                 // division by zero
    // The one division that overflows: |INT64_MIN| is one past INT64_MAX, so
    // the true quotient has no int64 representation.  On x86 the idiv
    // instruction does not return a wrong answer here, it raises SIGFPE and
    // takes the process down -- so this must be caught BEFORE the divide, not
    // corrected after it.
    if (a.i == std::numeric_limits<int64_t>::min() && b.i == -1)
        return big::mul(*Container::big_from_i64(a.i).as_big(),
                        *Container::big_from_i64(-1).as_big());
    return Container::integer(a.i / b.i);
}

Container mod_int(const Container& a, const Container& b) {
    if (b.i == 0) return Container::nil();
    // INT64_MIN % -1 is mathematically 0 and needs no promotion -- but it
    // traps for the same reason the division does, because the CPU computes
    // both halves with one idiv and overflows on the quotient it was not even
    // asked for.  Answering directly is the only way to avoid the fault.
    if (a.i == std::numeric_limits<int64_t>::min() && b.i == -1)
        return Container::integer(0);
    return Container::integer(a.i % b.i);
}

}  // namespace ops
}  // namespace satellite
