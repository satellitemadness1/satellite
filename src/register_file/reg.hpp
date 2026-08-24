#pragma once

// One slot in the virtual machine's register file — §17.4.
//
// Status: the register TYPE and the stack that holds it. There is no VM yet,
// and nothing in the interpreter includes this file. The tree walker of §10 is
// still what runs, and stays the authority until a VM produces byte-identical
// output and identical error vectors.
//
// WHAT THIS IS FOR. §17 measured the tree walker and found two costs. One is
// dispatch — a recursive eval() call and a variant dispatch per node, 75% of an
// addition — and that is what bytecode removes. The other is the remaining 25%:
// `ValuePtr` is `shared_ptr<const Value>`, so every intermediate result is a
// make_shared, a malloc plus an ATOMIC refcount, to add two integers. No
// dispatch strategy touches that. A VM built on `ValuePtr` would remove the 75%,
// keep the 25%, land near 1.6x and read as evidence that bytecode was oversold.
// This type is how the 25% goes.
//
// WHAT IT IS NOT. `Value` does not change, and neither do `ValuePtr`, `Library`
// or `Object::fields`. A design pass measured a hybrid register — inline for the
// common case, `ValuePtr` for everything else — against a full 16-byte tagged
// union at 15.38 against 15.91 ns/iter, which is a wash. The hybrid buys the
// same speed without reopening the lock-free publish protocol that
// ThreadSanitizer currently verifies, and that trade is why `Value` is left
// alone. Do not rewrite it.
//
// A REGISTER IS NEVER ALLOCATED. §17.4: one array exists, and a register is a
// slot in it. Register 5 of the current frame is `base[5]`. A design where each
// register were a separately allocated object would put a malloc back on every
// value and land exactly where the tree walker already is — the same mistake
// under a new name.

#include "satellite_number/bignum.hpp"
#include "satellite_value/value.hpp"

#include <cstddef>
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

// ---------------------------------------------------------------------------
// RegStack — §17.4's "a frame is a window, not an allocation"
// ---------------------------------------------------------------------------

// One array, allocated once. Calling advances `base`, returning restores it:
// two pointer adjustments and no allocator, which deletes the per-activation
// `Frame` vector that is part of the measured 600 ns per loop iteration.
//
// FIXED SIZE, and that is not a convenience. A growing std::vector would
// reallocate and invalidate every frame base being held — a use-after-free that
// surfaces as inexplicable garbage rather than a clean crash. The size is fixed
// up front and exhaustion is reported.
//
// The ceiling is the one the language ALREADY promises. src/evaluator/helpers.cpp carries
// DEFAULT_MAX_DEPTH = 2000, reachable through satellite.library.system.max_depth,
// so the bound here is chosen to honour that rather than to fit a cache: a
// program that recurses to the documented limit must not run out of registers
// first.
//
// **That promise is no longer a constant, and this number has not caught up.**
// The ceiling used to be a fixed 3000, which is where 3000 activations came
// from. It is now derived from RLIMIT_STACK — still 3000 on the ordinary 8 MB
// stack, but ~24,000 on 64 MB and ~8.2 million activations on 64 GB. A VM built
// on this stack as sized would report exhaustion far below what the tree walker
// accepts on a raised `ulimit -s`, which inverts the rule above.
//
// Two ways out when the VM is real, and this is not the file that picks one:
// size the stack from max_max_depth() at startup, since the allocation is an
// mmap and untouched pages never become resident, or keep a bound of its own
// and say plainly that the VM recurses less deeply than the walker. Nothing is
// broken today — no translation unit includes this header.
//
// It is deliberately NOT sized to fit L1. §17 measured a 2,000x growth in
// working set as a 5% change in per-operation cost, so anything justified by
// cache residency is playing for 5% — and the working set here is the top frame
// or two, which stays hot on its own without being designed for.
class RegStack {
public:
    // 3000 activations at 64 slots each. 64 is generous for one capsule's
    // locals, temporaries and argument staging, and being generous is the point:
    // the depth guard should be what stops a runaway recursion, because it
    // reports a satellite error with a span, and slot exhaustion is a blunter
    // failure that should stay unreachable in practice.
    static constexpr size_t DEFAULT_SLOTS = 3000 * 64;

    // RAW STORAGE, AND SLOTS CONSTRUCTED ON DEMAND — and this is measured, not
    // fastidiousness. `new Reg[DEFAULT_SLOTS]` runs a constructor per slot,
    // because a Reg holds a ValuePtr and is therefore not trivially
    // constructible, and that writes every one of the 5.86 MB. Measured at
    // **3.2 ms**, against a satl startup of about 2.5 ms: it would more than
    // double the cost of running hello world, to prepare 3000 frames for a
    // program that will use four.
    //
    // The allocation itself is nearly free — a request this size becomes an
    // mmap, and untouched pages never become resident — so the cost was
    // entirely in constructing slots nobody had asked for. Slots are now
    // constructed as the stack grows into them, `constructed_` being the
    // high-water mark, and a program pays for the depth it actually reaches.
    // The array still never moves, which is the property §17.4 requires.
    explicit RegStack(size_t slots = DEFAULT_SLOTS)
        : slots_(slots),
          storage_(new std::byte[slots * sizeof(Reg)]),
          regs_(reinterpret_cast<Reg *>(storage_.get()))
    {
    }

    ~RegStack()
    {
        for (size_t i = constructed_; i-- > 0;)
            regs_[i].~Reg();
    }

    RegStack(const RegStack &) = delete;
    RegStack &operator=(const RegStack &) = delete;

    // A frame is [base, base + count). Nothing is copied; the slots are already
    // where the callee expects them. Slots above the high-water mark are
    // constructed here, which is the only place they ever are.
    bool push_frame(size_t count, size_t &base)
    {
        if (count > slots_ || top_ > slots_ - count)
            return false;                       // exhausted; caller reports it
        base = top_;
        top_ += count;
        for (; constructed_ < top_; constructed_++)
            new (regs_ + constructed_) Reg();
        return true;
    }

    // Returning restores the mark. The slots above it are released rather than
    // cleared, so a heap register's refcount is dropped but an inline one costs
    // nothing to abandon.
    void pop_frame(size_t base)
    {
        for (size_t i = base; i < top_; i++)
            regs_[i] = Reg();
        top_ = base;
    }

    Reg *at(size_t base) { return regs_ + base; }
    const Reg *at(size_t base) const { return regs_ + base; }

    size_t top() const { return top_; }
    size_t capacity() const { return slots_; }

    // How many slots have ever been constructed — the deepest this program has
    // been, in slots. Reported by reg_test to keep the laziness honest.
    size_t constructed() const { return constructed_; }

private:
    size_t slots_;
    size_t top_ = 0;
    size_t constructed_ = 0;
    std::unique_ptr<std::byte[]> storage_;
    Reg *regs_;
};

} // namespace satellite
