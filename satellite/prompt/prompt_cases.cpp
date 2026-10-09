// satellite/prompt with no terminal: bytes to keys, keys to a line, a line to
// the cells it takes, and the piped reader (PLAN M0.6). 003's prompt_test clauses
// for the decoder and the editor, and one clause a change 004 made.
//
//     make build/prompt_cases && build/prompt_cases    (check.sh runs it)
//
// check_prompt.py drives the same code at a real terminal.

#include "editor.hpp"
#include "history.hpp"
#include "keys.hpp"
#include "line_reader.hpp"
#include "render.hpp"

#include "../machine/shown.hpp"

#include <cstdio>
#include <pty.h>
#include <string>
#include <thread>
#include <unistd.h>

using namespace satellite004::prompt;

namespace {

int failed = 0;

void check(bool right, const std::string &what)
{
    failed += right ? 0 : 1;
    std::printf("%s %s\n", right ? "ok  " : "FAIL", what.c_str());
}

// Feed every byte, and answer the LAST key that came out.
KeyEvent feed_all(KeyDecoder &decoder, const std::string &bytes)
{
    KeyEvent last;
    for (const char c : bytes) {
        KeyEvent got = decoder.feed(static_cast<unsigned char>(c));
        if (got.key != Key::None)
            last = std::move(got);
    }
    return last;
}

KeyEvent key(Key k, std::string text = {})
{
    KeyEvent event;
    event.key = k;
    event.text = std::move(text);
    return event;
}

void type(Editor &editor, const std::string &text)
{
    for (const char c : text)
        editor.apply(key(Key::Char, std::string(1, c)));
}

void keys()
{
    KeyDecoder d;
    check(d.feed(0x1b).key == Key::None && d.feed('[').key == Key::None && d.feed('A').key == Key::Up,
          "ESC [ A fed a byte at a time is Up, and nothing before the A");
    check(feed_all(d, "\033[B").key == Key::Down && feed_all(d, "\033[C").key == Key::Right &&
              feed_all(d, "\033[D").key == Key::Left && feed_all(d, "\033OH").key == Key::Home &&
              feed_all(d, "\033[F").key == Key::End && feed_all(d, "\033[3~").key == Key::Delete &&
              feed_all(d, "\033[7~").key == Key::Home && feed_all(d, "\033[4~").key == Key::End,
          "the arrows, Home, End and Delete, in both spellings");
    check(feed_all(d, "\033[1;5C").key == Key::WordRight && feed_all(d, "\033[1;5D").key == Key::WordLeft &&
              feed_all(d, "\033b").key == Key::WordLeft && feed_all(d, "\033f").key == Key::WordRight,
          "Ctrl-arrows and Alt-b/Alt-f are word moves");
    check(feed_all(d, "\033[1;50C").key == Key::Right, "a modifier of 50 is not control's 5");
    check(d.feed(0x03).key == Key::Interrupt && d.feed(0x04).key == Key::EndOfInput &&
              d.feed(0x0c).key == Key::ClearScreen && d.feed(0x7f).key == Key::Backspace &&
              d.feed(0x08).key == Key::Backspace && d.feed('\r').key == Key::Enter && d.feed('\n').key == Key::Enter,
          "Ctrl-C, Ctrl-D, Ctrl-L, both backspaces, CR and LF");
    check(d.feed(0xC3).key == Key::None && feed_all(d, "\xA9").text == "\xC3\xA9", "e-acute is one Char of two bytes");
    d.feed(0xC3);
    check(feed_all(d, "x").text == "x", "a character cut short: the next byte is read as itself");
    check(feed_all(d, "\t").text == std::string(tab_width, ' '), "a tab arrives as spaces");

    // THE CHANGES FROM 003.
    check(feed_all(d, "\033\003").key == Key::Interrupt, "ESC then Ctrl-C: the Ctrl-C arrives (003 swallowed it)");
    check(feed_all(d, "\033[12\r").key == Key::Enter, "a sequence cut off by Enter: the Enter arrives");
    check(feed_all(d, "\033\xC3\xA9").text == "\xC3\xA9", "ESC then e-acute: the character arrives");
    check(feed_all(d, "\033[" + std::string(100000, '9') + "~").key == Key::Ignored && feed_all(d, "x").text == "x",
          "a 100,000-byte escape sequence is read to its end, and the next key is a key");
    check(feed_all(d, "\033[?1A").key == Key::Ignored, "a private marker is no key");

    check(feed_all(d, "\033[200~").key == Key::PasteStart, "ESC [ 200 ~ starts a paste");
    KeyEvent line = feed_all(d, "ab\tc\003\033x\r");
    check(line.key == Key::PasteLine && line.text == "ab    c\003\033x", "a pasted line is text, controls and all");
    check(d.feed('\n').key == Key::None, "a pasted CR LF is one newline");
    check(feed_all(d, "\n").key == Key::PasteLine, "a pasted LF alone ends a line");
    KeyEvent end = feed_all(d, "d\033[20x\033\033[201~");
    check(end.key == Key::PasteEnd && end.text == "d\033[20x\033", "text that begins like the end is text; the end ends it");
    check(d.feed(0x03).key == Key::Interrupt, "after the paste, Ctrl-C is a key again");
}

void editing()
{
    Editor e;
    type(e, "dislay");
    for (int i = 0; i < 3; ++i)
        e.apply(key(Key::Left));
    type(e, "p");
    check(e.line() == "display" && e.cursor() == 4, "an insert lands at the cursor");

    Editor b;
    type(b, "a");
    b.apply(key(Key::Char, "\xC3\xA9"));
    b.apply(key(Key::Backspace));
    check(b.line() == "a", "backspace over e-acute takes both of its bytes");

    Editor w;
    type(w, "satellite.console.display hello");
    w.apply(key(Key::WordLeft));
    const bool at_space = w.cursor() == 26;
    w.apply(key(Key::WordLeft));
    check(at_space && w.cursor() == 0, "word-left stops at spaces, not dots");

    Editor k;
    type(k, "one two three");
    k.apply(key(Key::KillWordBack));
    check(k.line() == "one two ", "Ctrl-W takes the last word");

    Editor o;
    check(o.apply(key(Key::Enter)) == Editor::Outcome::Accept &&
              o.apply(key(Key::Interrupt)) == Editor::Outcome::Interrupt &&
              o.apply(key(Key::EndOfInput)) == Editor::Outcome::EndOfInput,
          "Enter accepts, Ctrl-C interrupts, Ctrl-D on an empty line ends");
    type(o, "abc");
    o.apply(key(Key::Home));
    check(o.apply(key(Key::EndOfInput)) == Editor::Outcome::Continue && o.line() == "bc",
          "Ctrl-D on a line with text deletes forward");

    History history;
    history.add("first");
    history.add("second");
    Editor h(&history);
    type(h, "half typed");
    h.apply(key(Key::Up));
    h.apply(key(Key::Up));
    h.apply(key(Key::Up));
    const bool oldest = h.line() == "first";
    h.apply(key(Key::Down));
    h.apply(key(Key::Down));
    check(oldest && h.line() == "half typed", "Up to the oldest and Down past the newest gives back the half-typed line");

    Editor p;
    type(p, "ab");
    p.apply(key(Key::Left));
    check(p.apply(key(Key::PasteLine, "XY")) == Editor::Outcome::Accept && p.line() == "aXYb",
          "a pasted line is inserted at the cursor and accepts the line");
    check(p.apply(key(Key::PasteEnd, "Z")) == Editor::Outcome::Continue && p.line() == "aXYZb",
          "the paste's last text is inserted and waits");

    History many;
    many.add("same");
    many.add("same");
    const bool once = many.size() == 1;
    for (int i = 0; i < 100000; ++i)
        many.add("line " + std::to_string(i));
    check(once && many.size() == 100001 && many.at(1) == "line 0", "no line twice in a row, and no ceiling (003 kept 1,000)");
}

void cells()
{
    const auto spot = [](std::string_view text, std::size_t width, Spot from = {}) { return place(text, width, from); };
    const auto is = [](Spot s, std::size_t row, std::size_t column) { return s.row == row && s.column == column; };
    check(is(spot("abc", 10), 0, 3), "three ASCII characters are three cells");
    check(is(spot("abcdefghij", 10), 0, 10) && is(spot("k", 10, spot("abcdefghij", 10)), 1, 1),
          "a full row holds its wrap until the next character");
    check(is(spot("\xE6\x97\xA5\xE6\x9C\xAC", 10), 0, 4), "two CJK characters are four cells");
    check(is(spot("abcd\xE6\x97\xA5", 5), 1, 2), "a two-cell character that does not fit the last cell starts the next row");
    check(is(spot("e\xCC\x81", 10), 0, 1), "a combining mark takes no cell");
    check(is(spot("\xF0\x9F\x98\x80", 10), 0, 2), "an emoji is two cells");
    check(is(spot("\xE6\x97\xA5\xE6\x97\xA5", 1), 1, 2), "a one-column terminal does not loop on a two-cell character");

    // THE DRAW SHOWS CONTROLS ESCAPED: every ESC the renderer writes starts a CSI
    // of its own, and the pasted OSC is text.
    int ends[2];
    if (pipe(ends) != 0)
        return check(false, "a pipe for the renderer");
    Renderer renderer(ends[1]);
    renderer.draw("\033]0;p\007> ", "x\033]2;title\007\xFFy", 3);
    close(ends[1]);
    std::string drawn;
    char buffer[4096];
    for (ssize_t got; (got = read(ends[0], buffer, sizeof buffer)) > 0;)
        drawn.append(buffer, static_cast<std::size_t>(got));
    close(ends[0]);
    bool only_csi = true;
    for (std::size_t i = drawn.find('\033'); i != std::string::npos; i = drawn.find('\033', i + 1))
        only_csi = only_csi && i + 1 < drawn.size() && drawn[i + 1] == '[';
    check(drawn.find("\\x1b]0;p\\x07> x\\x1b]2;title\\x07\\xffy") != std::string::npos && only_csi &&
              drawn.find('\007') == std::string::npos,
          "an OSC, a BEL and a bad byte, in the prompt or the line, are drawn as \\xNN: only CSIs reach the terminal");
}

void piped()
{
    const auto file_of = [](const std::string &bytes) {
        std::FILE *file = std::tmpfile();
        std::fwrite(bytes.data(), 1, bytes.size(), file);
        std::fflush(file);
        lseek(fileno(file), 0, SEEK_SET);
        return file;
    };
    std::FILE *in = file_of(std::string("a\nb\0c\r\n\nlast", 12));
    std::FILE *out = std::tmpfile();
    LineReader reader(fileno(in), fileno(out));
    std::string line, got;
    while (reader.read("satl> ", line) == LineStatus::Line)
        got += "[" + line + "]";
    check(!reader.interactive() && got == std::string("[a][b\0c\r][][last]", 17) &&
              reader.read("satl> ", line) == LineStatus::EndOfFile,
          "a file's lines: a NUL and a CR kept, an empty line, a last line with no newline, then the end again");
    check(lseek(fileno(out), 0, SEEK_END) == 0, "no prompt text is written when the input is not a terminal");
    std::fclose(in);

    std::string big(10000000, 'x');
    big += "\ny\n";
    for (int i = 0; i < 100000; ++i)
        big += std::to_string(i) + "\n";
    in = file_of(big);
    LineReader many(fileno(in), fileno(out));
    const bool ten_mb = many.read("", line) == LineStatus::Line && line.size() == 10000000;
    const bool y = many.read("", line) == LineStatus::Line && line == "y";
    int count = 0;
    while (many.read("", line) == LineStatus::Line && line == std::to_string(count))
        ++count;
    check(ten_mb && y && count == 100000, "a 10 MB line, then 100,000 short lines, each whole and in order");
    std::fclose(in);
    std::fclose(out);

    // A TERMINAL THAT HANGS UP AFTER A LINE: raw mode can no longer be entered, so
    // the reader falls back to reading as from a pipe, and the bytes it had already
    // read after that line come out once each (the review of the port: they came
    // out as "xy\ncd\n", then "xy", then "cd").
    int master = -1, slave = -1;
    if (openpty(&master, &slave, nullptr, nullptr, nullptr) != 0)
        return check(false, "a pty for the hang-up case");
    LineReader typed(slave, slave);
    std::thread typist([master] {
        usleep(200000);  // after the reader waits: keys already there when it starts are dropped (D0.6.5)
        (void)!write(master, "wait\nxy\ncd\n", 11);
    });
    std::string lines;
    if (typed.read("> ", line) == LineStatus::Line)
        lines += "[" + line + "]";
    typist.join();
    close(master);  // what the reader drew is a few bytes, which the pty holds unread
    for (int more = 0; more < 4 && typed.read("> ", line) == LineStatus::Line; ++more)
        lines += "[" + line + "]";
    close(slave);
    check(lines == "[wait][xy][cd]", "a terminal that hangs up after a line: the rest of what was typed, once each (" + satellite004::shown(lines) + ")");
}

} // namespace

int main()
{
    keys();
    editing();
    cells();
    piped();
    std::printf("%s\n", failed == 0 ? "every prompt case passed" : "a prompt case FAILED");
    return failed == 0 ? 0 : 1;
}
