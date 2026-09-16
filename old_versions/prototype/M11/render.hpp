#pragma once

// Drawing one prompt line onto a terminal, wrapped across rows.
// Milestone 11 Prototype in prototype/M11.

#include <cstddef>
#include <string>

namespace satellite {

// Returns terminal column width via TIOCGWINSZ (80 fallback).
int terminal_columns();

class Renderer {
public:
    void draw(const std::string &prompt, const std::string &line, size_t cursor);
    void reset();
    void clear_screen();

private:
    int max_rows_ = 1;
    int cursor_row_ = 1;
};

} // namespace satellite

