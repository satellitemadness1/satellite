// satellite/prompt with no terminal: bytes to keys, keys to a line, a line to
// the cells it takes, and the piped reader (PLAN M0.6). 003's prompt_test clauses
// for the decoder and the editor, and one clause a change 004 made. And the lines
// a reader keeps (KEY_PLAN.md KEY-3), handed lines of its own as satl hands it the
// console store's. And Left and Right past a line's ends, through the consoles
// before this one (KEY-6), filled here by hand as the store fills them from its walk.
//
//     make build/prompt_cases && build/prompt_cases    (check.sh runs it)
//
// check_prompt.py drives the same code at a real terminal.

#include "console_lines.hpp"
#include "earlier_consoles.hpp"
#include "editor.hpp"
#include "keys.hpp"
#include "line_reader.hpp"
#include "render.hpp"

#include "../machine/console_store.hpp"
#include "../machine/shown.hpp"

#include <cstdio>
#include <pty.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

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

    // UP AND DOWN GO THROUGH THE CONSOLE'S LINES (KEY_PLAN.md KEY-4), as they went through History.
    ConsoleLines lines;
    lines.add("first");
    lines.add("second");
    Editor h(&lines);
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

    // TURNED ROUND BY KEY-4: every Enter is a line now, a repeat too, and it is Up that shows it once.
    ConsoleLines many;
    many.add("same");
    many.add("same");
    Editor twice(&many);
    type(twice, "x");
    twice.apply(key(Key::Up));
    twice.apply(key(Key::Up));
    const bool shown_once = twice.line() == "same";
    twice.apply(key(Key::Down));
    const bool once = many.size() == 2 && shown_once && twice.line() == "x";
    for (int i = 0; i < 100000; ++i)
        many.add("line " + std::to_string(i));
    check(once && many.size() == 100002 && many.stops() == 100001 && many.stop_at(1) == "line 0",
          "two Enters of the same line are two lines, Up shows it once, and no ceiling (003 kept 1,000)");
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

// THE LINES A READER KEEPS (KEY_PLAN.md KEY-3): every line it answers at a terminal --
// typed and mended, empty, each line of a paste -- once, as Enter sent it, less what
// what_is_kept drops; never a line stopped with Ctrl-C, and never a piped line. The
// write hook, which satl points at the console's store, is pointed here at a counter
// of the harness's own: it is called after each line is kept, and told where.
const ConsoleLines *hooked_with = nullptr;
std::size_t hooked = 0, kept_when_hooked = 0;

void hook_called(const std::string &, const ConsoleLines *kept_in)
{
    ++hooked;
    hooked_with = kept_in;
    kept_when_hooked = kept_in != nullptr ? kept_in->size() : 0;
}

