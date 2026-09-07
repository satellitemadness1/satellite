#pragma once

// Reading one line from the person at the keyboard.
//
// THIS IS WHERE THE THREE PIECES MEET: raw_mode puts the terminal into a state
// where bytes arrive one at a time, keys turns those bytes into keys, editor
// turns keys into a line, and render draws it. None of those four knows about
// any of the others except through this file.
//
// AND IT READS COOKED WHEN THERE IS NO TERMINAL, which is not a fallback so much
// as the other half of the job. `echo 'satellite.help' | satl --repl` and
// `satl --repl < script.satl` are how the prompt gets TESTED without a human --
// tests/prompt_test does exactly that -- and a reader that refused a pipe would
// make the prompt the one part of this tree with no automated coverage. In that
// mode there is no editing, no history browsing and no redraw, because there is
// nobody to see them; a line is bytes up to a newline, exactly as std::getline
// gives them.
//
// RAW MODE IS ENTERED PER LINE AND NOT PER SESSION. The RawMode object lives
// inside read(), so the terminal is cooked again the moment a line is accepted
// and stays cooked for as long as the line takes to run. A satellite program
// that asks for input (M14's `satellite.console.input`) therefore finds the
// terminal in the state every other run of it finds -- the prompt does not leak
// its mode into the program it launches.

#include "satellite_prompt/history.hpp"

#include <functional>
#include <string>

namespace satellite::prompt {

enum class LineStatus {
    Line,        // a line was typed; it is in `line`
    EndOfFile,   // Ctrl-D on an empty line, or the end of a pipe
    Interrupted, // Ctrl-C -- the line is abandoned and `line` is empty
};

class LineReader {
public:
    LineReader() = default;

    LineReader(const LineReader &) = delete;
    LineReader &operator=(const LineReader &) = delete;

    // THE PROMPT IS A FUNCTION OF WHAT HAS BEEN TYPED, and that is what lets
    // the indent change UNDER the cursor. A `}` closes a block, so the line it
    // is on belongs one level out -- and the user types it as the first
    // character of an already-indented line. Recomputing the prompt on every
    // redraw is what moves that line left the instant the brace appears, which
    // is the whole of "the right bracket goes on a line tabbed only as far as
    // it needs to be". A fixed string cannot do it: the prompt is printed
    // before the character that changes it exists.
    using PromptFor = std::function<std::string(const std::string &line)>;

    LineStatus read(const PromptFor &prompt, std::string &line);

    // The fixed-prompt case, which is every caller that has no block open.
    LineStatus read(const std::string &prompt, std::string &line);

    // Add an accepted line to the history. SEPARATE FROM read() ON PURPOSE:
    // the caller decides what is worth remembering, and prompt.cpp does not
    // remember a line that was only a blank Enter.
    void remember(const std::string &line);

    const History &history() const { return history_; }

private:
    LineStatus read_cooked(const std::string &prompt, std::string &line);

    History history_;
};

} // namespace satellite::prompt
