// String and list operators, and the str_* helpers the library calls directly.
//
// Characters are satellite codes throughout -- see charmap.hpp.  String-to-
// string work copies codes straight across with no encode/decode step; only a
// non-string operand pays for a conversion, through load_str.
#include "ops_internal.hpp"

namespace satellite {
namespace ops {

// "a" + "b"  ->  concatenation.  Any type concatenated with a string becomes
// a string, so satellite.console.display("n: " + 5) works as expected.
Container add_str(const Container& a, const Container& b) {
    auto* p = new SatString();
    load_str(*p, a);
    load_str(*p, b);
    return wrap(p);
}

// "my_string" - "str"  ->  "my_ing"
// DEFAULT DECISION (change if you want different semantics): removes the FIRST
// occurrence only.  Alternatives are all-occurrences or trailing-only.
Container sub_str(const Container& a, const Container& b) {
    auto* p = new SatString();
    load_str(*p, a);
    SatString needle;
    load_str(needle, b);

    int64_t at = p->find(needle);
    if (at >= 0) p->erase((uint32_t)at, needle.len);
    return wrap(p);
}

// `"ab" * 3` repeats; also int * str
Container mul_str(const Container& a, const Container& b) {
    const Container& s = a.type == Type::Str ? a : b;
    const Container& n = a.type == Type::Str ? b : a;
    if (n.type != Type::Int || n.i < 0) return Container::nil();
    auto* p = new SatString();
    load_str(*p, s);
    p->repeat((uint32_t)n.i);
    return wrap(p);
}

// `"a,b,c" / ","` splits into a list
Container div_str(const Container& a, const Container& b) { return str_split(a, b); }

// list + list -> concatenation
Container add_list(const Container& a, const Container& b) {
    Container out = Container::list();
    auto& dst = out.as_list()->items;
    for (const auto& x : a.as_list()->items) dst.push_back(x);
    for (const auto& x : b.as_list()->items) dst.push_back(x);
    return out;
}

}  // namespace ops

// ---------------------------------------------------------------------------
// String helpers that build new containers.  These are declared in
// container.hpp and called from the library, so they stay in namespace
// satellite rather than moving into ops.
// ---------------------------------------------------------------------------
using ops::load_str;
using ops::wrap;

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

}  // namespace satellite
