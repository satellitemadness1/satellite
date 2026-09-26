// Drawing a prompt line, wrapped or not. See render.hpp for the method and
// where it comes from.

#include "console_input/render.hpp"
#include "console_input/keys.hpp"

#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

namespace satellite {

namespace {

// One write(2) for the whole redraw. Building the escape sequences into a
// string and writing once is not an optimisation -- a redraw issued as eight
// separate writes is eight chances for the terminal to render a half-erased
// line, which is what flicker is.
void put(std::string &out, const char *text) { out += text; }

void put_number(std::string &out, const char *before, int value,
                const char *after)
{
    out += before;
    out += std::to_string(value);
    out += after;
}

void flush(const std::string &out)
{
    (void)!write(STDOUT_FILENO, out.data(), out.size());
}

} // namespace

int terminal_columns()
{
    struct winsize size;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
        return size.ws_col;
    return 80;
}

void Renderer::reset()
{
    max_rows_ = 1;
    cursor_row_ = 1;
}

void Renderer::clear_screen()
{
    std::string out;
    put(out, "\033[H\033[2J");
    flush(out);
    reset();
}

void Renderer::draw(const std::string &prompt, const std::string &line,
                    size_t cursor)
{
    const int columns = terminal_columns();
    const int prompt_cols = display_width(prompt);
    const int line_cols = display_width(line);
    const int cursor_cols =
        prompt_cols + display_width(line.substr(0, cursor));

    // Rows the text occupies now, and the row the cursor belongs on. Both
    // count from 1, which is what makes "walk up (rows - cursor_row)" read as
    // the number of rows it actually is.
    int rows = (prompt_cols + line_cols + columns - 1) / columns;
    if (rows < 1)
        rows = 1;
    if (rows > max_rows_)
        max_rows_ = rows;

    std::string out;

    // --- erase what is there ------------------------------------------------
    // Down to the last row that was ever used, then clear rows on the way back
    // up. max_rows_ and not rows, because a line that has just been shortened
    // still has its old tail on the screen.
    if (max_rows_ - cursor_row_ > 0)
        put_number(out, "\033[", max_rows_ - cursor_row_, "B");
    for (int i = 0; i < max_rows_ - 1; i++)
        put(out, "\r\033[0K\033[1A");
    put(out, "\r\033[0K");

    // --- write it again -----------------------------------------------------
    out += prompt;
    out += line;

    // THE DEFERRED WRAP, which is the one edge case that cannot be reasoned
    // about from the arithmetic alone. Writing into the last column of a row
    // does NOT move the cursor to the next row -- terminals leave it in a
    // phantom column and wrap only when the next character arrives. So a
    // cursor sitting exactly at the end of a line that exactly fills its rows
    // is displayed at the right edge of the row above where it belongs, and
    // the next character typed appears to jump. Emitting the newline ourselves
    // forces the wrap that has not happened yet.
    if (cursor > 0 && cursor == line.size() &&
        (prompt_cols + line_cols) % columns == 0) {
        put(out, "\n\r");
        rows++;
        if (rows > max_rows_)
            max_rows_ = rows;
    }

    // --- put the cursor where it belongs ------------------------------------
    // The write above left it at the end of the text, which is row `rows`.
    const int target_row = cursor_cols / columns + 1;
    if (rows - target_row > 0)
        put_number(out, "\033[", rows - target_row, "A");

    const int target_col = cursor_cols % columns;
    put(out, "\r");
    if (target_col > 0)
        put_number(out, "\033[", target_col, "C");

    cursor_row_ = target_row;
    flush(out);
}

} // namespace satellite
