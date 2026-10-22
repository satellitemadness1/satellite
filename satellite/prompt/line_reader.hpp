#pragma once
// Reading one line from the person at the keyboard, or from a pipe (PLAN M0.6,
// ported from 003's satellite_prompt/line_reader).
//
// WHERE THE PIECES MEET: raw_mode puts the terminal where bytes arrive one at a
// time, keys turns bytes into keys, editor turns keys into a line, and render
// draws it. None of the four knows the others except through this file.
//
// RAW MODE IS ENTERED PER LINE, NOT PER SESSION: the terminal is cooked again
// the moment a line is accepted, and stays cooked while it runs.
//
// WHAT CHANGED FROM 003:
//   - FROM A PIPE OR A FILE, NO PROMPT TEXT, AND NEVER std::cin (D0.6.2). When
//     stdin or stdout is not a terminal, lines are read with read(2) into this
//     reader's own buffer and split at '\n' only; a line keeps any '\r' and any
//     NUL it holds, and the last line needs no newline. A '\r' is the
//     statement reader's to judge, not this file's.
//   - BYTES ARE READ IN CHUNKS, NOT ONE read() A BYTE, and the bytes a line did
//     not use wait here for the next one. Keys typed before Enter are therefore
//     never lost.
//   - KEYS TYPED WHILE A LINE RUNS ARE THROWN AWAY (D0.6.5, 003's choice), but
//     READ AND DROPPED, NOT FLUSHED: when the next line starts and this reader
//     holds nothing of its own, every byte already waiting is read and dropped,
//     and a paste or key begun among them is read to its end (up to 100 ms of
//     quiet) and dropped whole. 003's TCSAFLUSH cut a paste at whatever the
//     kernel held at that instant. When the reader DOES hold bytes from before
//     the run -- typed ahead, or the start of a paste -- nothing is dropped,
//     because they and what followed them are one piece of input.
//   - A PASTE IS READ WHOLE BEFORE ANY LINE OF IT IS ANSWERED, and its lines are
//     answered one a read(), each drawn after the prompt as if typed. The text
//     after its last newline waits in the editor for the person to finish. A
//     pasted line runs like a typed one; nothing is kept as text and run again.
//     discard_pending() drops what is left (Ctrl-C during one of those lines).
//   - A DRAW WAITS UNTIL INPUT STOPS ARRIVING, and nothing is drawn inside a
//     paste. A chunk of more than a keystroke's bytes waits up to 20 ms for the
//     next before drawing, so a 10 MB line from a terminal without bracketed
//     paste is drawn when it pauses, not at every gap between its pieces. A
//     keystroke is drawn at once.
//   - SIGWINCH redraws the line at the new width (render.hpp).
//   - A STDIN LEFT NON-BLOCKING IS WAITED ON (poll), never taken as its end.
//
// THE PROMPT IS A FIXED TEXT. 003 took a function of the line, so a `}` could move
// its own line left; 004's prompt refuses blocks until M6, and gets it back then.
//
// EVERY LINE IT ANSWERS AT A TERMINAL IS KEPT (KEY_PLAN.md KEY-3): added to the console's
// lines it was handed (console_lines.hpp), then given to the console's write hook
// (machine/console_store.hpp). A piped line, a line stopped with Ctrl-C and the end of
// the input are not; a pasted line is kept when it is answered, never when it is queued.
// AND UP AND DOWN GO THROUGH THOSE SAME LINES (KEY-4), input()'s answers among them: the
// History the prompt kept of its own is gone, and the caller remembers nothing. LEFT AND RIGHT GO
// ON PAST THE LINE'S ENDS TO THE CONSOLES BEFORE THIS ONE (KEY-6), when the reader is handed them.

#include "console_lines.hpp"
#include "earlier_consoles.hpp"
#include "render.hpp"

#include <cstddef>
#include <deque>
#include <string>
#include <unistd.h>

namespace satellite004::prompt {

enum class LineStatus {
    Line,         // a line was typed, pasted or piped; it is in `line`
    EndOfFile,    // Ctrl-D on an empty line, the end of a pipe, or a closed terminal
    Interrupted,  // Ctrl-C while typing: the line is abandoned and `line` is empty
};

class LineReader {
public:
    explicit LineReader(int in = STDIN_FILENO, int out = STDOUT_FILENO);

    LineReader(const LineReader &) = delete;
    LineReader &operator=(const LineReader &) = delete;

    LineStatus read(const std::string &prompt, std::string &line);
    // THE SAME, WITH A PROMPT THAT HAS COLOUR (render.hpp's Prompt), or one written as given.
    LineStatus read(const Prompt &prompt, std::string &line);

    // WHERE EVERY LINE IT ANSWERS AT A TERMINAL IS KEPT (KEY_PLAN.md KEY-3), and what Up and
    // Down go through (KEY-4): the console's lines -- in satl the store's (session.cpp, and a
    // file run's reader in structured-library.cpp), in a harness its own. Null keeps nothing
    // here, the write hook, when one is set, keeps the line itself, and Up and Down do nothing.
    void keep_lines_in(ConsoleLines *lines) { lines_ = lines; }
    // WHERE LEFT GOES PAST THE START OF A LINE, AND RIGHT COMES BACK THROUGH (KEY_PLAN.md KEY-6): the
    // consoles before this one -- in satl the store's, found by its walk (KEY-5), in prompt_cases its
    // own. Null, as in prompt_reader, Left and Right stop at the line's ends.
    void left_reaches(EarlierConsoles *earlier) { earlier_ = earlier; }

    // Drop every line and byte this reader holds and has not answered: the rest
    // of a paste, keys typed ahead. The session calls it when Ctrl-C stops a line.
    void discard_pending();

    // Both ends are a terminal: lines are edited, and the prompt is drawn.
    bool interactive() const { return interactive_; }

    // THE NEXT LINE STARTS WITH THIS ALREADY WRITTEN, the cursor after it -- the indentation
    // of a block the prompt opened, or a section it typed for a spacesuit (the author,
    // 2026-09-25). Only at a terminal, and never over a paste's unfinished line.
    void preset(std::string text)
    {
        if (interactive_ && carried_.empty()) carried_ = std::move(text);
    }
    // WHETHER THE LAST LINE WAS TYPED AND ENTERED BY HAND -- not pasted, not piped. Only a
    // line typed by hand has its { written for it: a paste and a pipe bring their own.
    bool last_line_was_typed() const { return typed_; }

private:
    LineStatus read_piped(std::string &line);
    LineStatus read_typed(const Prompt &prompt, std::string &line);

    // read(2) once more onto the end of input_; false at the end of input.
    bool fill();
    // D0.6.5: read what was typed while the last line ran, and drop it.
    void drop_what_was_typed();

    int in_;
    int out_;
    bool interactive_;
    ConsoleLines *lines_ = nullptr;   // the console's lines, or null (keep_lines_in)
    EarlierConsoles *earlier_ = nullptr;   // the consoles before this one, or null (left_reaches)

    std::string input_;       // bytes read and not yet used, from `taken_` on
    std::size_t taken_ = 0;
    std::size_t scanned_ = 0; // a pipe: input_ holds no '\n' before this
    std::size_t last_read_ = 0; // bytes the last read(2) brought

    bool typed_ = false;              // the last line came from the keys and Enter
    std::deque<std::string> pasted_;  // whole lines a paste brought, not yet answered
    std::string carried_;             // the paste's unfinished last line
};

} // namespace satellite004::prompt
