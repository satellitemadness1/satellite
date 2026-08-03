#include "satellite/container.hpp"

#include <cstring>
#include <limits>

namespace satellite {

const char* type_name(Type t) {
    switch (t) {
        case Type::Nil:  return "nil";
        case Type::Bool: return "bool";
        case Type::Int:  return "int";
        case Type::Big:  return "big";
        case Type::Real: return "real";
        case Type::Str:  return "string";
        case Type::List: return "list";
        default:         return "?";
    }
}

// ===========================================================================
// The satellite character map
// ===========================================================================
namespace charmap {

// index 0 is void; '?' is only a placeholder so the table indexes line up
static const char kTable[] =
    "?"                                 // 0     void / error
    "abcdefghijklmnopqrstuvwxyz"        // 1-26
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"        // 27-52
    "0123456789"                        // 53-62
    "!@#$%^&*()"                        // 63-72
    "-_=+"                              // 73-76
    "[{]}\\|"                           // 77-82
    ";:'\","                            // 83-87
    "<.>/?"                             // 88-92
    "`~"                                // 93-94
    " \n\t";                            // 95-97  space, newline, tab

const char* table() { return kTable; }

// reverse lookup, built once
static const unsigned char* reverse() {
    static unsigned char rev[256] = {0};
    static bool built = false;
    if (!built) {
        for (uint32_t code = 1; code < TEXT_COUNT; ++code)
            rev[(unsigned char)kTable[code]] = (unsigned char)code;
        built = true;
    }
    return rev;
}

char32_t from_ascii(char c) { return reverse()[(unsigned char)c]; }

char to_ascii(char32_t code) {
    return code > 0 && code < TEXT_COUNT ? kTable[code] : '\0';
}

// ---- satellite math tokens ------------------------------------------------
static const char* kOps[] = {
    "+", "-", "*", "/", "%", "^", "=", "==", "!=", "<", ">", "<=", ">=",
};

bool is_operator(char32_t code) { return code >= OP_PLUS && code < COUNT; }

const char* op_text(char32_t code) {
    return is_operator(code) ? kOps[code - OP_PLUS] : "";
}

}  // namespace charmap

// ===========================================================================
// satellite_string -- 32 bits per character, holding satellite charmap codes
// ===========================================================================
void SatString::reserve(uint32_t want) {
    if (want <= cap) return;
    uint32_t ncap = cap * 2;
    if (ncap < want) ncap = want;
    char32_t* nb = new char32_t[ncap];
    std::memcpy(nb, data(), (size_t)len * sizeof(char32_t));
    delete[] heap;
    heap = nb;
    cap  = ncap;
}

void SatString::append(const char32_t* p, uint32_t n) {
    reserve(len + n);
    std::memcpy(data() + len, p, (size_t)n * sizeof(char32_t));
    len += n;
}

// Anything the charmap does not cover becomes code 0 (void).  That is the
// design: the map defines what a satellite character IS, and the 32-bit slot
// leaves room to extend it later without touching stored strings.
void SatString::append_text(const std::string& ascii) {
    reserve(len + (uint32_t)ascii.size());
    char32_t* d = data();
    for (char c : ascii) d[len++] = charmap::from_ascii(c);
}

std::string SatString::text() const {
    std::string out;
    out.reserve(len);
    const char32_t* d = data();
    for (uint32_t k = 0; k < len; ++k) {
        if (charmap::is_operator(d[k])) { out += charmap::op_text(d[k]); continue; }
        char c = charmap::to_ascii(d[k]);
        out += c ? c : '?';                 // void renders visibly
    }
    return out;
}

int64_t SatString::find(const SatString& needle, uint32_t from) const {
    if (needle.len == 0 || needle.len > len) return -1;
    const char32_t* h = data();
    const char32_t* n = needle.data();
    for (uint32_t k = from; k + needle.len <= len; ++k)
        if (std::memcmp(h + k, n, (size_t)needle.len * sizeof(char32_t)) == 0)
            return (int64_t)k;
    return -1;
}

void SatString::erase(uint32_t at, uint32_t n) {
    if (at >= len) return;
    if (at + n > len) n = len - at;
    std::memmove(data() + at, data() + at + n, (size_t)(len - at - n) * sizeof(char32_t));
    len -= n;
}

uint32_t charmap::collation_key(char32_t code) {
    if (code == VOID) return 0;
    if (code < TEXT_COUNT) return (unsigned char)kTable[code];   // ASCII order
    if (is_operator(code)) return 256 + (code - OP_PLUS);
    return 512;
}

int64_t SatString::find_last(const SatString& needle) const {
    if (needle.len == 0 || needle.len > len) return -1;
    for (uint32_t k = len - needle.len + 1; k-- > 0;)
        if (std::memcmp(data() + k, needle.data(), (size_t)needle.len * sizeof(char32_t)) == 0)
            return (int64_t)k;
    return -1;
}

uint32_t SatString::count(const SatString& needle) const {
    if (needle.len == 0) return 0;
    uint32_t n = 0;
    for (uint32_t k = 0; k + needle.len <= len;) {
        if (std::memcmp(data() + k, needle.data(), (size_t)needle.len * sizeof(char32_t)) == 0) {
            ++n;
            k += needle.len;                       // non-overlapping
        } else {
            ++k;
        }
    }
    return n;
}

bool SatString::starts_with(const SatString& n) const {
    return n.len <= len &&
           std::memcmp(data(), n.data(), (size_t)n.len * sizeof(char32_t)) == 0;
}

bool SatString::ends_with(const SatString& n) const {
    return n.len <= len &&
           std::memcmp(data() + (len - n.len), n.data(),
                       (size_t)n.len * sizeof(char32_t)) == 0;
}

int SatString::compare(const SatString& o) const {
    uint32_t n = len < o.len ? len : o.len;
    for (uint32_t k = 0; k < n; ++k) {
        uint32_t a = charmap::collation_key(data()[k]);
        uint32_t b = charmap::collation_key(o.data()[k]);
        if (a != b) return a < b ? -1 : 1;
    }
    if (len == o.len) return 0;
    return len < o.len ? -1 : 1;
}

void SatString::insert(uint32_t at, const char32_t* p, uint32_t n) {
    if (at > len) at = len;
    reserve(len + n);
    char32_t* d = data();
    std::memmove(d + at + n, d + at, (size_t)(len - at) * sizeof(char32_t));
    std::memcpy(d + at, p, (size_t)n * sizeof(char32_t));
    len += n;
}

void SatString::replace_all(const SatString& from, const SatString& to) {
    if (from.len == 0) return;
    for (uint32_t k = 0; k + from.len <= len;) {
        if (std::memcmp(data() + k, from.data(), (size_t)from.len * sizeof(char32_t)) == 0) {
            erase(k, from.len);
            insert(k, to.data(), to.len);
            k += to.len;                           // never rescan what we wrote
        } else {
            ++k;
        }
    }
}

void SatString::remove_all(const SatString& needle) {
    if (needle.len == 0) return;
    for (uint32_t k = 0; k + needle.len <= len;) {
        if (std::memcmp(data() + k, needle.data(), (size_t)needle.len * sizeof(char32_t)) == 0)
            erase(k, needle.len);
        else
            ++k;
    }
}

void SatString::upper() { char32_t* d = data(); for (uint32_t k=0;k<len;++k) d[k]=charmap::to_upper(d[k]); }
void SatString::lower() { char32_t* d = data(); for (uint32_t k=0;k<len;++k) d[k]=charmap::to_lower(d[k]); }

void SatString::trim() {
    uint32_t b = 0, e = len;
    while (b < e && charmap::is_space(data()[b]))     ++b;
    while (e > b && charmap::is_space(data()[e - 1])) --e;
    if (b) erase(0, b);
    len = e - b;
}

void SatString::reverse() {
    char32_t* d = data();
    for (uint32_t k = 0; k < len / 2; ++k) std::swap(d[k], d[len - 1 - k]);
}

void SatString::repeat(uint32_t times) {
    if (times == 0) { len = 0; return; }
    uint32_t base = len;
    reserve(base * times);
    char32_t* d = data();
    for (uint32_t k = 1; k < times; ++k)
        std::memcpy(d + base * k, d, (size_t)base * sizeof(char32_t));
    len = base * times;
}

void SatString::assign_slice(const SatString& o, uint32_t at, uint32_t n) {
    len = 0;
    if (at >= o.len) return;
    if (at + n > o.len) n = o.len - at;
    append(o.data() + at, n);
}

// ===========================================================================
// Container lifecycle
// ===========================================================================
Container Container::str(const char* s) {
    return str(std::string(s));
}

Container Container::str(const std::string& s) {
    auto* p = new SatString();
    p->append_text(s);
    Container c;
    c.type = Type::Str;
    c.obj  = p;
    return c;
}

Container Container::list() {
    Container c;
    c.type = Type::List;
    c.obj  = new List();
    return c;
}

Container Container::big_from_i64(int64_t v) { return big::from_i64(v); }

void Container::release() {
    if (!is_heap() || !obj) return;
    if (obj->rc.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        switch (obj->type) {
            case Type::Str:  delete static_cast<SatString*>(obj); break;
            case Type::Big:  delete static_cast<BigInt*>(obj);    break;
            case Type::List: delete static_cast<List*>(obj);      break;
            default:         delete obj;                          break;
        }
    }
    obj = nullptr;
}

Container& Container::operator=(const Container& o) {
    if (this == &o) return *this;
    Container tmp(o);            // retain first: handles aliasing safely
    release();
    type = tmp.type;
    i    = tmp.i;
    tmp.type = Type::Nil;
    tmp.i    = 0;
    return *this;
}

Container& Container::operator=(Container&& o) noexcept {
    if (this == &o) return *this;
    release();
    type = o.type;
    i    = o.i;
    o.type = Type::Nil;
    o.i    = 0;
    return *this;
}

bool Container::truthy() const {
    switch (type) {
        case Type::Nil:  return false;
        case Type::Bool: return b;
        case Type::Int:  return i != 0;
        case Type::Real: return d != 0.0;
        case Type::Big:  return !as_big()->is_zero();
        case Type::Str:  return as_str()->len != 0;
        case Type::List: return !as_list()->items.empty();
        default:         return false;
    }
}

std::string Container::to_string() const {
    switch (type) {
        case Type::Nil:  return "nil";
        case Type::Bool: return b ? "true" : "false";
        case Type::Int:  return std::to_string(i);
        case Type::Real: return std::to_string(d);
        case Type::Big:  return big::to_string(*as_big());
        case Type::Str:  return as_str()->text();
        case Type::List: {
            std::string s = "[";
            const auto& v = as_list()->items;
            for (size_t k = 0; k < v.size(); ++k) {
                if (k) s += ", ";
                s += v[k].to_string();
            }
            return s + "]";
        }
        default: return "?";
    }
}

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
