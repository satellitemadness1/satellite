#pragma once

// The satellite Handler Dispatch Table & M11 Handlers -- Milestone 11 Prototype.
//
// PLAN §8 (M11): The Prompt, Terminal Control & Window Console.
// Registers handlers for:
// - satellite.window.console (1 24 2 (0))
// - satellite.window.console.new(title, width, height) (1 24 2 1)
// - satellite.console.input() (1 5 2)
// - satellite.console.input(prompt) (1 5 3)
// - satellite.console.input(prompt, target) (1 5 4)
// - satellite.console.width (1 5 6)
// - satellite.console.height (1 5 7)
// - satellite.console.clear() (1 5 8)
// - satellite.console.home() (1 5 9)
// - All container methods, search power, statements, booleans, string methods from M8/M9/M10.

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

// Initializes all Milestone 11 handlers
void init_m11_dispatch();

} // namespace satellite

