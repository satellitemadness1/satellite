#pragma once

// The satellite Activation Frame -- Milestone 7 Prototype.
//
// DESIGN §7.2 & PLAN §8: Per-activation frame with integer slot indexing.
// Guarantees isolation across recursion and threads with zero atomic or locking overhead.

#include "value.hpp"

#include <cstdint>
#include <vector>

namespace satellite {

struct Frame {
    std::vector<Value> slots;

    explicit Frame(size_t slot_count = 0) : slots(slot_count) {}

    Value get(size_t slot) const {
        if (slot < slots.size())
            return slots[slot];
        return Value::nil();
    }

    void set(size_t slot, Value val) {
        if (slot >= slots.size())
            slots.resize(slot + 1);
        slots[slot] = std::move(val);
    }
};

} // namespace satellite

