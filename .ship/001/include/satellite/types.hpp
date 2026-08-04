// The satellite type tag and the refcounted heap-object base.
#pragma once

#include <atomic>
#include <cstdint>

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
    Capsule, // satellite.capsule -- a name, its parameters, and a span of source
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

}  // namespace satellite
