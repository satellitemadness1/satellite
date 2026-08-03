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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace satellite {

// ---------------------------------------------------------------------------
// Types.  Keep COUNT last -- the dispatch tables are sized from it.
// ---------------------------------------------------------------------------
enum class Type : uint8_t {
    Nil = 0,
    Bool,
    Int,     // fits in int64_t, stored inline, never allocates
    Big,     // promoted on overflow, heap limbs
    Real,
    Str,
    List,
    COUNT
};

const char* type_name(Type t);

// ---------------------------------------------------------------------------
// Heap payloads.  Refcounted, destroyed through the type tag rather than a
// vtable -- no virtual dispatch anywhere in the value model.
// ---------------------------------------------------------------------------
struct Obj {
    std::atomic<uint32_t> rc{1};
    Type type;
    explicit Obj(Type t) : type(t) {}

    // A copy is a NEW object, so it starts at one reference -- the refcount is
    // a property of the allocation, never of the value.  Spelling this out is
    // what lets BigInt be copied around as a plain value during arithmetic.
    Obj(const Obj& o) : rc(1), type(o.type) {}
    Obj& operator=(const Obj& o) { type = o.type; return *this; }
};

// ---------------------------------------------------------------------------
// The satellite character map.
//
// A satellite character is NOT a Unicode codepoint -- it is an index into this
// table.  Code 0 is void/error; everything else is assigned in order:
//
//   0        void / error
//   1-26     a-z          27-52    A-Z          53-62    0-9
//   63-72    !@#$%^&*()   73-76    -_=+         77-82    [{]}\|
//   83-87    ;:'",        88-92    <.>/?        93-94    `~
//   95-97    space, newline, tab   <-- added; nothing works without a space
//
// Stored in 32 bits so the map can grow without a migration.  Text outside the
// map converts to 0 (void).
// ---------------------------------------------------------------------------
namespace charmap {
constexpr char32_t VOID  = 0;
constexpr char32_t SPACE = 95;
constexpr char32_t TEXT_COUNT = 98;    // codes 0-97 are ordinary text

// ---- satellite math tokens ------------------------------------------------
// Deliberately NOT the same codes as the ASCII characters that look like them.
// A hyphen inside a string is code 73; SUBTRACTION is code 99.  Nothing can
// confuse the two, which is what lets tokenized code be stored as an ordinary
// satellite string and read back exactly.
constexpr char32_t OP_PLUS   = 98;
constexpr char32_t OP_MINUS  = 99;
constexpr char32_t OP_TIMES  = 100;
constexpr char32_t OP_DIVIDE = 101;
constexpr char32_t OP_MODULO = 102;
constexpr char32_t OP_POWER  = 103;
constexpr char32_t OP_ASSIGN = 104;
constexpr char32_t OP_EQ     = 105;
constexpr char32_t OP_NE     = 106;
constexpr char32_t OP_LT     = 107;
constexpr char32_t OP_GT     = 108;
constexpr char32_t OP_LE     = 109;
constexpr char32_t OP_GE     = 110;
constexpr char32_t COUNT     = 111;

char32_t    from_ascii(char c);        // ASCII -> TEXT code, never an operator
char        to_ascii(char32_t code);   // text code -> ASCII, 0 if not text
const char* table();                   // index by code; [0] is a placeholder

bool        is_operator(char32_t code);
const char* op_text(char32_t code);    // "+", "-", "<=" ... for display
}  // namespace charmap

// satellite_string.  One character is one char32_t holding a satellite charmap
// code, so indexing is exact and O(1): my_str.remove(0) removes the first
// CHARACTER, never part of one.
//
// Short strings live in the inline buffer and never touch the allocator; most
// strings in most programs are short, and allocation -- not copying -- is what
// actually costs.
struct SatString : Obj {
    static constexpr uint32_t INLINE_CAP = 16;   // characters, not bytes

    uint32_t  len  = 0;              // in characters
    uint32_t  cap  = INLINE_CAP;
    char32_t* heap = nullptr;        // null while the string is inline
    char32_t  buf[INLINE_CAP];

    SatString() : Obj(Type::Str) { buf[0] = 0; }
    ~SatString() { delete[] heap; }

    char32_t*       data()       { return heap ? heap : buf; }
    const char32_t* data() const { return heap ? heap : buf; }
    bool inline_stored() const { return heap == nullptr; }

    // rendered back to ASCII for display; void characters show as '?'
    std::string text() const;
    char32_t at(uint32_t i) const { return i < len ? data()[i] : charmap::VOID; }

    void reserve(uint32_t want);
    void append(const char32_t* p, uint32_t n);
    void append_text(const std::string& ascii);   // maps through the charmap

    // character index of `needle` at or after `from`, or -1
    int64_t find(const SatString& needle, uint32_t from = 0) const;
    void    erase(uint32_t at, uint32_t n);
};

// satellite_number's big form.  Little-endian limbs, no trailing zero limb.
// Reached only when an int64 operation actually overflows.
struct BigInt : Obj {
    bool                  neg = false;
    std::vector<uint64_t> limbs;

    BigInt() : Obj(Type::Big) {}
    bool is_zero() const { return limbs.empty(); }
    void trim() { while (!limbs.empty() && limbs.back() == 0) limbs.pop_back(); }
};

struct Container;

struct List : Obj {
    std::vector<Container> items;
    List() : Obj(Type::List) {}
};

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

    // --- refcounting -------------------------------------------------------
    Container(const Container& o) : type(o.type) { i = o.i; retain(); }
    Container(Container&& o) noexcept : type(o.type) { i = o.i; o.type = Type::Nil; o.i = 0; }
    Container& operator=(const Container& o);
    Container& operator=(Container&& o) noexcept;
    ~Container() { release(); }

    bool is_heap() const {
        return type == Type::Big || type == Type::Str || type == Type::List;
    }
    void retain()  { if (is_heap() && obj) obj->rc.fetch_add(1, std::memory_order_relaxed); }
    void release();

    // --- typed access ------------------------------------------------------
    SatString* as_str()  const { return static_cast<SatString*>(obj); }
    BigInt*    as_big()  const { return static_cast<BigInt*>(obj); }
    List*      as_list() const { return static_cast<List*>(obj); }

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
