#pragma once

// The satellite REPL & Interactive Prompt Engine.
// Milestone 11 Prototype in prototype/M11.
//
// PLAN §8 (M11.B): The prompt, multi-line block entry, and persistent session state.

#include "runtime.hpp"
#include "line_reader.hpp"

#include <string>

namespace satellite {

extern const char *color_user;
extern const char *color_cwd;
extern const char *color_off;

void choose_prompt_colors();

// Evaluates a single line in context, returning output or expression string
std::string eval_line(const std::string &line, ExecContext &ctx, bool echo = true);

// Runs the interactive REPL loop
int run_repl(Runtime &rt);

} // namespace satellite

