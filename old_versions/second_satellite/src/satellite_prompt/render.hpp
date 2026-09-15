#pragma once

// Drawing one prompt line, wrapped across as many terminal rows as it takes.
//
// IT WRITES TO THE DESCRIPTOR DIRECTLY AND NOT THROUGH THE CONSOLE, and that is
// a boundary rather than a shortcut. The Console (M10) owns a printer THREAD and
// a queue, which is exactly right for a program's output -- lines are whole, and
// two threads cannot tear each other's -- and exactly wrong for a cursor: a
// keystroke whose echo waits in a queue is a terminal that feels broken. So the
// prompt writes synchronously, and prompt.cpp drains the Console before drawing
// so that the two never hold the descriptor at the same moment.
//
// THE WIDTH COMES FROM THE CONSOLE, WHICH IS PLAN M22's INSTRUCTION IN AS MANY
// WORDS: "M22's prompt consumes these from here rather than reimplementing them"
// (satellite_console/console.hpp). M14 already owns the ioctl and the 80-column
// fallback, and asks fresh every time so a resize mid-session is seen.
//
// WHAT IT REMEMBERS BETWEEN DRAWS, and why it has to. A line that wraps onto
// three rows cannot be erased by `\r` and a clear-to-end-of-line -- that clears
// one row and leaves two. So the renderer keeps the tallest the line has been
// and where the cursor was left, walks back up that many rows clearing each, and
// redraws. Without `max_rows_` a line that shrinks leaves its old tail on screen.

#include <cstddef>
#include <string>

namespace satellite::prompt {

class Renderer {
public:
    // Draw `prompt` then `line`, and leave the terminal cursor at `cursor`
    // (a byte offset into `line`).
    void draw(const std::string &prompt, const std::string &line, size_t cursor);

    // Forget the row bookkeeping. Called when a line is accepted or abandoned,
    // because the next line starts fresh below whatever was printed.
    void reset();

    // Ctrl-L. Clears and homes, then resets the bookkeeping to match.
    void clear_screen();

private:
    int max_rows_ = 1;
    int cursor_row_ = 1;
};

} // namespace satellite::prompt
