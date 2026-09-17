#pragma once
// Drawing one prompt line, wrapped across as many rows as it takes (PLAN M0.6,
// ported from 003's satellite_prompt/render).
//
// NOTHING REACHES THE TERMINAL RAW (D0.6.3). The prompt text and the line are
// drawn through shown() (machine/shown.hpp): a control byte, ESC, or a byte that
// is not UTF-8 is drawn as \xNN, so a pasted ESC ] 2 ; ... cannot retitle the
// window. Every column is counted on that shown form.
//
// A CHARACTER'S COLUMNS COME FROM wcwidth, under C.UTF-8 whatever satl's own
// locale is: CJK and most emoji take two cells, a combining mark none. 003
// counted one a character, so CJK wrapped the line in the wrong place. A
// two-cell character that does not fit the last cell of a row starts the next
// row, which is where a terminal puts it.
//
// THE WIDTH IS ASKED AGAIN WHEN THE WINDOW SAYS IT CHANGED (SIGWINCH). The handler
// only sets a flag, and it is installed with SA_RESTART, so a resize never makes
// a running program's write() fail. The reader waits in poll(), which a signal
// interrupts whatever SA_RESTART says, and calls resize().
//
// A RESIZE COUNTS WHAT THE LAST DRAW LEFT ON THE SCREEN, never the line as it is
// now: input that arrived faster than it was drawn was never on the screen (the
// review of the port: counting it erased five rows of output above the prompt).
// AND IT ASSUMES THE TERMINAL REFLOWS THE LINE, as VTE (satl-term, GNOME
// Terminal), Konsole and kitty do: the rows to clear are counted at the NEW
// width. On a terminal that does not reflow (xterm), a narrower window can clear
// a row above the prompt, and a wider one can leave one behind.
//
// EVERY WRITE HAS A LENGTH AND IS FINISHED. 003 wrote a draw with one write()
// and dropped what the terminal did not take; a long line reaches a pty in parts.
//
// A DRAW THAT ONLY ADDS TO THE END WRITES ONLY WHAT IT ADDS, when the cursor was
// and is at the end: typing, and any stream of input. Anything else redraws the
// line whole, so a line of 800 KB arriving in pieces is written once, not once a
// piece (the review of the port measured 25 MB written for it).
//
// WHAT IT REMEMBERS BETWEEN DRAWS. A line wrapped onto three rows cannot be
// erased with \r and a clear to end of line, which clears one. So the renderer
// keeps the tallest the line has been and the row the cursor was left on, walks
// down to the bottom, clears each row going up, and redraws.
//
// THE FIRST DRAW CLEARS THE ROW IT STARTS ON, so whatever printed before the
// prompt must end with a newline, or its last line is erased.

#include <cstddef>
#include <string>
#include <string_view>

namespace satellite004::prompt {

// write(2) until all of `text` is written, or the terminal refuses it.
void write_all(int out, std::string_view text);

// A place on the screen, counted from the row the prompt starts on. column ==
// width means the row is full and the terminal holds its wrap until the next
// character is written.
struct Spot {
    std::size_t row = 0;
    std::size_t column = 0;
};

// Where `text` -- valid UTF-8 with no control characters, as shown() makes it --
// leaves the cursor, written from `from` on a terminal `width` columns wide.
Spot place(std::string_view text, std::size_t width, Spot from);

// SIGWINCH sets a flag; take_resize() answers it and clears it.
void watch_resizes();
bool take_resize();

class Renderer {
public:
    explicit Renderer(int out);

    // Draw `prompt` then `line`, and leave the cursor at `cursor`, a byte offset
    // into `line` that is always on a character's first byte (editor.hpp).
    void draw(const std::string &prompt, const std::string &line, std::size_t cursor);

    // The window changed size: ask the width again, and count what the last draw
    // left on the screen at it, before the next draw.
    void resize();

    // Forget the rows: a line was accepted or abandoned, and the next one starts
    // below whatever was printed.
    void reset();

    // Ctrl-L: clear, home, and forget the rows.
    void clear_screen();

    std::size_t width() const { return width_; }

    // The last draw left the cursor at the start of the row BELOW its text: the
    // line filled its last row exactly, and the cursor was at its end. A newline
    // written now would leave a blank row.
    bool below_the_text() const { return below_; }

private:
    struct Spots {
        Spot cursor;
        Spot end;
    };
    // Where the cursor stands, `cursor` bytes into shown text, and where the text ends.
    Spots layout(std::string_view text, std::size_t cursor) const;

    int out_;
    std::size_t width_;
    std::size_t rows_ = 1;
    std::size_t cursor_row_ = 0;
    std::string drawn_;              // the last draw's text, shown: what is on the screen
    std::size_t drawn_cursor_ = 0;
    bool below_ = false;
    bool resized_ = false;           // the next draw redraws whole
};

} // namespace satellite004::prompt
