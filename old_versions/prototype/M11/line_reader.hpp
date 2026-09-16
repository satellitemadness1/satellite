#pragma once

// High-level interactive line reader.
// Milestone 11 Prototype in prototype/M11.

#include "history.hpp"

#include <string>

namespace satellite {

enum class LineStatus {
    Line,         // typed line in `line`
    EndOfFile,    // Ctrl-D on empty line, or EOF
    Interrupted,  // Ctrl-C: line abandoned
};

class LineReader {
public:
    LineReader();
    ~LineReader();

    LineReader(const LineReader &) = delete;
    LineReader &operator=(const LineReader &) = delete;

    LineStatus read(const std::string &prompt, std::string &line);
    void remember(const std::string &line);

    const History &history() const { return history_; }

private:
    LineStatus read_cooked(const std::string &prompt, std::string &line);

    History history_;
};

} // namespace satellite