void kept()
{
    int master = -1, slave = -1;
    if (openpty(&master, &slave, nullptr, nullptr, nullptr) != 0)
        return check(false, "a pty for the lines a reader keeps");
    ConsoleLines lines;
    LineReader typed(slave, slave);
    typed.keep_lines_in(&lines);
    satellite004::line_entered() = hook_called;
    std::thread typist([master] {
        usleep(200000);  // after the reader waits: keys already there when it starts are dropped (D0.6.5)
        const std::string keys = "pritn\x7f\x7fnt\r" "\r" "half\x03" "\x1b[200~one\rtwo\x1b\rthree\r\x1b[201~";
        (void)!write(master, keys.data(), keys.size());
    });
    std::string answered, line;
    bool in_step = true;   // the hook is called once a line, after the line is kept, with the reader's lines
    for (int read = 0; read < 6; ++read) {
        const std::size_t before = hooked;
        const LineStatus status = typed.read("> ", line);
        answered += status == LineStatus::Line ? "[" + line + "]" : status == LineStatus::Interrupted ? "(interrupted)" : "(end)";
        in_step = in_step && hooked == before + (status == LineStatus::Line ? 1 : 0) &&
                  (status != LineStatus::Line || (hooked_with == &lines && kept_when_hooked == lines.size()));
    }
    typist.join();
    satellite004::line_entered() = nullptr;
    close(master);
    close(slave);
    std::string held;
    for (std::size_t index = 0; index < lines.size(); ++index)
        held += "[" + lines.at(index).to_utf8() + "]";
    check(answered == "[print][](interrupted)[one][two\x1b][three]" && held == "[print][][one][two][three]" && in_step,
          "a reader keeps each line it answers at a terminal once -- mended, empty, each of a paste, an ESC dropped "
          "-- and not the one Ctrl-C stopped, then calls the write hook with its lines (answered " +
              satellite004::shown(answered) + ", kept " + satellite004::shown(held) + ")");

    std::FILE *in = std::tmpfile(), *out = std::tmpfile();
    std::fputs("piped\nand another\n", in);
    std::fflush(in);
    lseek(fileno(in), 0, SEEK_SET);
    ConsoleLines none;
    LineReader piped(fileno(in), fileno(out));
    piped.keep_lines_in(&none);
    satellite004::line_entered() = hook_called;
    hooked = 0;
    int count = 0;
    while (piped.read("> ", line) == LineStatus::Line)
        ++count;
    satellite004::line_entered() = nullptr;
    std::fclose(in);
    std::fclose(out);
    check(count == 2 && none.size() == 0 && hooked == 0, "... and a piped line is nobody's typing: not kept, and the hook not called");
}

// LEFT AND RIGHT FROM CONSOLE TO CONSOLE (KEY_PLAN.md KEY-6, rule 5): Left moves back through the line, and
// from its start shows the console before at its last line, the cursor at its end; Right is the mirror,
// and back at this console the half-typed line returns. The consoles before this one are filled by hand,
// newest first, as the store fills them from its walk (KEY-5), and each is read when Left first reaches it.
int reads = 0;

EarlierConsoles::Read holding(std::vector<std::string> lines)
{
    return [lines](ConsoleLines &into) {
        ++reads;
        for (const std::string &line : lines)
            into.add(line);
        return true;
    };
}

void press(Editor &editor, Key k, std::size_t times = 1)
{
    for (std::size_t i = 0; i < times; ++i)
        editor.apply(key(k));
}

