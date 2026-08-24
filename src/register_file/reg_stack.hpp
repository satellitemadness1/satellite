#pragma once

// Part of the register file — include "register_file/reg.hpp", which is the
// umbrella over this file and its siblings. Split out of reg.hpp verbatim; the
// header comment there is the one that explains why any of this exists.

#include "register_file/reg_slot.hpp"

#include <cstddef>
#include <memory>

namespace satellite {

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
