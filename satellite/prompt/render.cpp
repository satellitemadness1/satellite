// Drawing the prompt line. See render.hpp.

#include "render.hpp"

#include "../machine/shown.hpp"

#include <atomic>
#include <cerrno>
#include <clocale>
#include <csignal>
#include <cwchar>
#include <locale.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace satellite004::prompt {

namespace {

std::atomic<bool> resized{false};
std::atomic<bool> watching{false};

void on_resize(int)
{
    resized.store(true, std::memory_order_relaxed);
}

// 80 WHEN THE TERMINAL WILL NOT SAY, which a pty with no size set does not.
std::size_t ask_width(int out)
{
    winsize size{};
    if (ioctl(out, TIOCGWINSZ, &size) == 0 && size.ws_col > 0)
        return size.ws_col;
    return 80;
}

// THE UTF-8 CTYPE, MADE ONCE AND SWITCHED TO PER CALL ON THIS THREAD ONLY
// (uselocale), so satl's own locale is never touched. Null where the system has
// no C.UTF-8; every character is then one cell, 003's counting.
locale_t utf8_ctype()
{
    static const locale_t made = newlocale(LC_CTYPE_MASK, "C.UTF-8", static_cast<locale_t>(0));
    return made;
}

// One character of valid UTF-8 at `i`: its code, and `i` moved past it.
char32_t decode(std::string_view text, std::size_t &i)
{
    const unsigned char lead = static_cast<unsigned char>(text[i]);
    const std::size_t length = lead < 0x80 ? 1 : lead < 0xE0 ? 2 : lead < 0xF0 ? 3 : 4;
    char32_t code = length == 1 ? lead : length == 2 ? lead & 0x1F : length == 3 ? lead & 0x0F : lead & 0x07;
    for (std::size_t k = 1; k < length && i + k < text.size(); ++k)
        code = (code << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3F);
    i += length;
    return code;
}

int cells_of(char32_t code, bool wide_aware)
{
    if (code < 0x80 || !wide_aware)
        return 1;
    const int cells = wcwidth(static_cast<wchar_t>(code));
    return cells < 0 ? 1 : cells;
}

void move(std::string &out, std::size_t count, char direction)
{
    out += "\033[";
    out += std::to_string(count);
    out += direction;
}

} // namespace

void write_all(int out, std::string_view text)
{
    while (!text.empty()) {
        const ssize_t wrote = write(out, text.data(), text.size());
        if (wrote < 0 && errno == EINTR)
            continue;
        if (wrote <= 0)
            return;
        text.remove_prefix(static_cast<std::size_t>(wrote));
    }
}

Spot place(std::string_view text, std::size_t width, Spot from)
{
    const locale_t utf8 = utf8_ctype();
    const bool wide_aware = utf8 != static_cast<locale_t>(0);
    const locale_t was = wide_aware ? uselocale(utf8) : static_cast<locale_t>(0);
    for (std::size_t i = 0; i < text.size();) {
        const std::size_t cells = static_cast<std::size_t>(cells_of(decode(text, i), wide_aware));
        if (cells == 0)
            continue;  // a combining mark sits on the character before it
        if (from.column != 0 && from.column + cells > width) {
            ++from.row;
            from.column = 0;
        }
        from.column += cells;
    }
    if (wide_aware)
        uselocale(was);
    return from;
}

void watch_resizes()
{
    if (watching.exchange(true))
        return;
    struct sigaction action{};
    action.sa_handler = on_resize;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGWINCH, &action, nullptr);
}

bool take_resize()
{
    return resized.exchange(false, std::memory_order_relaxed);
}

Renderer::Renderer(int out) : out_(out), width_(ask_width(out)) {}

void Renderer::reset()
{
    rows_ = 1;
    cursor_row_ = 0;
    drawn_.clear();
    drawn_cursor_ = 0;
    below_ = false;
    resized_ = false;
}

void Renderer::clear_screen()
{
    write_all(out_, "\033[H\033[2J");
    reset();
}

