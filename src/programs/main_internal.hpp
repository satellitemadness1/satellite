#pragma once

// Private to src/programs/, and to the `satl` binary only -- satl-term links
// window.o and nothing here.
//
// main.cpp was 565 lines. The 2026-08-24 restructure split it four ways at the
// seams it already had: the prompt, the two ways a program is run, the repl
// loop, and what is left, which is argument handling and main() itself.
//
// Everything below lost its `static` for one reason: internal linkage cannot
// cross a file boundary. That is the same trade eval_internal.hpp records for
// the evaluator, and it is the ONLY semantic change the split made.

#include <string>
#include <vector>

#include "console_output/console.hpp"
#include "satellite_value/value.hpp"
#include "interpreter/interp.hpp"

// --- the prompt, main_prompt.cpp -------------------------------------------
// Written by choose_prompt_colors() and read by the repl when it draws a line.
extern const char *color_user;
extern const char *color_cwd;
extern const char *color_off;
void choose_prompt_colors();
void evaluate(const std::string &line, satellite::Console *console);
satellite::Value parse_value(const std::string &text);
void debug_command(const std::string &line);

// --- running a program, main_run.cpp ---------------------------------------
void start_runtime();
int run_file_mode(const std::string &path,
                  const std::vector<std::string> &args);
void run_command(const satellite::RunCommand &command,
                 satellite::Console *console);

// --- the repl, main_repl.cpp -----------------------------------------------
bool exit_command(const std::string &line, int &status);
void abandon_block(std::string &block, int &depth);
int run_repl();
