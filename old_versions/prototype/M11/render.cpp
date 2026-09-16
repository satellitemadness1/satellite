// Prompt rendering implementation with multi-row line wrapping.
// Milestone 11 Prototype in prototype/M11.

#include "render.hpp"
#include "keys.hpp"

#include <sys/ioctl.h>
#include <unistd.h>

namespace satellite {

namespace {

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

    int rows = (prompt_cols + line_cols + columns - 1) / columns;
    if (rows < 1)
        rows = 1;
    if (rows > max_rows_)
        max_rows_ = rows;

    std::string out;

    // Erase old tail
    if (max_rows_ - cursor_row_ > 0)
        put_number(out, "\033[", max_rows_ - cursor_row_, "B");
    for (int i = 0; i < max_rows_ - 1; i++)
        put(out, "\r\033[0K\033[1A");
    put(out, "\r\033[0K");

    // Redraw prompt and line
    out += prompt;
    out += line;

    // Deferred wrap handling
    if (cursor > 0 && cursor == line.size() &&
        (prompt_cols + line_cols) % columns == 0) {
        put(out, "\n\r");
        rows++;
        if (rows > max_rows_)
            max_rows_ = rows;
    }

    // Reposition cursor
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

