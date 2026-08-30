#pragma once

// Multi-line block scanner and command line parser for REPL.
// Milestone 11 Prototype in prototype/M11.

#include <string>
#include <vector>

namespace satellite {

struct BlockScan {
    int depth = 0;
    bool opens_body = false;
    bool lex_error = false;
};

// Scans line tokens to count brace depth and detect auto-body declaration heads
BlockScan scan_block(const std::string &line);

struct RunCommand {
    bool matched = false;
    std::string path;
    std::vector<std::string> args;
    std::string error;
};

// Parses `run <file> [args]`, `interpret ...`, `--run ...`
RunCommand parse_run_command(const std::string &line);

// Recognizes exit, quit, exit(), quit(), satellite.return(...)
bool exit_command(const std::string &line, int &status);

// Formats and prints abandoned multi-line block diagnostic
void abandon_block(std::string &block, int &depth);

} // namespace satellite

