#pragma once

#include <cstddef>
#include <string>

// Drawing one prompt line onto a terminal, including the case where it does
// not fit on one row.
//
// WRAPPING IS THE WHOLE OF THIS FILE. A line shorter than the terminal is two
// escape sequences and no state -- carriage return, write, erase to end of
// line. The moment prompt + line exceeds the width, the terminal has wrapped
// it across several rows and a carriage return only reaches the start of the
// LAST one, so redrawing needs to know how many rows the previous draw used
// and which of them the cursor was left on. That is the state below, and it is
// why this is an object rather than a function.
//
// The method is the one linenoise settled on and it is worth naming rather
// than rediscovering: walk down to the last row, clear each row on the way
// back up, redraw the whole thing from the top, then walk back down to the
// row the cursor belongs on. Clearing row by row rather than with a single
// erase-to-end-of-display is what keeps it from wiping whatever the terminal
// is showing below the prompt.

namespace satellite {

// The terminal's width in columns, from TIOCGWINSZ. Answers 80 when the
// terminal will not say, which is the conventional fallback and also the only
// number that cannot make the arithmetic below divide by zero.
int terminal_columns();

class Renderer {
public:
    // Draws `prompt` then `line`, and leaves the cursor `cursor` bytes into
    // the line. `cursor` is a byte offset on a character boundary, which is
    // what Editor guarantees.
    void draw(const std::string &prompt, const std::string &line,
              size_t cursor);

    // Forgets what is on screen. Called after the newline that ends a line,
    // and after Ctrl-L repaints -- both leave the terminal in a state this
    // object's row counts no longer describe, and drawing against a stale
    // count is what erases somebody's scrollback.
    void reset();

    // Clears the screen and puts the cursor at the top left. Ctrl-L, and the
    // one thing here that is not about the current line.
    void clear_screen();

private:
    // The most rows this line has EVER occupied, not the rows it occupies now.
    // A line that was three rows long and has just been cut back to one still
    // has two rows of stale text on screen to erase, and the current length
    // cannot say so.
    int max_rows_ = 1;

    // Which row the cursor was left on by the previous draw, counted from the
    // top of the line's first row starting at 1. Where the next draw has to
    // walk down from before it can start clearing.
    int cursor_row_ = 1;
};

} // namespace satellite
