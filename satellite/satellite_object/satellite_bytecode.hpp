#pragma once
// satellite/satellite_object/satellite_bytecode.hpp -- the numbered program as a
// VALUE, so it can be an arm of the object model's variant beside a number and a
// string.
//
// (the author, 2026-09-16) "we must include satellite_bytecode into the
// satelliteObject and satelliteUserDefinedObject... keep the bytecode in, it
// makes it easier, it gives you more options for fast path functions."
//
// WHY THIS IS A TYPE AND NOT A POINTER INTO THE REGISTRY. The interpreter's
// BytecodeRegistry is one structure for the whole program -- one row a FILE --
// and a CapsuleSite into it is two integers. That shape is still there and still
// right for the WALKER. This type is the other thing: bytecode held as a value a
// program can be handed, stored in a variable, joined to another, and asked
// about. A capsule holds one of these (satellite_capsule.hpp), and that is what
// makes `satelliteCapsule` a thing the object model can carry rather than a
// position only the walker understands.
//
// IT COSTS NOTHING TO INCLUDE. A std::vector is 24 bytes whatever it holds, so
// putting bytecode in the variant does not widen the object by one byte -- the
// codes live on the heap and the arm is a handle. Measured 2026-09-16: the
// variant is 80 bytes with this arm and 80 bytes without it.
//
// A CODE IS std::bitset<16> AND NOT uint16_t, which is the author's decision and
// is written down with its cost: libstdc++ rounds a bitset<16> to 8 bytes, so a
// pass costs 3.5-5x what uint16_t would and four times the memory. He chose it
// knowing that, because `.to_string()` is the sixteen binary digits
// REGISTRY.satellite's own column is written in, and what the converters print.

#include <bitset>
#include <cstddef>
#include <vector>

namespace satellite004 {

// One run of 16-bit codes: a capsule's body, a file, or a piece a program built.
//
// Deliberately a struct with its codes in the open. Everything in this language
// that reads bytecode reads it as a sequence, and a wrapper that hid the vector
// would be a wrapper every caller had to work around.
struct satellite_bytecode {
    std::vector<std::bitset<16>> codes;

    satellite_bytecode() = default;
    explicit satellite_bytecode(std::vector<std::bitset<16>> from) : codes(std::move(from)) {}

    std::size_t size() const { return codes.size(); }
    bool empty() const { return codes.empty(); }

    // The code at `index`, or 0 -- the registry's own *nothing* -- past the end.
    // No throw and no refusal: reading past the end of a program is how a walker
    // discovers it has reached the end, and it happens on the ordinary path.
    unsigned int at(std::size_t index) const
    {
        return index < codes.size() ? static_cast<unsigned int>(codes[index].to_ulong()) : 0u;
    }

    void append(const satellite_bytecode &other)
    {
        codes.insert(codes.end(), other.codes.begin(), other.codes.end());
    }
    void append_code(unsigned int code) { codes.push_back(std::bitset<16>(code)); }
    void clear() { codes.clear(); }

    friend bool operator==(const satellite_bytecode &l, const satellite_bytecode &r) { return l.codes == r.codes; }
    friend bool operator!=(const satellite_bytecode &l, const satellite_bytecode &r) { return !(l == r); }
};

} // namespace satellite004
