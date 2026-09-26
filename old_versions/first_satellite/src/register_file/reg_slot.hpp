#pragma once

// Part of the register file — include "register_file/reg.hpp", which is the
// umbrella over this file and its siblings. Split out of reg.hpp verbatim; the
// header comment there is the one that explains why any of this exists.

#include "satellite_number/bignum.hpp"
#include "satellite_value/value.hpp"

#include <cstdint>
#include <memory>
#include <string>

namespace satellite {

// ---------------------------------------------------------------------------
// Reg
// ---------------------------------------------------------------------------

// A slot holds one of five things, and the FIVE is the point — four would be a
// bug that hides for a long time.
//
// A slot has three states a value model would collapse into two: nothing has
// been put here yet, something was put here and it is nil, and something was
// put here and it is a value. `Evaluator::read_slot` already depends on the
// first two being distinguishable — src/evaluator/slots.cpp reports "is read before its
// declaration runs" precisely by testing whether the cell is still empty, and it
// can do that today only because `ValuePtr` has a null state that no Value
// occupies. Zero a frame to nil instead and that check silently stops firing,
// because nil IS a value.
//
// So EMPTY is a tag of its own, and it is tag 0 so that a default-constructed
// Reg is empty without anyone writing a rule down twice.
struct Reg {
    enum Tag : uint8_t {
        EMPTY = 0,   // nothing has been written here
        NIL,         // satellite's nil — a value
        BOOL,
        SMALL,       // an exact decimal that fits: sig * 10^exp
        HEAP,        // everything else, behind the existing ValuePtr
    };

    // WHY THE LAYOUT IS PLAIN. `heap` is an ordinary member rather than a union
    // arm with hand-rolled lifetime, which costs 16 bytes in a slot holding an
    // integer. That is the hybrid the measurement blessed, and the alternative
    // is manual construct/destruct on every assignment — the kind of clever that
    // is wrong once and then wrong silently. Copy, move and destroy are the
    // compiler's problem here, exactly as §17.4 wants: "Value is an ordinary C++
    // struct ... and the correctness is the compiler's problem rather than the
    // VM author's."
    Tag tag = EMPTY;
    bool b = false;
    int32_t exp = 0;
    int64_t sig = 0;
    ValuePtr heap;

    Reg() = default;

    bool is_empty() const { return tag == EMPTY; }
    bool is_nil() const { return tag == NIL; }
    bool is_small() const { return tag == SMALL; }
    bool is_heap() const { return tag == HEAP; }

    // A slot holds a value once anything has been written to it, nil included.
    bool is_set() const { return tag != EMPTY; }

    static Reg nil()
    {
        Reg r;
        r.tag = NIL;
        return r;
    }
    static Reg boolean(bool value)
    {
        Reg r;
        r.tag = BOOL;
        r.b = value;
        return r;
    }
    // Takes the representation, not a number. The only caller who should have a
    // significand and an exponent is one who got them from Number::small_parts.
    static Reg small(long long significand, int exponent)
    {
        Reg r;
        r.tag = SMALL;
        r.sig = significand;
        r.exp = exponent;
        return r;
    }
    static Reg boxed(ValuePtr value)
    {
        Reg r;
        r.tag = HEAP;
        r.heap = std::move(value);
        return r;
    }

    // A Value into a slot, inline when it fits.
    //
    // The promotion boundary is Number::small_parts, which is the ONLY
    // sanctioned window onto the small form and returns false for exactly the
    // numbers that have none. §8.1's guarantee is untouched: the type is still
    // one exact arbitrary-precision decimal, and this is two representations of
    // one value rather than two answers.
    static Reg from_value(const ValuePtr &value)
    {
        if (!value)
            return Reg();                       // EMPTY, not nil
        if (std::holds_alternative<std::monostate>(*value))
            return nil();
        if (const bool *flag = std::get_if<bool>(value.get()))
            return boolean(*flag);
        if (const Number *n = std::get_if<Number>(value.get())) {
            long long significand = 0;
            int exponent = 0;
            if (n->small_parts(significand, exponent))
                return small(significand, exponent);
        }
        return boxed(value);
    }

    // Back out. Allocates only when the slot is inline and something outside the
    // register file needs a Value — which is the boundary the VM crosses when it
    // stores to satellite.library, builds a container, or reports an error.
    ValuePtr to_value() const
    {
        switch (tag) {
        case EMPTY:
            return nullptr;
        case NIL:
            return std::make_shared<const Value>(std::monostate{});
        case BOOL:
            return std::make_shared<const Value>(b);
        case SMALL:
            return std::make_shared<const Value>(Number::from_small(sig, exp));
        case HEAP:
            return heap;
        }
        return nullptr;
    }

    // For messages and tests. Deliberately agrees with to_string(Value) so a
    // slot and a Value never render the same thing two ways.
    std::string to_string() const
    {
        switch (tag) {
        case EMPTY:
            return "<unset>";
        case NIL:
            return "nil";
        case BOOL:
            return b ? "true" : "false";
        case SMALL:
            return Number::from_small(sig, exp).to_string();
        case HEAP:
            return heap ? satellite::to_string(*heap) : "nil";
        }
        return "<unset>";
    }
};

// The whole reason the inline case exists: an addition that allocates nothing.
//
// Returns false rather than being wrong. Any overflow anywhere — including
// ALIGNING the two exponents, which can overflow before a single digit has been
// added — falls back to the caller's bignum path, which is always available and
// always correct. `1 + 1e-30` is the shape that catches a scheme which only
// checks the addition: lining the decimal points up needs 10^30 first.
//
// The check itself is one instruction and a branch that never fires in the
// common case, so the predictor gets it right every time. Against the ~28 ns
// make_shared it avoids, it is roughly 30-to-1 in our favour.
inline bool add_inline(const Reg &a, const Reg &b, Reg &out)
{
    if (a.tag != Reg::SMALL || b.tag != Reg::SMALL)
        return false;

    long long sa = a.sig, sb = b.sig;
    int ea = a.exp, eb = b.exp;

    // Align to the smaller exponent. Bounded: a long long runs out of room after
    // about 19 multiplications, so this cannot spin.
    while (ea > eb) {
        if (__builtin_mul_overflow(sa, 10LL, &sa))
            return false;
        ea--;
    }
    while (eb > ea) {
        if (__builtin_mul_overflow(sb, 10LL, &sb))
            return false;
        eb--;
    }

    long long sum = 0;
    if (__builtin_add_overflow(sa, sb, &sum))
        return false;

    out = Reg::small(sum, ea);
    return true;
}

} // namespace satellite
