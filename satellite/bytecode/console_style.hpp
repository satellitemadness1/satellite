#pragma once
// satellite/bytecode/console_style.hpp -- COLOUR AND STYLE ON A TERMINAL: what 003's
// M30 built as "what ncurses does, without ncurses", and what 004 adds to it.
//
// The author, 2026-09-23, shown 003 refusing `s.foreground(xFF8800)`,
// `"text".foreground(...)`, `satellite.console.foreground(...)`,
// `satellite.terminal.foreground(...)` and `input("name? ", foreground=...)`: *"Let's
// build 004 differently then, so every single one of those errors is legal"*. So the
// colour of text can be said in four places, and each one means one thing:
//
//     satellite.console.display("warning", foreground=xFF8800, bold=satellite.bool.true)
//         003's, exactly: this one line, and nothing after it.
//     "OK".foreground(x00FF00)          s.background(x000000)
//         THE STRING, IN THAT COLOUR -- a value, so it can be joined into a line:
//         "status: " + "OK".foreground(x00FF00) colours one word of it.
//     satellite.console.foreground(xFF8800)      satellite.console.background(...)
//         EVERY LINE display PRINTS FROM NOW ON, until () puts it back.
//     satellite.terminal.foreground(xFF8800)     satellite.terminal.background(...)
//         THE TERMINAL'S OWN COLOURS -- the whole window, empty rows and all (OSC 10
//         and 11, 003 PLAN M30's open item) -- put back when satl ends, however it ends.
//
// ---------------------------------------------------------------------------
// 003'S RULES, KEPT FOR ALL FOUR (003 satellite_console/style.cpp, confirmed by
// running 003 under a terminal on 2026-09-23):
// ---------------------------------------------------------------------------
//   - A colour is exactly six hex digits -- xRRGGBB, or a satellite.variable.color --
//     and it is checked even when nothing will be written, so a program that is wrong
//     at a terminal is wrong in a pipe too.
//   - NOTHING IS WRITTEN WHEN stdout IS NOT A TERMINAL: a pipe or a file gets the
//     plain text (the author, 2026-09-12). satl's own console IS a terminal (VTE).
//   - NO_COLOR SET AND NOT EMPTY DROPS THE COLOURS and keeps bold and italic
//     (no-color.org, the author, 2026-09-12).
//   - A STYLE NEVER SPANS A NEWLINE: a background still set when the terminal moves
//     to a new row paints the whole row in some terminals, so every newline-free piece
//     is wrapped on its own and the newlines go out plain (003, for dark_mechanicum).
//
// ---------------------------------------------------------------------------
// A STRING'S COLOUR IS IN THE STRING, AS THE TERMINAL'S OWN CODES.
// ---------------------------------------------------------------------------
//
// `"OK".foreground(x00FF00)` answers ESC[38;2;0;255;0m OK ESC[39m -- the string a
// terminal draws green. That is what every scripting language's colour library does
// (termcolor, chalk), and it is what lets a coloured word be JOINED into a line with
// `+`, which no per-line option can do.
//
// THE STRING IS THE SAME WHEREVER THE PROGRAM RUNS, and the screen is what differs
// (the review, 2026-09-23). It first came back plain when stdout was a pipe -- chalk's
// rule -- and then `.find` and `==` answered one thing at a terminal and another in a
// pipe, which is 003's "a program wrong at a terminal is wrong in a pipe too" broken
// the other way. So the codes are always in the string: `.find` and `==` see them, and
// a file written with one holds them. DISPLAY IS WHERE THEY ARE LEFT OUT -- into a
// pipe or a file every SGR code goes, under NO_COLOR only the colours -- by
// for_the_screen(), on everything display and input's prompt write.
//
// A COLOUR INSIDE A COLOUR COMES BACK: ("a" + "b".foreground(blue) + "c").foreground(red)
// draws c red, not in the terminal's own colour, because the inner string's ending
// code is re-opened as the outer colour when the outer one is put round it.

