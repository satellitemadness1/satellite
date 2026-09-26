#pragma once

#include <cstddef>
#include <string>

#include "console_input/keys.hpp"

// One line being typed: the buffer, the cursor, and where the up arrow has
// walked to in the history.
//
// It has NO file descriptor and no terminal in it. Everything a keystroke does
// to a line happens here as a function of (state, key), so the whole of the
// editing -- including every history-browsing rule below, which is the part
// that is easy to get subtly wrong -- is testable by feeding it keys from a
// test rather than by typing into a pty. line_reader.cpp is what is left after
// this and keys.cpp have taken everything that does not need a terminal.

namespace satellite {

class History;

class Editor {
public:
    // `history` may be null, which is a session that remembers nothing; Up and
    // Down are then no-ops rather than a special case anywhere else.
    explicit Editor(const History *history = nullptr);

    // What the caller has to do about the key it just fed in.
    enum class Outcome {
        Continue,     // the line changed, or did not; redraw and read on
        Accept,       // Enter: the line is finished
        Interrupt,    // Ctrl-C: abandon this line
        EndOfInput,   // Ctrl-D on an EMPTY line: there is nobody there
        ClearScreen,  // Ctrl-L: the caller repaints the screen, then redraws
    };

    Outcome apply(const KeyEvent &event);

    const std::string &line() const { return buffer_; }
    size_t cursor() const { return cursor_; }

    // Starts a fresh line, and puts history browsing back at the live end.
    // Called between lines, so a Up-arrow on the next line starts from the
    // newest entry again rather than from wherever the last one left off.
    void reset();

    // For a caller that wants to seed the line -- a test, and nothing else so
    // far. The cursor lands at the end, which is where every history recall
    // puts it too.
    void set_line(std::string text);

private:
    void recall(size_t index);
    void kill_word_back();

    std::string buffer_;
    size_t cursor_ = 0;

    const History *history_ = nullptr;

    // Where Up has walked to, counted the way readline counts it: equal to the
    // history's size means "not browsing -- this is the live line", and every
    // smaller value indexes an entry. That is what makes Down able to come
    // back to a line the user had half typed before they pressed Up.
    size_t browse_ = 0;

    // The half-typed line, saved the FIRST time Up leaves it. Without this,
    // pressing Up and then Down gives back an empty line and the work is gone
    // with no way to say it was there -- which is the one history bug every
    // reader notices and nobody can describe.
    std::string live_;
    bool browsing_ = false;
};

} // namespace satellite
