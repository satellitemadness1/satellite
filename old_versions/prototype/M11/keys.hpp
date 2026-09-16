#pragma once

// Bytes from a terminal, turned into keys.
// Milestone 11 Prototype in prototype/M11.

#include <cstddef>
#include <string>

namespace satellite {

enum class Key {
    None,
    Char,        // `text` holds character or UTF-8 run
    Enter,
    Backspace,   // delete character before cursor
    Delete,      // delete character at cursor
    Left,
    Right,
    Up,
    Down,
    Home,
    End,
    WordLeft,
    WordRight,
    KillToStart,
    KillToEnd,
    KillWordBack,
    ClearScreen,
    Interrupt,    // Ctrl-C
    EndOfInput,   // Ctrl-D
    Ignored,
};

struct KeyEvent {
    Key key = Key::None;
    std::string text;
};

constexpr int TAB_WIDTH = 4;

class KeyDecoder {
public:
    KeyEvent feed(unsigned char byte);
    void reset();

private:
    enum class State {
        Ground,
        Escape,
        Csi,
        Ss3,
        Utf8,
    };

    KeyEvent ground(unsigned char byte);
    KeyEvent csi_final(unsigned char final_byte);

    State state_ = State::Ground;
    std::string params_;
    std::string pending_;
    int owed_ = 0;
};

// Computes display column width, counting UTF-8 chars as 1 column and skipping ANSI escape sequences.
int display_width(const std::string &text);

// Character boundary navigation skipping UTF-8 continuation bytes.
size_t prev_char(const std::string &text, size_t offset);
size_t next_char(const std::string &text, size_t offset);

} // namespace satellite

