// Shared internals of the operator implementation.
//
// src/ops.cpp used to be one file holding every binary operator in the
// language.  It hit the 400-line limit, so it is now src/ops/:
//
//     ops_internal.hpp   this file -- helpers and the operator declarations
//     arith.cpp          + - * / % over Int, Big and Real
//     strings.cpp        the string and list operators, and the str_* helpers
//     compare.cpp        equality and ordering
//     tables.cpp         the dispatch tables and init_tables()
//
// Splitting cost the operators their `static`.  When they all lived in one
// translation unit, init_tables() could name a file-local function directly;
// across five it cannot, so they have external linkage and are declared here.
// They sit in namespace satellite::ops for that reason -- these are
// implementation details that happen to need a linker-visible name, not part
// of the language's surface.
//
// In src/ rather than include/, like big_limbs.hpp and capture_out.hpp:
// everything under include/ is preprocessed by every satellite.cxx block at
// runtime, and a block has no business seeing the dispatch machinery.
#pragma once

#include "satellite/container.hpp"

namespace satellite {
namespace ops {

// ---------------------------------------------------------------------------
// Helpers shared across the operator files.  Inline because they are small and
// on the hot path -- as_bigint in particular is called twice per promoted
// arithmetic operation.
// ---------------------------------------------------------------------------

// The answer for a pair of types that has no operator.  Nil, not a crash: a
// program asking for list - capsule has made a mistake, but it is the
// interpreter's job to report it, not to die of it.
inline Container op_unsupported(const Container&, const Container&) {
    return Container::nil();
}

inline double to_double(const Container& c) {
    switch (c.type) {
        case Type::Int:  return (double)c.i;
        case Type::Real: return c.d;
        case Type::Bool: return c.b ? 1.0 : 0.0;
        case Type::Big:  return std::stod(big::to_string(*c.as_big()));
        default:         return 0.0;
    }
}

// Wrap an Int as a temporary BigInt without allocating a Container.
inline BigInt as_bigint(const Container& c) {
    BigInt t;
    if (c.type == Type::Big) { t.limbs = c.as_big()->limbs; t.neg = c.as_big()->neg; return t; }
    int64_t v = c.i;
    t.neg = v < 0;
    // -INT64_MIN overflows, so the magnitude is taken the long way round.
    uint64_t mag = t.neg ? (uint64_t)(-(v + 1)) + 1 : (uint64_t)v;
    if (mag) t.limbs.push_back(mag);
    return t;
}

inline Container wrap(SatString* p) {
    Container c;
    c.type = Type::Str;
    c.obj  = p;
    return c;
}

// Borrow a container's characters into a SatString, converting if it is not
// already a string.
inline void load_str(SatString& dst, const Container& c) {
    if (c.type == Type::Str) { const SatString* s = c.as_str(); dst.append(s->data(), s->len); }
    else                     { dst.append_text(c.to_string()); }
}

// ---------------------------------------------------------------------------
// arith.cpp
// ---------------------------------------------------------------------------
Container add_int_int(const Container& a, const Container& b);
Container sub_int_int(const Container& a, const Container& b);
Container mul_int_int(const Container& a, const Container& b);

Container add_big_any(const Container& a, const Container& b);
Container sub_big_any(const Container& a, const Container& b);
Container mul_big_any(const Container& a, const Container& b);
Container div_big_any(const Container& a, const Container& b);
Container mod_big_any(const Container& a, const Container& b);

Container add_real(const Container& a, const Container& b);
Container sub_real(const Container& a, const Container& b);
Container mul_real(const Container& a, const Container& b);
Container div_real(const Container& a, const Container& b);
Container mod_real(const Container& a, const Container& b);

Container div_int(const Container& a, const Container& b);
Container mod_int(const Container& a, const Container& b);

// ---------------------------------------------------------------------------
// strings.cpp
// ---------------------------------------------------------------------------
Container add_str(const Container& a, const Container& b);
Container sub_str(const Container& a, const Container& b);
Container mul_str(const Container& a, const Container& b);
Container div_str(const Container& a, const Container& b);
Container add_list(const Container& a, const Container& b);

// ---------------------------------------------------------------------------
// compare.cpp
// ---------------------------------------------------------------------------
bool      same(const Container& a, const Container& b);
Container op_eq(const Container& a, const Container& b);
Container op_lt(const Container& a, const Container& b);

}  // namespace ops
}  // namespace satellite
