#pragma once

// One line being typed: the bytes, where the cursor is in them, and browsing
// backwards through what was typed before.
//
// IT DRAWS NOTHING AND READS NOTHING. Keys in, buffer out -- render.cpp draws
// and line_reader.cpp reads. That split is what makes the editor testable
// without a terminal: tests/prompt_test feeds it KeyEvents and asserts on the
// buffer, with no pty anywhere. The first satellite's line editor mixed the
// three and could only be tested by a human looking at a screen.
//
// THE CURSOR IS A BYTE OFFSET AND MOVES BY CHARACTERS. Every move goes through
// keys.hpp's previous_character / next_character, so a left arrow over a
// multi-byte character lands on its first byte rather than inside it -- which
// would split the character in two the next time anything inserted at it.

#include "satellite_prompt/keys.hpp"

#include <cstddef>
#include <string>

namespace satellite::prompt {

class History;

class Editor {
public:
    explicit Editor(const History *history = nullptr);

    enum class Outcome {
        Continue,     // the line changed, or did not; either way keep reading
        Accept,       // Enter -- the line is finished
        Interrupt,    // Ctrl-C -- abandon this line
        EndOfInput,   // Ctrl-D on an EMPTY line -- end the session
        ClearScreen,  // Ctrl-L -- repaint
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

    // BROWSING KEEPS THE HALF-TYPED LINE, which is the part people notice when
    // it is missing: type three words, press Up to check something, press Down,
    // and the three words are still there. `live_` is where they wait, and
    // `browsing_` is what says whether there is anything in it -- an empty
    // `live_` is a legitimate saved line and cannot double as "not browsing".
    const History *history_ = nullptr;
    size_t browse_ = 0;
    std::string live_;
    bool browsing_ = false;
};

} // namespace satellite::prompt
