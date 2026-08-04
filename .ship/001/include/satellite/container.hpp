// satellite_container -- the universal value of the SATELLITE language.
//
// Every value in satellite lives in one of these: numbers, strings, lists,
// capsules, threads.  It is a 16-byte tagged union, which means four of them
// fit in a cache line and passing one costs two register moves.
//
// Operators do NOT branch on type.  `a + b` is a table lookup indexed by the
// two type tags, so adding a new type to the language means filling in a row,
// never editing an if-chain.  Dispatch cost is constant no matter how many
// types satellite grows.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "satellite/bignum.hpp"
#include "satellite/charmap.hpp"
#include "satellite/string.hpp"
#include "satellite/types.hpp"

namespace satellite {

struct Container;

struct List : Obj {
    std::vector<Container> items;
    List() : Obj(Type::List) {}
};

// Declared, not defined: a capsule carries parameter names and a span of
// source, which is the lexer's and the library's business, not the value
// model's.  container.hpp stays the 16-byte value and nothing else.
struct Capsule;

// ---------------------------------------------------------------------------
// The container itself.
// ---------------------------------------------------------------------------
struct Container {
    Type type;
    union {
        bool    b;
        int64_t i;
        double  d;
        Obj*    obj;
    };

    Container() : type(Type::Nil), i(0) {}

    // --- construction ------------------------------------------------------
    static Container nil()            { return Container(); }
    static Container boolean(bool v)  { Container c; c.type = Type::Bool; c.b = v; return c; }
    static Container integer(int64_t v){ Container c; c.type = Type::Int;  c.i = v; return c; }
    static Container real(double v)   { Container c; c.type = Type::Real; c.d = v; return c; }
    static Container str(const char* s);
    static Container str(const std::string& s);
    static Container big_from_i64(int64_t v);
    static Container list();
    static Container capsule(Capsule* p);   // takes ownership of one reference

    // --- refcounting -------------------------------------------------------
    Container(const Container& o) : type(o.type) { i = o.i; retain(); }
    Container(Container&& o) noexcept : type(o.type) { i = o.i; o.type = Type::Nil; o.i = 0; }
    Container& operator=(const Container& o);
    Container& operator=(Container&& o) noexcept;
    ~Container() { release(); }

    bool is_heap() const {
        return type == Type::Big || type == Type::Str || type == Type::List ||
               type == Type::Capsule;
    }
    void retain()  { if (is_heap() && obj) obj->rc.fetch_add(1, std::memory_order_relaxed); }
    void release();

    // --- typed access ------------------------------------------------------
    SatString* as_str()  const { return static_cast<SatString*>(obj); }
    BigInt*    as_big()  const { return static_cast<BigInt*>(obj); }
    List*      as_list() const { return static_cast<List*>(obj); }

    // Defined in capsule.hpp, because a downcast needs the complete type and
    // Capsule is only declared here.  Still inline, so it costs what the three
    // above cost: nothing.
    Capsule*   as_capsule() const;

    std::string to_string() const;
    bool truthy() const;
};

static_assert(sizeof(Container) == 16, "container must stay 16 bytes");

// ---------------------------------------------------------------------------
// Dispatch tables.  One indexed load + one indirect call, no branch chain.
// ---------------------------------------------------------------------------
using BinFn = Container (*)(const Container&, const Container&);

constexpr int TCOUNT = static_cast<int>(Type::COUNT);

extern BinFn add_table[TCOUNT][TCOUNT];
extern BinFn sub_table[TCOUNT][TCOUNT];
extern BinFn mul_table[TCOUNT][TCOUNT];
extern BinFn div_table[TCOUNT][TCOUNT];
extern BinFn eq_table[TCOUNT][TCOUNT];
extern BinFn lt_table[TCOUNT][TCOUNT];

void init_tables();   // must run once before any operator is used

// HYBRID DISPATCH.  Measured on this hardware (see bench/bench_dispatch.cpp):
// a pure table is 1.1-1.3x SLOWER than a plain if-chain at satellite's current
// type count, because an indirect call is itself an unpredictable branch and
// it blocks inlining.  So the hot type pairs get inline branches, and the
// table catches the long tail.
//
// The split is what keeps both properties: hot code never pays for the
// indirection, and adding a new satellite type still means filling in a table
// row rather than growing a chain that slows every operation down.
inline Container add(const Container& a, const Container& b) {
    if (a.type == Type::Int && b.type == Type::Int) {
        int64_t r;
        if (!__builtin_add_overflow(a.i, b.i, &r)) return Container::integer(r);
    } else if (a.type == Type::Real && b.type == Type::Real) {
        return Container::real(a.d + b.d);
    }
    return add_table[static_cast<int>(a.type)][static_cast<int>(b.type)](a, b);
}

inline Container sub(const Container& a, const Container& b) {
    if (a.type == Type::Int && b.type == Type::Int) {
        int64_t r;
        if (!__builtin_sub_overflow(a.i, b.i, &r)) return Container::integer(r);
    } else if (a.type == Type::Real && b.type == Type::Real) {
        return Container::real(a.d - b.d);
    }
    return sub_table[static_cast<int>(a.type)][static_cast<int>(b.type)](a, b);
}

inline Container mul(const Container& a, const Container& b) {
    if (a.type == Type::Int && b.type == Type::Int) {
        int64_t r;
        if (!__builtin_mul_overflow(a.i, b.i, &r)) return Container::integer(r);
    } else if (a.type == Type::Real && b.type == Type::Real) {
        return Container::real(a.d * b.d);
    }
    return mul_table[static_cast<int>(a.type)][static_cast<int>(b.type)](a, b);
}

// `"a,b,c" / ","` splits into a list; `"ab" * 3` repeats.
inline Container div(const Container& a, const Container& b) {
    return div_table[static_cast<int>(a.type)][static_cast<int>(b.type)](a, b);
}
inline Container equals(const Container& a, const Container& b) {
    return eq_table[static_cast<int>(a.type)][static_cast<int>(b.type)](a, b);
}
inline Container less(const Container& a, const Container& b) {
    return lt_table[static_cast<int>(a.type)][static_cast<int>(b.type)](a, b);
}

// ---------------------------------------------------------------------------
// String helpers that build new containers
// ---------------------------------------------------------------------------
Container str_slice(const Container& s, int64_t start, int64_t n);
Container str_split(const Container& s, const Container& sep);
Container str_join(const Container& list, const Container& sep);
Container str_upper(const Container& s);
Container str_lower(const Container& s);
Container str_trim(const Container& s);
Container str_reverse(const Container& s);
Container str_replace_all(const Container& s, const Container& from, const Container& to);
Container str_to_number(const Container& s);

// ---------------------------------------------------------------------------
// BigInt arithmetic (used by the promotion paths above)
// ---------------------------------------------------------------------------
namespace big {
Container from_i64(int64_t v);
Container add(const BigInt& a, const BigInt& b);
Container sub(const BigInt& a, const BigInt& b);
Container mul(const BigInt& a, const BigInt& b);
int       cmp_mag(const BigInt& a, const BigInt& b);
std::string to_string(const BigInt& a);
Container normalize(BigInt* b);   // demote back to Int when it fits
}  // namespace big

}  // namespace satellite
