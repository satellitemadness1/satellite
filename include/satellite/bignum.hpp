// satellite_number's big form -- reached only when an int64 op overflows.
#pragma once

#include <cstdint>
#include <vector>

#include "satellite/types.hpp"

namespace satellite {

// satellite_number's big form.  Little-endian limbs, no trailing zero limb.
// Reached only when an int64 operation actually overflows.
struct BigInt : Obj {
    bool                  neg = false;
    std::vector<uint64_t> limbs;

    BigInt() : Obj(Type::Big) {}
    bool is_zero() const { return limbs.empty(); }
    void trim() { while (!limbs.empty() && limbs.back() == 0) limbs.pop_back(); }
};

}  // namespace satellite
