#pragma once

// One line being typed: the bytes, where the cursor is in them, and browsing
// backwards through what was typed before (PLAN M0.6, ported from 003's
// satellite_prompt/editor, with the three paste keys added).
//
// IT DRAWS NOTHING AND READS NOTHING. Keys in, buffer out -- render.cpp draws
// and line_reader.cpp reads. That split is what makes the editor testable
// without a terminal: prompt_cases.cpp feeds it KeyEvents and asserts on the
// buffer, with no pty anywhere. The first satellite's line editor mixed the
// three and could only be tested by a human looking at a screen. AND IT KEEPS NO
// TERMINAL TYPE, so M0.7's window can drive the same editor with its own keys.
//
// THE CURSOR IS A BYTE OFFSET AND MOVES BY CHARACTERS. Every move goes through
// keys.hpp's previous_character / next_character, so a left arrow over a
// multi-byte character lands on its first byte rather than inside it -- which
// would split the character in two the next time anything inserted at it.

#include "keys.hpp"

#include <cstddef>
#include <string>

namespace satellite004::prompt {

class ConsoleLines;
class EarlierConsoles;

class Editor {
public:
    // UP AND DOWN GO THROUGH `lines` (KEY_PLAN.md KEY-4): the console's lines, from stop to stop
    // (console_lines.hpp). Null, they do nothing.
    // LEFT AND RIGHT GO ON PAST THE LINE'S ENDS TO `earlier` (KEY-6): the consoles before this one, newest
    // first (earlier_consoles.hpp). Null, Left and Right stop at the line's ends, as they always have.
    explicit Editor(const ConsoleLines *lines = nullptr, EarlierConsoles *earlier = nullptr);

    enum class Outcome {
        Continue,     // the line changed, or did not; either way keep reading
        Accept,       // Enter -- the line is finished
        Interrupt,    // Ctrl-C -- abandon this line
        EndOfInput,   // Ctrl-D on an EMPTY line -- end the session
        ClearScreen,  // Ctrl-L -- repaint
    };

    Outcome apply(const KeyEvent &event);

    const std::string &line() const { return buffer_; }
    std::size_t cursor() const { return cursor_; }

    void reset();
    void set_line(std::string text);

private:
    void recall(std::size_t index);
    void kill_word_back();
    // THE LINES OF CONSOLE `shown`: this console's for 0, the console `shown` back from it for the
    // rest -- read the first time Left reaches it. Null when there are none, or it was passed over.
    const ConsoleLines *console(std::size_t shown) const;
    // CONSOLE `shown`, AT ITS LAST LINE, the half-typed line kept first when this console is left.
    void show(std::size_t shown, const ConsoleLines &lines);
    // BACK AT THIS CONSOLE: the half-typed line returns, browsing ends.
    void back_to_this_console();

    std::string buffer_;
    std::size_t cursor_ = 0;

    // BROWSING KEEPS THE HALF-TYPED LINE, which is the part people notice when
    // it is missing: type three words, press Up to check something, press Down,
    // and the three words are still there. `live_` is where they wait, and
    // `browsing_` is what says whether there is anything in it -- an empty
    // `live_` is a legitimate saved line and cannot double as "not browsing".
    // `browse_` counts stops, never lines: the stop shown, or stops() when none is.
    //
    // AND WHICH CONSOLE IS SHOWN (KEY_PLAN.md KEY-6): `shown_` is 0 for this one, 1 for the console
    // before it, and so on back. Up and Down go through the stops of the console shown, and `browsing_`
    // stays true all the while another console is shown, so the half-typed line waits in `live_` until
    // Right or Down brings this console back.
    const ConsoleLines *lines_ = nullptr;
    EarlierConsoles *earlier_ = nullptr;
    std::size_t shown_ = 0;
    std::size_t browse_ = 0;
    std::string live_;
    bool browsing_ = false;
};

} // namespace satellite004::prompt
