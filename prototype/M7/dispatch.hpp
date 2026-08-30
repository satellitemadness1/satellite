#pragma once

// The satellite Handler Dispatch Table -- Milestone 7 Prototype.
//
// DESIGN §4 & §6.4: O(1) array dispatch through handlers[path_id].
// Includes receiver-binding tag (DESIGN §6.4 qualification 2) to distinguish
// receiver methods from standalone module functions.

#include "value.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace satellite {

class ExecContext;

using HandlerFn = Value (*)(ExecContext &ctx, const std::vector<Value> &args);

struct HandlerEntry {
    HandlerFn fn = nullptr;
    bool binds_receiver = false; // DESIGN §6.4 qualification 2: does 1st arg bind receiver?
    uint32_t expected_arity = 0;
    const char *name = nullptr;

    bool is_valid() const { return fn != nullptr; }
};

class DispatchTable {
public:
    static DispatchTable &instance();

    void register_handler(words::PathId path_id, HandlerFn fn, bool binds_receiver = false,
                          uint32_t arity = 0, const char *name = nullptr);

    const HandlerEntry *get(words::PathId path_id) const;

    size_t size() const { return handlers_.size(); }

private:
    DispatchTable();
    void init_standard_handlers();

    std::vector<HandlerEntry> handlers_;
};

} // namespace satellite

