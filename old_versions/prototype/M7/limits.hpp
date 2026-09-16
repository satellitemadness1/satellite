#pragma once

// The satellite Recursion Depth Limits & System Dials -- Milestone 7 Prototype.
//
// DESIGN §7.5 & PLAN §8: Bounded recursion depth derived from RLIMIT_STACK.
// Default depth is 2000; exceeds produce clean "capsule call too deep" error.

#include "system_facts/system.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace satellite {

inline constexpr int DEFAULT_MAX_DEPTH = 2000;
inline constexpr unsigned long long STACK_FRAME_ESTIMATE_BYTES = 3169; // Measured at -O2

class DepthExceededError : public std::runtime_error {
public:
    explicit DepthExceededError(const std::string &msg = "capsule call too deep")
        : std::runtime_error(msg) {}
};

inline int compute_derived_max_depth()
{
    unsigned long long stack_bytes = stack_limit_bytes();
    if (stack_bytes == STACK_LIMIT_UNKNOWN || stack_bytes == 0)
        return DEFAULT_MAX_DEPTH;

    // Derive ceiling from stack size reserving 512KB for runtime/OS
    if (stack_bytes > 512 * 1024) {
        unsigned long long avail = stack_bytes - 512 * 1024;
        int derived = static_cast<int>(avail / STACK_FRAME_ESTIMATE_BYTES);
        if (derived < 50) derived = 50;
        if (derived > 50000) derived = 50000;
        return derived;
    }
    return DEFAULT_MAX_DEPTH;
}

struct RecursionGuard {
    int &depth;
    int limit;

    RecursionGuard(int &current_depth, int max_limit)
        : depth(current_depth), limit(max_limit)
    {
        depth++;
        if (depth > limit) {
            depth--;
            throw DepthExceededError("capsule call too deep");
        }
    }

    ~RecursionGuard()
    {
        depth--;
    }

    RecursionGuard(const RecursionGuard &) = delete;
    RecursionGuard &operator=(const RecursionGuard &) = delete;
};

} // namespace satellite

