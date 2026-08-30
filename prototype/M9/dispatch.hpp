#pragma once

// The satellite Handler Dispatch Table & M9 Handlers -- Milestone 9 Prototype.
//
// PLAN §8 (M9): Scalars & Control Flow.
// Registers handlers for:
// - satellite.bool.true (1 17 2), satellite.bool.false (1 17 1)
// - satellite.statement.if, .for, .while, .else
// - 16 methods on satellite.variable.string (1 6 1 1 .. 1 6 1 16)

#include "satellite_words/words.hpp"
#include "value.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace satellite {

class ExecContext;

using HandlerFn = std::function<Value(ExecContext &, const std::vector<Value> &)>;

struct DispatchEntry {
    words::PathId path_id = words::kNoPath;
    HandlerFn fn;
    bool binds_receiver = false;
    uint32_t arity = 0;
    std::string path_string;
};

class DispatchTable {
public:
    static DispatchTable &instance();

    void register_handler(words::PathId id, HandlerFn fn,
                          bool binds_receiver = false, uint32_t arity = 0,
                          std::string path_string = "");

    const DispatchEntry *get(words::PathId id) const;

    bool has(words::PathId id) const { return get(id) != nullptr; }

    void clear();

    size_t count() const { return entries_.size(); }

private:
    DispatchTable() = default;
    std::unordered_map<words::PathId, DispatchEntry> entries_;
};

// Initializes all Milestone 9 handlers
void init_m9_dispatch();

} // namespace satellite