void across_consoles()
{
    ConsoleLines here;
    here.add("this console's own");
    EarlierConsoles earlier;
    earlier.add(holding({"three first", "three"}));   // the console just before this one
    earlier.add(holding({"", "    ", ""}));            // a console with no line to stop at
    earlier.add([](ConsoleLines &) {                   // a console whose file went after the walk
        ++reads;
        return false;
    });
    earlier.add(holding({"two"}));
    earlier.add(holding({"one a", "one"}));            // the oldest Left reaches

    Editor e(&here, &earlier);
    type(e, "abc");
    press(e, Key::Left, 3);
    const bool at_start = e.line() == "abc" && e.cursor() == 0 && reads == 0;
    e.apply(key(Key::Left));
    check(at_start && e.line() == "three" && e.cursor() == 5 && reads == 1,
          "on abc with the cursor at its end, three Lefts reach the start, and the fourth shows the last line of the "
          "console before, with the cursor at its end -- read only then");

    press(e, Key::Left, 5);
    const bool through = e.line() == "three" && e.cursor() == 0;
    e.apply(key(Key::Left));
    check(through && e.line() == "two" && e.cursor() == 3 && reads == 4,
          "Left goes on through that line, and at its start shows the line of the console before that -- a console "
          "with no line to stop at, and one that cannot be read, stepped over");
    press(e, Key::Left, 4);
    const bool oldest = e.line() == "one" && e.cursor() == 3;
    press(e, Key::Left, 10);
    check(oldest && e.line() == "one" && e.cursor() == 0 && reads == 5,
          "... and at the start of the oldest console's line, Left does nothing");

    e.apply(key(Key::End));
    e.apply(key(Key::Right));
    const bool after = e.line() == "two" && e.cursor() == 0;
    press(e, Key::Right, 3);
    e.apply(key(Key::Right));
    const bool after_again = e.line() == "three" && e.cursor() == 0;
    press(e, Key::Right, 5);
    e.apply(key(Key::Right));
    const bool back = e.line() == "abc" && e.cursor() == 0;
    e.apply(key(Key::End));
    e.apply(key(Key::Right));
    check(after && after_again && back && e.line() == "abc" && e.cursor() == 3 && reads == 5,
          "Right at the end of a line from a console before shows the console after, with the cursor at its start, "
          "and back at this console the half-typed line returns; at its end Right does nothing");

    Editor empty(&here, &earlier);
    empty.apply(key(Key::Left));
    const bool straight = empty.line() == "three" && empty.cursor() == 5;
    KeyDecoder decoder;
    Editor ctrl(&here, &earlier);
    ctrl.apply(decoder.feed(0x02));
    const bool ctrl_b = ctrl.line() == "three";
    ctrl.apply(key(Key::End));
    ctrl.apply(decoder.feed(0x06));
    check(straight && ctrl_b && ctrl.line().empty() && ctrl.cursor() == 0,
          "on an empty line, Left goes straight to the console before -- and Ctrl-B and Ctrl-F do as Left and Right");

    Editor home(&here, &earlier);
    type(home, "xyz");
    home.apply(key(Key::Home));
    home.apply(key(Key::Left));
    check(home.line() == "three" && home.cursor() == 5, "Home, then Left, goes to the console before");

    Editor browse(&here, &earlier);
    type(browse, "half");
    browse.apply(key(Key::Home));
    browse.apply(key(Key::Left));
    browse.apply(key(Key::Up));
    const bool up = browse.line() == "three first";
    browse.apply(key(Key::Up));
    const bool stops = browse.line() == "three first";
    browse.apply(key(Key::Down));
    const bool down = browse.line() == "three";
    browse.apply(key(Key::Down));
    const bool here_again = browse.line() == "half" && browse.cursor() == 4;
    browse.apply(key(Key::Up));
    check(up && stops && down && here_again && browse.line() == "this console's own",
          "Up and Down move inside a console before this one, Up stopping at its first line, and Down past its last "
          "line comes back here, to the half-typed line and this console's own lines");

    // ENTER ON A LINE FROM A CONSOLE BEFORE: the reader answers it, and keeps it as this console's line --
    // and the next line starts at this console, where Up brings it back.
    int master = -1, slave = -1;
    if (openpty(&master, &slave, nullptr, nullptr, nullptr) != 0)
        return check(false, "a pty for Enter on a line from a console before");
    ConsoleLines mine;
    EarlierConsoles before;
    before.add(holding({"from before"}));
    LineReader typed(slave, slave);
    typed.keep_lines_in(&mine);
    typed.left_reaches(&before);
    std::thread typist([master] {
        usleep(200000);  // after the reader waits: keys already there when it starts are dropped (D0.6.5)
        (void)!write(master, "\x1b[D\r", 4);
        usleep(200000);
        (void)!write(master, "\x1b[A\r", 4);
    });
    std::string first, second;
    const bool read_both = typed.read("> ", first) == LineStatus::Line && typed.read("> ", second) == LineStatus::Line;
    typist.join();
    close(master);
    close(slave);
    check(read_both && first == "from before" && second == "from before" && mine.size() == 2 &&
              mine.at(0).to_utf8() == "from before",
          "Enter on a line from a console before this one keeps it in this console, and the next line starts here "
          "(answered \"" + satellite004::shown(first) + "\" then \"" + satellite004::shown(second) + "\")");
}

} // namespace

int main()
{
    keys();
    editing();
    cells();
    piped();
    kept();
    across_consoles();
    std::printf("%s\n", failed == 0 ? "every prompt case passed" : "a prompt case FAILED");
    return failed == 0 ? 0 : 1;
}
