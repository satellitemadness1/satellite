// Operators and dispatch tables -- the whole binary-operator surface of the
// language lives here, so adding a type means editing one file.
#include "satellite/container.hpp"

#include <cstring>
#include <limits>

namespace satellite {

// ===========================================================================
// Dispatch tables
// ===========================================================================
BinFn add_table[TCOUNT][TCOUNT];
BinFn sub_table[TCOUNT][TCOUNT];
BinFn mul_table[TCOUNT][TCOUNT];

static Container op_unsupported(const Container&, const Container&) {
    return Container::nil();
}

static double to_double(const Container& c) {
    switch (c.type) {
        case Type::Int:  return (double)c.i;
        case Type::Real: return c.d;
        case Type::Bool: return c.b ? 1.0 : 0.0;
        case Type::Big:  return std::stod(big::to_string(*c.as_big()));
        default:         return 0.0;
    }
}

// --- promotion helpers -----------------------------------------------------
// Wrap an Int as a temporary BigInt without allocating a Container.
static BigInt as_bigint(const Container& c) {
    BigInt t;
    if (c.type == Type::Big) { t.limbs = c.as_big()->limbs; t.neg = c.as_big()->neg; return t; }
    int64_t v = c.i;
    t.neg = v < 0;
    uint64_t mag = t.neg ? (uint64_t)(-(v + 1)) + 1 : (uint64_t)v;
    if (mag) t.limbs.push_back(mag);
    return t;
}

// --- integer ops (slow path: at least one side overflowed or is Big) -------
static Container add_int_int(const Container& a, const Container& b) {
    int64_t r;
    if (!__builtin_add_overflow(a.i, b.i, &r)) return Container::integer(r);
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::add(x, y);
}
static Container sub_int_int(const Container& a, const Container& b) {
    int64_t r;
    if (!__builtin_sub_overflow(a.i, b.i, &r)) return Container::integer(r);
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::sub(x, y);
}
static Container mul_int_int(const Container& a, const Container& b) {
    int64_t r;
    if (!__builtin_mul_overflow(a.i, b.i, &r)) return Container::integer(r);
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::mul(x, y);
}

static Container add_big_any(const Container& a, const Container& b) {
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::add(x, y);
}
static Container sub_big_any(const Container& a, const Container& b) {
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::sub(x, y);
}
static Container mul_big_any(const Container& a, const Container& b) {
    BigInt x = as_bigint(a), y = as_bigint(b);
    return big::mul(x, y);
}

// --- real ops --------------------------------------------------------------
static Container add_real(const Container& a, const Container& b) {
    return Container::real(to_double(a) + to_double(b));
}
static Container sub_real(const Container& a, const Container& b) {
    return Container::real(to_double(a) - to_double(b));
}
static Container mul_real(const Container& a, const Container& b) {
    return Container::real(to_double(a) * to_double(b));
}

// --- string ops ------------------------------------------------------------
// "a" + "b"  ->  concatenation.  Any type concatenated with a string becomes
// a string, so satellite.console.display("n: " + 5) works as expected.
static Container add_str(const Container& a, const Container& b) {
    auto* p = new SatString();
    // string + string copies characters straight across -- no encode/decode
    auto push = [&](const Container& c) {
        if (c.type == Type::Str) { const SatString* s = c.as_str(); p->append(s->data(), s->len); }
        else                     { p->append_text(c.to_string()); }
    };
    push(a);
    push(b);
    Container c;
    c.type = Type::Str;
    c.obj  = p;
    return c;
}

// "my_string" - "str"  ->  "my_ing"
// DEFAULT DECISION (change if you want different semantics): removes the FIRST
// occurrence only.  Alternatives are all-occurrences or trailing-only.
static Container sub_str(const Container& a, const Container& b) {
    auto load = [](SatString& dst, const Container& c) {
        if (c.type == Type::Str) { const SatString* s = c.as_str(); dst.append(s->data(), s->len); }
        else                     { dst.append_text(c.to_string()); }
    };
    auto* p = new SatString();
    load(*p, a);
    SatString needle;
    load(needle, b);

    int64_t at = p->find(needle);
    if (at >= 0) p->erase((uint32_t)at, needle.len);

    Container c;
    c.type = Type::Str;
    c.obj  = p;
    return c;
}

// list + list -> concatenation
static Container add_list(const Container& a, const Container& b) {
    Container out = Container::list();
    auto& dst = out.as_list()->items;
    for (const auto& x : a.as_list()->items) dst.push_back(x);
    for (const auto& x : b.as_list()->items) dst.push_back(x);
    return out;
}

// ---------------------------------------------------------------------------
// String helpers that build new containers
// ---------------------------------------------------------------------------
static Container wrap(SatString* p) {
    Container c;
    c.type = Type::Str;
    c.obj  = p;
    return c;
}

// borrow a container's characters into a SatString, converting if it is not
// already a string
static void load_str(SatString& dst, const Container& c) {
    if (c.type == Type::Str) { const SatString* s = c.as_str(); dst.append(s->data(), s->len); }
    else                     { dst.append_text(c.to_string()); }
}

Container str_slice(const Container& s, int64_t start, int64_t n) {
    if (s.type != Type::Str) return Container::nil();
    const SatString* src = s.as_str();
    if (start < 0) start += src->len;                 // negative counts from the end
    if (start < 0) start = 0;
    if (start > (int64_t)src->len) start = src->len;
    if (n < 0) n = 0;
    auto* p = new SatString();
    p->assign_slice(*src, (uint32_t)start, (uint32_t)n);
    return wrap(p);
}

Container str_split(const Container& s, const Container& sep) {
    SatString hay, needle;
    load_str(hay, s);
    load_str(needle, sep);

    Container out = Container::list();
    auto& items = out.as_list()->items;

    if (needle.len == 0) {                            // empty separator: characters
        for (uint32_t k = 0; k < hay.len; ++k) {
            auto* p = new SatString();
            p->append(hay.data() + k, 1);
            items.push_back(wrap(p));
        }
        return out;
    }

    uint32_t from = 0;
    for (;;) {
        int64_t at = hay.find(needle, from);
        if (at < 0) break;
        auto* p = new SatString();
        p->assign_slice(hay, from, (uint32_t)at - from);
        items.push_back(wrap(p));
        from = (uint32_t)at + needle.len;
    }
    auto* tail = new SatString();
    tail->assign_slice(hay, from, hay.len - from);
    items.push_back(wrap(tail));
    return out;
}

Container str_join(const Container& list, const Container& sep) {
    if (list.type != Type::List) return Container::nil();
    SatString s;
    load_str(s, sep);
    auto* p = new SatString();
    const auto& items = list.as_list()->items;
    for (size_t k = 0; k < items.size(); ++k) {
        if (k) p->append(s.data(), s.len);
        if (items[k].type == Type::Str) {
            const SatString* q = items[k].as_str();
            p->append(q->data(), q->len);
        } else {
            p->append_text(items[k].to_string());
        }
    }
    return wrap(p);
}

static Container transform(const Container& s, void (SatString::*fn)()) {
    auto* p = new SatString();
    load_str(*p, s);
    (p->*fn)();
    return wrap(p);
}

Container str_upper(const Container& s)   { return transform(s, &SatString::upper); }
Container str_lower(const Container& s)   { return transform(s, &SatString::lower); }
Container str_trim(const Container& s)    { return transform(s, &SatString::trim); }
Container str_reverse(const Container& s) { return transform(s, &SatString::reverse); }

Container str_replace_all(const Container& s, const Container& from, const Container& to) {
    auto* p = new SatString();
    load_str(*p, s);
    SatString f, t;
    load_str(f, from);
    load_str(t, to);
    p->replace_all(f, t);
    return wrap(p);
}

// digits are codes 53-62, so the numeric value of a character is a subtraction
Container str_to_number(const Container& s) {
    SatString v;
    load_str(v, s);
    uint32_t k = 0;
    bool neg = false;
    while (k < v.len && charmap::is_space(v.at(k))) ++k;
    if (k < v.len && (v.at(k) == charmap::OP_MINUS || v.at(k) == 73)) { neg = true; ++k; }

    Container acc = Container::integer(0);
    bool any = false;
    for (; k < v.len && charmap::is_digit(v.at(k)); ++k) {
        acc = add(mul(acc, Container::integer(10)),
                  Container::integer(charmap::digit_value(v.at(k))));
        any = true;
    }
    if (k < v.len && v.at(k) == 89) {                 // '.' -- a real
        double frac = 0, scale = 0.1;
        ++k;
        for (; k < v.len && charmap::is_digit(v.at(k)); ++k, scale *= 0.1) {
            frac += charmap::digit_value(v.at(k)) * scale;
            any = true;
        }
        double whole = acc.type == Type::Int ? (double)acc.i : 0.0;
        return Container::real(neg ? -(whole + frac) : whole + frac);
    }
    if (!any) return Container::nil();
    return neg ? sub(Container::integer(0), acc) : acc;
}

// ---------------------------------------------------------------------------
// division, equality, ordering
// ---------------------------------------------------------------------------
BinFn div_table[TCOUNT][TCOUNT];
BinFn eq_table[TCOUNT][TCOUNT];
BinFn lt_table[TCOUNT][TCOUNT];

static Container div_int(const Container& a, const Container& b) {
    if (b.i == 0) return Container::nil();                 // division by zero
    if (a.i == std::numeric_limits<int64_t>::min() && b.i == -1)
        return big::mul(*Container::big_from_i64(a.i).as_big(),
                        *Container::big_from_i64(-1).as_big());
    return Container::integer(a.i / b.i);
}
static Container div_real(const Container& a, const Container& b) {
    double r = to_double(b);
    if (r == 0.0) return Container::nil();
    return Container::real(to_double(a) / r);
}
static Container div_str(const Container& a, const Container& b) { return str_split(a, b); }

// `"ab" * 3` repeats; also int * str
static Container mul_str(const Container& a, const Container& b) {
    const Container& s = a.type == Type::Str ? a : b;
    const Container& n = a.type == Type::Str ? b : a;
    if (n.type != Type::Int || n.i < 0) return Container::nil();
    auto* p = new SatString();
    load_str(*p, s);
    p->repeat((uint32_t)n.i);
    return wrap(p);
}

static bool same(const Container& a, const Container& b) {
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

static Container op_eq(const Container& a, const Container& b) {
    return Container::boolean(same(a, b));
}
static Container op_lt(const Container& a, const Container& b) {
    if (a.type == Type::Str && b.type == Type::Str)
        return Container::boolean(a.as_str()->compare(*b.as_str()) < 0);
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

void init_tables() {
    for (int x = 0; x < TCOUNT; ++x)
        for (int y = 0; y < TCOUNT; ++y) {
            add_table[x][y] = op_unsupported;
            sub_table[x][y] = op_unsupported;
            mul_table[x][y] = op_unsupported;
            div_table[x][y] = op_unsupported;
            eq_table[x][y]  = op_eq;         // every pair is comparable
            lt_table[x][y]  = op_unsupported;
        }

    constexpr int I = (int)Type::Int, B = (int)Type::Big, R = (int)Type::Real;
    constexpr int S = (int)Type::Str, L = (int)Type::List, O = (int)Type::Bool;
    constexpr int N = (int)Type::Nil;

    add_table[I][I] = add_int_int;
    sub_table[I][I] = sub_int_int;
    mul_table[I][I] = mul_int_int;

    for (int t : {I, B}) {
        add_table[B][t] = add_big_any;  add_table[t][B] = add_big_any;
        sub_table[B][t] = sub_big_any;  sub_table[t][B] = sub_big_any;
        mul_table[B][t] = mul_big_any;  mul_table[t][B] = mul_big_any;
    }

    for (int t : {I, B, R, O}) {
        add_table[R][t] = add_real;  add_table[t][R] = add_real;
        sub_table[R][t] = sub_real;  sub_table[t][R] = sub_real;
        mul_table[R][t] = mul_real;  mul_table[t][R] = mul_real;
    }

    // strings absorb everything on +
    for (int t = 0; t < TCOUNT; ++t) {
        add_table[S][t] = add_str;
        add_table[t][S] = add_str;
    }
    sub_table[S][S] = sub_str;
    mul_table[S][I] = mul_str;
    mul_table[I][S] = mul_str;
    div_table[S][S] = div_str;              // "a,b,c" / "," splits

    div_table[I][I] = div_int;
    for (int t : {I, B, R, O})
        for (int u : {I, B, R, O})
            if (t == R || u == R) div_table[t][u] = div_real;

    // ordering: numbers among themselves, strings among themselves
    for (int t : {I, B, R, O})
        for (int u : {I, B, R, O}) lt_table[t][u] = op_lt;
    lt_table[S][S] = op_lt;

    add_table[L][L] = add_list;

    (void)N;
}

}  // namespace satellite