#include "value.hpp"

#include <string>

namespace satellite004 {

// A COLOUR, AS ONE 24-BIT NUMBER 0xRRGGBB, from a satellite.variable.color or a
// six-digit hex -- or false with `why` set. A colour with a transparency is refused:
// a terminal draws nothing see-through, and dropping it without a word would draw a
// colour the program did not ask for.
bool colour_of(const Value &given, unsigned int &rgb, std::string &why);

// IS ANY STYLE WRITTEN AT ALL -- stdout is a terminal. And are COLOURS written --
// that, and NO_COLOR unset or empty. ASKED ONCE, at the first display: stdout does
// not stop being a terminal during a run, and satl's own console is handed stdout
// before the program's first line. A syscall per display was measurable.
bool style_reaches_a_terminal();
bool colours_reach_a_terminal();

// WHAT A LINE BECOMES ON ITS WAY OUT. At a terminal with colours, itself; into a pipe
// or a file, with every SGR code (ESC [ digits and ; m) taken out; under NO_COLOR,
// with only the colours taken out of each. Nothing else is touched -- clear() and
// home() are not SGR and go out as 003 wrote them. screen_text() is the cheap door:
// at a terminal it costs one flag read, and elsewhere a scan for ESC first.
std::string for_the_screen(const std::string &line);
std::string screen_text(std::string line);

// WHAT display'S NAMED OPTIONS, OR THE CONSOLE'S COLOURS, ASK FOR.
struct TextStyle {
    bool has_foreground = false;
    bool has_background = false;
    unsigned int foreground = 0;       // 0xRRGGBB
    unsigned int background = 0;
    bool bold = false;
    bool italic = false;

    bool any() const { return has_foreground || has_background || bold || italic; }
};

// 003's display wrapping: ONE SGR SEQUENCE opened before each newline-free piece and
// ESC[0m after it, or the text untouched when nothing is to be written. Colours are
// dropped here under NO_COLOR, so a caller passes what was ASKED for. UTF-8.
std::string styled_line(const std::string &text, const TextStyle &style);

// A STRING'S OWN .foreground(c) / .background(c): wrapped in that colour and its own
// ending (ESC[39m / ESC[49m), each newline-free piece on its own, wherever the
// program runs (the header says why). UTF-8 in and out.
std::string coloured_text(const std::string &text, unsigned int rgb, bool behind);

// satellite.console.foreground(c) / .background(c): the colour every later display
// line is drawn in when the call names none of its own. `set` false puts it back to
// the terminal's own. The console is ONE for the whole run -- it is where every
// thread's lines go -- so these are held once, behind a lock that is not taken at
// all until a program first sets one.
void set_console_colour(bool behind, bool set, unsigned int rgb);
TextStyle console_colours();
// ONE LOAD, for the display that runs before any program set a console colour --
// which is every display of every program that never does.
bool console_colours_ever_set();

// satellite.terminal.foreground(c) / .background(c): OSC 10 / OSC 11, the terminal's
// DEFAULT colours, when colours reach a terminal. `set` false is OSC 110 / 111, the
// terminal's own again -- ITS PROFILE'S, which is not always what it had: a theme a
// shell set with OSC 10/11 (pywal, base16) is not remembered, because asking a
// terminal what it has means reading its answer off stdin, where the program's input
// is. Only a colour that was changed is put back. THE FIRST CHANGE ARRANGES ITS OWN UNDOING: Ctrl-C, a hang-up,
// a kill or a crash writes 110 and 111 before satl goes, and put_the_terminal_back()
// does it on every ordinary ending (main(), after the run).
void set_terminal_colour(bool behind, bool set, unsigned int rgb);
void put_the_terminal_back();
// THE SAME FROM A SIGNAL HANDLER: write() and nothing else. The prompt's session calls
// it before its own _exit on a second Ctrl-C or a hang-up, which leave no other way.
void put_the_terminal_back_now();

} // namespace satellite004
