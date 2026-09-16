#pragma once

// One line being typed: the buffer, cursor, and history browsing.
// Milestone 11 Prototype in prototype/M11.

#include "keys.hpp"

#include <cstddef>
#include <string>

namespace satellite {

class History;

class Editor {
public:
    explicit Editor(const History *history = nullptr);

    enum class Outcome {
        Continue,     // redraw and read on
        Accept,       // Enter: line is finished
        Interrupt,    // Ctrl-C: abandon this line
        EndOfInput,   // Ctrl-D on empty line
        ClearScreen,  // Ctrl-L: repaint screen
    };

    Outcome apply(const KeyEvent &event);

    const std::string &line() const { return buffer_; }
    size_t cursor() const { return cursor_; }

    void reset();
    void set_line(std::string text);

private:
    void recall(size_t index);
    void kill_word_back();

    std::string buffer_;
    size_t cursor_ = 0;

    const History *history_ = nullptr;
    size_t browse_ = 0;
    std::string live_;
    bool browsing_ = false;
};

} // namespace satellite