Renderer::Spots Renderer::layout(std::string_view text, std::size_t cursor) const
{
    Spots s;
    s.cursor = place(text.substr(0, cursor), width_, Spot{});
    s.end = place(text.substr(cursor), width_, s.cursor);

    // THE CURSOR STANDS WHERE THE NEXT CHARACTER WILL BE DRAWN: past a full row,
    // or before a two-cell character that does not fit this one, that is the
    // start of the next row.
    std::size_t next_cells = 1;
    if (cursor < text.size()) {
        std::size_t i = cursor;
        decode(text, i);
        const std::size_t cells = place(text.substr(cursor, i - cursor), width_, Spot{}).column;
        next_cells = cells == 0 ? 1 : cells;
    }
    if (s.cursor.column >= width_ || (s.cursor.column != 0 && s.cursor.column + next_cells > width_)) {
        ++s.cursor.row;
        s.cursor.column = 0;
    }
    return s;
}

void Renderer::resize()
{
    width_ = ask_width(out_);
    resized_ = true;
    if (rows_ == 1 && drawn_.empty())
        return;  // nothing is on the screen to count
    const Spots s = layout(drawn_, drawn_cursor_);
    cursor_row_ = s.cursor.row;
    rows_ = (s.end.row > s.cursor.row ? s.end.row : s.cursor.row) + 1;
}

void Renderer::draw(const Prompt &prompt, const std::string &line, std::size_t cursor)
{
    // shown(line) is shown(before) + shown(after): the cursor is on a character's
    // first byte, and shown() decides each character from its own bytes.
    const std::string_view whole(line);
    std::string text = shown(prompt.text);
    const std::size_t prompt_ends = text.size();
    text += shown(whole.substr(0, cursor));
    const std::size_t at = text.size();
    text += shown(whole.substr(cursor));
    const Spots s = layout(text, at);

    // ONLY ADDED AT THE END, with the cursor at the end before and after: the
    // addition is written from where the cursor stands. After a held wrap the
    // terminal already stands on the next row, which is right for a character
    // with a cell and wrong for a combining mark, so that one redraws.
    bool appended = !resized_ && !drawn_.empty() && drawn_cursor_ == drawn_.size() && at == text.size() &&
                    text.size() > drawn_.size() && text.compare(0, drawn_.size(), drawn_) == 0;
    if (appended && below_) {
        std::size_t i = drawn_.size();
        decode(text, i);
        appended = place(std::string_view(text).substr(drawn_.size(), i - drawn_.size()), width_, Spot{}).column > 0;
    }
    resized_ = false;

    std::string out;
    if (appended) {
        out.assign(text, drawn_.size());
    } else {
        // Down to the bottom of the last draw, then up clearing each row: going
        // down first is what makes this right when the cursor was left mid-line.
        if (rows_ - 1 > cursor_row_)
            move(out, rows_ - 1 - cursor_row_, 'B');
        for (std::size_t i = 1; i < rows_; ++i)
            out += "\r\033[0K\033[1A";
        out += "\r\033[0K";
        // THE PROMPT DRESSED, THEN THE LINE: the same characters as `text`, so
        // every column counted above is still true (render.hpp's Prompt).
        if (prompt.drawn.empty())
            out += text;
        else
            out.append(prompt.drawn).append(text, prompt_ends, std::string::npos);
    }

    // THE HELD WRAP. A terminal that has just filled its last column does not
    // move to the next row until another character comes. When the cursor
    // belongs there, the newline is written here, so the terminal is where the
    // arithmetic says it is.
    std::size_t end_row = s.end.row;
    below_ = s.cursor.row > end_row;
    if (below_) {
        out += "\r\n";
        end_row = s.cursor.row;
    }
    if (!appended) {
        if (end_row > s.cursor.row)
            move(out, end_row - s.cursor.row, 'A');
        out += '\r';
        if (s.cursor.column > 0)
            move(out, s.cursor.column, 'C');
    }

    rows_ = end_row + 1 > rows_ ? end_row + 1 : rows_;
    cursor_row_ = s.cursor.row;
    write_all(out_, out);
    drawn_ = std::move(text);
    drawn_cursor_ = at;
}

} // namespace satellite004::prompt
