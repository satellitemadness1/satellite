#pragma once

#include <string>

#include "console_input/history.hpp"

// Reading one line from a person, with the arrow keys working.
//
// This is console_output's other half and is named for it. The output side is
// a queue with a printer thread so a program's output appears while it runs;
// this side is a line editor so that what the user types can be corrected,
// moved through, and recalled. Both are about the terminal and neither is
// about the language -- no satellite program can reach anything in here.
//
// The module is four pieces and only ONE of them touches a file descriptor:
//
//     keys.*      bytes -> keys. A pure state machine.
//     editor.*    keys -> a line and a cursor. Pure.
//     history.*   the previous entries, and the file they live in.
//     render.*    drawing a line that may not fit on one row.
//     line_reader.cpp   the loop that reads, and the only tty in the module.
//
// That split is deliberate: an arrow key is three bytes that arrive one read()
// at a time, and the only way to test the decoding of it is to be able to hand
// it bytes from a test. console_input_test drives every piece above without a
// terminal existing.

namespace satellite {

enum class LineStatus {
    Line,         // a line was typed; `line` holds it, without its newline
    EndOfFile,    // Ctrl-D on an empty line, or stdin closed
    Interrupted,  // Ctrl-C: the line was abandoned, the session was not
};

class LineReader {
public:
    // Loads the history file, if there is one to load. See history.hpp for
    // where it lives and how $SATL_HISTORY switches it off.
    LineReader();

    // Saves the history. A failure to save is ignored on purpose: a history
    // file that cannot be written is a nuisance and never a reason to make a
    // session end badly.
    ~LineReader();

    LineReader(const LineReader &) = delete;
    LineReader &operator=(const LineReader &) = delete;

    // Reads one line. `prompt` is drawn by this call rather than by the
    // caller, because a redraw has to rewrite it -- a caller that printed its
    // own prompt would leave this unable to say how many columns it took, and
    // every cursor position after the first would be wrong.
    //
    // A prompt containing SGR colour escapes is measured correctly, because
    // the width is counted over the escapes rather than over the bytes.
    LineStatus read(const std::string &prompt, std::string &line);

    // Adds a line to the history. Separate from read() on purpose: the prompt
    // decides what is worth remembering -- an abandoned block's lines are, a
    // `:` debug command is -- and a reader that remembered everything it read
    // would take that decision away.
    void remember(const std::string &line);

    const History &history() const { return history_; }

private:
    LineStatus read_cooked(const std::string &prompt, std::string &line);

    History history_;
};

} // namespace satellite
