#pragma once

// The satellite Handler Dispatch Table & M10 Handlers -- Milestone 10 Prototype.
//
// PLAN §8 (M10): Containers & the Search Power.
// Registers handlers for:
// - satellite.container (1 4 (0))
// - satellite.container.map (1 4 1 (0)) & 9 methods (1 4 1 1 .. 1 4 1 9)
// - satellite.container.list (1 4 2 (0)) & 25 methods (1 4 2 1 .. 1 4 2 25)
// - satellite.system.threshold() (1 22 5) & satellite.system.threshold(n) (1 22 6)
// - .search(pattern) on containers
// - Retains all prior M7/M8/M9 handlers.

#include "satellite_words/words.hpp"
#include "value.hpp"
#include "search.hpp"

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

// Initializes all Milestone 10 handlers
void init_m10_dispatch();

} // namespace satellite

