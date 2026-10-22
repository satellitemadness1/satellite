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

namespace {

// WHERE A PROMPT AS GIVEN LEAVES THE CURSOR'S COLUMN (render.hpp's Prompt), written from column 0 on a
// terminal `width` columns wide. Its characters count as place() counts them, wrapping where a terminal
// wraps them; \r and \n go to the row's start, as the pty's ONLCR makes \n; a backspace goes one back,
// a tab to the next stop of eight; ESC [ ... H, f, G and ` go to the column they name, C and D along
// the row, E and F to its start; ESC 7 and ESC 8 save the column and put it back. EVERY OTHER CODE
// MOVES NOTHING -- a colour, a clear, a scroll region, a title -- and no byte of any code is a column.
// Only the column is wanted: the line starts on whichever row the prompt leaves the cursor on.
//
// A PROMPT THAT NEVER SAYS ITS COLUMN -- no \r, \n or ESC [ ... H in it, or no prompt at all -- is
// counted from column 0, where a line the program displayed before it leaves the cursor. One written
// after a display(..., end="") stands further right, and then only a line long enough to wrap is drawn
// in the wrong place: a draw that stays on one row moves by what the renderer drew, never by this.
std::size_t column_after(std::string_view given, std::size_t width)
{
    const auto at_most = [width](std::size_t column) { return column < width ? column : width - 1; };
    std::size_t column = 0, saved = 0;
    for (std::size_t i = 0; i < given.size();) {
        const unsigned char c = static_cast<unsigned char>(given[i]);
        if (c == 0x1B && i + 1 < given.size()) {
            const char kind = given[i + 1];
            std::size_t end = i + 2;
            if (kind == '[') {
                // ESC [, its parameters, then the one byte from @ to ~ that says what it does. A
                // private one (ESC [ ? 25 l, the cursor hidden) moves nothing.
                while (end < given.size() && !(given[end] >= 0x40 && given[end] <= 0x7E))
                    ++end;
                if (end == given.size())
                    break;
                std::size_t number[2] = {0, 0}, which = 0;
                for (std::size_t k = i + 2; k < end; ++k) {
                    if (given[k] == ';')
                        ++which;
                    else if (which < 2 && given[k] >= '0' && given[k] <= '9' && number[which] < 100000)
                        number[which] = number[which] * 10 + static_cast<std::size_t>(given[k] - '0');
                }
                const std::size_t first = number[0] > 0 ? number[0] : 1, second = number[1] > 0 ? number[1] : 1;
                const bool private_one = end > i + 2 && given[i + 2] >= '<' && given[i + 2] <= '?';
                if (!private_one) {
                    switch (given[end]) {
                    case 'H': case 'f': column = at_most(second - 1); break;
                    case 'G': case '`': column = at_most(first - 1); break;
                    case 'C': column = at_most(column + first); break;
                    case 'D': column = column > first ? column - first : 0; break;
                    case 'E': case 'F': column = 0; break;
                    default: break;
                    }
                }
                i = end + 1;
            } else if (kind == ']' || kind == 'P' || kind == 'X' || kind == '^' || kind == '_') {
                // A STRING -- a title, a colour asked for -- to BEL or ESC backslash.
                while (end < given.size() && given[end] != '\a' &&
                       !(given[end] == '\033' && end + 1 < given.size() && given[end + 1] == '\\'))
                    ++end;
                i = end >= given.size() ? end : end + (given[end] == '\a' ? 1 : 2);
            } else {
                // ESC, any bytes from space to /, then the one that ends it: ESC 7, ESC 8, ESC ( B.
                if (kind == '7')
                    saved = column;
                else if (kind == '8')
                    column = saved;
                end = i + 1;
                while (end < given.size() && given[end] >= 0x20 && given[end] <= 0x2F)
                    ++end;
                i = end < given.size() ? end + 1 : end;
            }
            continue;
        }
        if (c < 0x20 || c == 0x7F) {
            if (c == '\r' || c == '\n')
                column = 0;
            else if (c == '\b')   // from the last cell, when a full row holds its wrap
                column = at_most(column) > 0 ? at_most(column) - 1 : 0;
            else if (c == '\t')
                column = at_most((column / 8 + 1) * 8);
            ++i;
            continue;
        }
        std::size_t next = i + 1;
        while (next < given.size() && (static_cast<unsigned char>(given[next]) & 0xC0) == 0x80)
            ++next;
        column = place(given.substr(i, next - i), width, Spot{0, column}).column;
        i = next;
    }
    return column;
}

} // namespace

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
    cursor_column_ = 0;
    drawn_.clear();
    drawn_cursor_ = 0;
    below_ = false;
    resized_ = false;
    given_ = false;
    start_ = 0;
}

