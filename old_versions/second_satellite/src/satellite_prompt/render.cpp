// Drawing the prompt line. See satellite_prompt/render.hpp.

#include "satellite_prompt/render.hpp"

#include "satellite_console/console.hpp"
#include "satellite_prompt/keys.hpp"

#include <unistd.h>

namespace satellite::prompt {

namespace {

void escape(std::string &out, const char *before, int value, const char *after)
{
    out += before;
    out += std::to_string(value);
    out += after;
}

// ONE write() PER DRAW, AND THAT IS THE WHOLE REASON THE STRING IS BUILT FIRST.
// A draw is a dozen escape sequences; issuing them as a dozen writes lets a
// terminal render a half-erased line, which is visible as flicker on every
// keystroke. Built into one buffer, it is one atomic-enough write.
//
// THE RETURN IS DISCARDED DELIBERATELY. There is no recovery from a failed write
// to a terminal that is being drawn on -- the next keystroke redraws the whole
// line anyway -- and the alternative, refusing to read further input because one
// escape sequence was short-written, would turn a cosmetic glitch into a hang.
void emit(const std::string &out)
{
    const ssize_t written = write(STDOUT_FILENO, out.data(), out.size());
    (void)written;
}

int columns()
{
    return console::Console::the().width();
}

} // namespace

void Renderer::reset()
{
    max_rows_ = 1;
    cursor_row_ = 1;
}

void Renderer::clear_screen()
{
    std::string out = "\033[H\033[2J";
    emit(out);
    reset();
}

void Renderer::draw(const std::string &prompt, const std::string &line,
                    size_t cursor)
{
    const int width = columns();
    const int prompt_columns = display_width(prompt);
    const int line_columns = display_width(line);
    const int cursor_columns =
        prompt_columns + display_width(line.substr(0, cursor));

    int rows = (prompt_columns + line_columns + width - 1) / width;
    if (rows < 1)
        rows = 1;
    if (rows > max_rows_)
        max_rows_ = rows;

    std::string out;

    // Down to the bottom of what was drawn last time, then up clearing each
    // row. Going down first is what makes this correct when the cursor was
    // left in the MIDDLE of a wrapped line.
    if (max_rows_ - cursor_row_ > 0)
        escape(out, "\033[", max_rows_ - cursor_row_, "B");
    for (int i = 0; i < max_rows_ - 1; i++)
        out += "\r\033[0K\033[1A";
    out += "\r\033[0K";

    out += prompt;
    out += line;

    // THE DEFERRED WRAP, which is the one piece of this that looks like
    // superstition and is not. A terminal that has just filled its last column
    // does NOT move to the next row: it leaves the cursor on the last column
    // with a "pending wrap" flag, so that a newline arriving next does not
    // produce a blank row. The consequence here is that a line ending exactly
    // at the right margin reports a cursor position one row too high, and the
    // next draw erases the wrong row. Emitting the newline ourselves resolves
    // the flag and puts the cursor where the arithmetic below assumes it is.
    if (cursor > 0 && cursor == line.size() &&
        (prompt_columns + line_columns) % width == 0) {
        out += "\n\r";
        rows++;
        if (rows > max_rows_)
            max_rows_ = rows;
    }

    const int target_row = cursor_columns / width + 1;
    if (rows - target_row > 0)
        escape(out, "\033[", rows - target_row, "A");

    const int target_column = cursor_columns % width;
    out += "\r";
    if (target_column > 0)
        escape(out, "\033[", target_column, "C");

    cursor_row_ = target_row;
    emit(out);
}

} // namespace satellite::prompt
