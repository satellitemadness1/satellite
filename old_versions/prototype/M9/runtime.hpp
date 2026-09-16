#pragma once

// The satellite Program Runtime & Execution Engine -- Milestone 9 Prototype.
//
// PLAN §8 (M9): Executes control flow, boolean expressions, and string operations.

#include "compiler.hpp"
#include "closure.hpp"
#include "evaluator.hpp"
#include "console.hpp"
#include "dispatch.hpp"
#include "interrupt.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <vector>

namespace satellite {

struct RunResult {
    int exit_code = 0;
    bool success = true;
    bool interrupted = false;
    std::string error_message;
    Value return_value = Value::nil();
    std::vector<std::string> output;
};

class Runtime {
public:
    explicit Runtime(int max_depth = compute_derived_max_depth());

    // Execute satellite source string directly
    RunResult run_string(const std::string &source);

    // Execute satellite program file from disk
    RunResult run_file(const std::string &path);

    // Drain console output barrier
    void drain() { Console::instance().drain(); }

    void set_max_depth(int limit) { max_depth_ = limit; }

private:
    int max_depth_;
};

} // namespace satellite