void Renderer::clear_screen()
{
    write_all(out_, "\033[H\033[2J");
    reset();
}

Renderer::Spots Renderer::layout(std::string_view text, std::size_t cursor) const
{
    Spots s;
    s.cursor = place(text.substr(0, cursor), width_, Spot{0, start_});
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
    cursor_column_ = s.cursor.column;
    rows_ = (s.end.row > s.cursor.row ? s.end.row : s.cursor.row) + 1;
}

void Renderer::draw(const Prompt &prompt, const std::string &line, std::size_t cursor)
{
    // A PROMPT AS GIVEN, AT ITS FIRST DRAW (render.hpp's Prompt): written as it was given, where the
    // cursor stands, and the line starts in the column it leaves -- with nothing of the line on the
    // screen yet, so nothing is cleared.
    std::string out;
    bool fresh = false;
    if (prompt.as_given && !given_) {
        out = prompt.drawn.empty() ? prompt.text : prompt.drawn;
        start_ = column_after(out, width_);
        given_ = fresh = true;
        rows_ = 1;
        cursor_row_ = 0;
        cursor_column_ = start_;
        drawn_.clear();
        drawn_cursor_ = 0;
        below_ = false;
    }

    // shown(line) is shown(before) + shown(after): the cursor is on a character's
    // first byte, and shown() decides each character from its own bytes. A prompt
    // as given is no part of what is counted.
    const std::string_view whole(line);
    std::string text = prompt.as_given ? std::string() : shown(prompt.text);
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

    if (appended) {
        out.assign(text, drawn_.size());
    } else if (fresh) {
        out += text;
    } else {
        if (prompt.as_given && rows_ == 1) {
            // ONE ROW, AFTER A PROMPT AS GIVEN: back along it to where the line starts, by what this
            // renderer drew before the cursor -- never into the prompt, whatever column it left.
            if (cursor_column_ > start_)
                move(out, cursor_column_ - start_, 'D');
        } else {
            // Down to the bottom of the last draw, then up clearing each row: going
            // down first is what makes this right when the cursor was left mid-line.
            if (rows_ - 1 > cursor_row_)
                move(out, rows_ - 1 - cursor_row_, 'B');
            for (std::size_t i = 1; i < rows_; ++i)
                out += "\r\033[0K\033[1A";
            out += '\r';
            if (start_ > 0)
                move(out, start_, 'C');
        }
        out += "\033[0K";
        // THE PROMPT DRESSED, THEN THE LINE: the same characters as `text`, so
        // every column counted above is still true (render.hpp's Prompt). A prompt
        // as given is on the screen already, and is not written again.
        if (prompt.as_given || prompt.drawn.empty())
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
        if (prompt.as_given && end_row == 0 && s.end.column < width_) {
            // ...AND BACK ALONG THE SAME ROW from the line's end to the cursor.
            if (s.end.column > s.cursor.column)
                move(out, s.end.column - s.cursor.column, 'D');
        } else {
            if (end_row > s.cursor.row)
                move(out, end_row - s.cursor.row, 'A');
            out += '\r';
            if (s.cursor.column > 0)
                move(out, s.cursor.column, 'C');
        }
    }

    rows_ = end_row + 1 > rows_ ? end_row + 1 : rows_;
    cursor_row_ = s.cursor.row;
    cursor_column_ = s.cursor.column;
    write_all(out_, out);
    drawn_ = std::move(text);
    drawn_cursor_ = at;
}

} // namespace satellite004::prompt
