// Equality and ordering.
//
// Equality is total: every pair of types is comparable, and pairs that share
// nothing simply answer false.  Ordering is not -- `"a" < 1` has no defensible
// answer, so it is left unsupported rather than given an arbitrary one.
#include "ops_internal.hpp"

namespace satellite {
namespace ops {

bool same(const Container& a, const Container& b) {
    // A capsule is a DECLARATION, so identity is the only equality that means
    // anything: two capsules are equal when they are the same declaration.
    // This branch has to come first, because every test below reads the union
    // as something a capsule is not -- to_double() would answer 0.0 and make
    // every capsule equal to 0.0, and as_bigint() would read the heap POINTER
    // as an integer magnitude, which publishes the address to the program.
    //
    // Deliberately not extended to List: a list wants structural equality, and
    // it already answers those two the same wrong way.  That is a pre-existing
    // hole in this function, older than capsules and out of scope here.
    if (a.type == Type::Capsule || b.type == Type::Capsule)
        return a.type == b.type && a.obj == b.obj;

    if (a.type == Type::Str && b.type == Type::Str)
        return a.as_str()->compare(*b.as_str()) == 0;
    if (a.type == Type::Str || b.type == Type::Str) return false;
    if (a.type == Type::Real || b.type == Type::Real) return to_double(a) == to_double(b);
    if (a.type == Type::Big || b.type == Type::Big) {
        BigInt x = as_bigint(a), y = as_bigint(b);
        return x.neg == y.neg && big::cmp_mag(x, y) == 0;
    }
    if (a.type == Type::Int && b.type == Type::Int) return a.i == b.i;
    if (a.type == Type::Bool && b.type == Type::Bool) return a.b == b.b;
    if (a.type == Type::Nil && b.type == Type::Nil) return true;
    return false;
}

Container op_eq(const Container& a, const Container& b) {
    return Container::boolean(same(a, b));
}

Container op_lt(const Container& a, const Container& b) {
    if (a.type == Type::Str && b.type == Type::Str)
        return Container::boolean(a.as_str()->compare(*b.as_str()) < 0);
    // Big before Real: a big integer converted to double loses precision past
    // 2^53, so comparing two 40-digit numbers as doubles would call distinct
    // values equal.  cmp_mag is exact.
    if (a.type == Type::Big || b.type == Type::Big) {
        BigInt x = as_bigint(a), y = as_bigint(b);
        if (x.neg != y.neg) return Container::boolean(x.neg);
        int c = big::cmp_mag(x, y);
        return Container::boolean(x.neg ? c > 0 : c < 0);
    }
    if (a.type == Type::Real || b.type == Type::Real)
        return Container::boolean(to_double(a) < to_double(b));
    return Container::boolean(a.i < b.i);
}

}  // namespace ops
}  // namespace satellite
