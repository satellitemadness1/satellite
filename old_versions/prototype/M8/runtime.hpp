#pragma once

// The satellite Program Runtime & Execution Engine -- Milestone 8 Prototype.
//
// PLAN §8 (M8.A): This is the milestone at which satellite executes anything at all.
// Integrates the full pipeline (Lexer -> Parser -> Resolver -> Compiler -> Runtime),
// runs bare satellite.main(), enforces Console drain() barriers, and maps
// return values to process exit codes.

#include "compiler.hpp"
#include "closure.hpp"
#include "evaluator.hpp"
#include "console.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <vector>

namespace satellite {

// Initializes Milestone 8 dispatch handlers (Console display, return shapes)
void init_m8_dispatch();

struct RunResult {
    int exit_code = 0;
    bool success = true;
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
