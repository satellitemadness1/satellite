// satellite/bytecode/console_style.cpp -- the header says what each of the four
// places a colour can be said means, and which of 003's rules all four keep.

#include "console_style.hpp"

#include <atomic>
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include <unistd.h>

namespace satellite004 {
namespace {

// "255;136;0" -- what SGR's 24-bit form takes after 38;2; or 48;2;.
std::string decimal_triplet(unsigned int rgb)
{
    return std::to_string((rgb >> 16) & 0xFFu) + ";" + std::to_string((rgb >> 8) & 0xFFu) + ";" +
           std::to_string(rgb & 0xFFu);
}

// "rgb:ff/88/00" -- XParseColor's spelling, which OSC 10 and 11 take in xterm, VTE
// and every terminal that copied them.
std::string osc_colour(unsigned int rgb)
{
    static const char digits[] = "0123456789abcdef";
    std::string out = "rgb:";
    for (int shift = 16; shift >= 0; shift -= 8) {
        const unsigned int byte = (rgb >> shift) & 0xFFu;
        out += digits[byte >> 4];
        out += digits[byte & 0xFu];
        if (shift != 0) out += '/';
    }
    return out;
}

// EVERY `from` IN `text` BECOMES `to`. How an inner string's ending is re-opened as
// the outer colour (the header's "a colour inside a colour comes back").
std::string replaced(std::string text, const std::string &from, const std::string &to)
{
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size()))
        text.replace(at, from.size(), to);
    return text;
}

// EACH NEWLINE-FREE PIECE WRAPPED ON ITS OWN, the newlines plain between them --
// 003's rule, so a background never paints the row a newline moves to.
std::string wrapped(const std::string &text, const std::string &open, const std::string &close)
{
    std::string out;
    std::size_t from = 0;
    for (;;) {
        const std::size_t newline = text.find('\n', from);
        const std::size_t to = newline == std::string::npos ? text.size() : newline;
        if (to > from)
            out += open + text.substr(from, to - from) + close;
        if (newline == std::string::npos)
            break;
        out += '\n';
        from = newline + 1;
    }
    return out;
}

const std::string kForegroundEnds = "\033[39m";
const std::string kBackgroundEnds = "\033[49m";

std::string foreground_opens(unsigned int rgb) { return "\033[38;2;" + decimal_triplet(rgb) + "m"; }
std::string background_opens(unsigned int rgb) { return "\033[48;2;" + decimal_triplet(rgb) + "m"; }

// THE CONSOLE'S COLOURS, set by satellite.console.foreground / .background.
// `ever_set` is read without the lock, so display pays nothing until a program
// first asks for a console colour.
std::atomic<bool> ever_set{false};
std::mutex console_lock;
TextStyle console_style;

// THE TERMINAL'S COLOURS, set by satellite.terminal.foreground / .background: one
// flag each, so only what was changed is put back. sig_atomic_t, because a signal
// handler reads them.
volatile sig_atomic_t foreground_changed = 0;
volatile sig_atomic_t background_changed = 0;
const char kForegroundBack[] = "\033]110\033\\";
const char kBackgroundBack[] = "\033]111\033\\";

// A SIGNAL THAT WOULD END satl WRITES THE TERMINAL'S OWN COLOURS BACK FIRST. SA_RESETHAND
// has already put the default back, so the raise() -- held until this returns -- ends
// satl exactly as the signal would have, with its own status; a fault returns to the
// faulting instruction and takes the default the same way.
extern "C" void put_back_and_go(int signal_number)
{
    put_the_terminal_back_now();
    raise(signal_number);
}

// ARRANGED ONCE, AT THE FIRST CHANGE, and only over a signal nothing else has claimed:
// the prompt's session has its own Ctrl-C, and taking it would break it. A stack of
// its own, so a capsule that overflows the stack -- left to crash, as the author ruled
// -- still gets the terminal's colours back on its way out.
//
// A STACK OVERFLOW ON ANOTHER THREAD is not covered: an alternate stack belongs to the
// thread that set it, and this runs on whichever thread first changed a colour.
void arrange_the_undoing_once()
{
    static char spare_stack[64 * 1024];
    stack_t alternate{};
    alternate.ss_sp = spare_stack;
    alternate.ss_size = sizeof spare_stack;
    sigaltstack(&alternate, nullptr);

    const int endings[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT, SIGSEGV, SIGBUS, SIGABRT, SIGFPE, SIGILL};
    for (const int each : endings) {
        struct sigaction before{};
        if (sigaction(each, nullptr, &before) != 0 || before.sa_handler != SIG_DFL)
            continue;
        struct sigaction mine{};
        mine.sa_handler = put_back_and_go;
        sigemptyset(&mine.sa_mask);
        mine.sa_flags = SA_RESETHAND | SA_ONSTACK;
        sigaction(each, &mine, nullptr);
    }
}

void arrange_the_undoing()
{
    static std::once_flag arranged;
    std::call_once(arranged, arrange_the_undoing_once);
}

// ONE SGR SEQUENCE'S PARAMETERS WITHOUT ITS COLOURS -- 38;2;r;g;b, 48;2;r;g;b, 38;5;n,
// 48;5;n and the plain 30-37, 40-47, 90-97, 100-107, 39 and 49 -- for NO_COLOR.
std::string without_colours(const std::string &parameters)
{
    std::vector<int> numbers;
    std::size_t from = 0;
    for (;;) {
        const std::size_t semicolon = parameters.find(';', from);
        const std::string one = parameters.substr(from, semicolon == std::string::npos ? std::string::npos : semicolon - from);
        numbers.push_back(one.empty() ? 0 : std::atoi(one.c_str()));
        if (semicolon == std::string::npos) break;
        from = semicolon + 1;
    }
    std::string kept;
    for (std::size_t at = 0; at < numbers.size(); ++at) {
        const int n = numbers[at];
        if ((n == 38 || n == 48) && at + 1 < numbers.size()) {
            at += numbers[at + 1] == 2 ? 4 : numbers[at + 1] == 5 ? 2 : 1;
            continue;
        }
        if ((n >= 30 && n <= 39) || (n >= 40 && n <= 49) || (n >= 90 && n <= 97) || (n >= 100 && n <= 107))
            continue;
        kept += (kept.empty() ? "" : ";") + std::to_string(n);
    }
    return kept;
}

} // namespace

bool colour_of(const Value &given, unsigned int &rgb, std::string &why)
{
    if (const satellite_color *colour = given.as_color()) {
        if (colour->transparency != satellite_color::kSolid) {
            why = colour->written() + " is see-through, and a terminal draws nothing see-through -- give it a "
                                      "solid colour, x" + colour->digits();
            return false;
        }
        rgb = colour->rgb;
        return true;
    }
    if (const satellite_hexadecimal_number *hex = given.as_hexadecimal()) {
        satellite_color made;
        if (hex->negative() || hex->digits().size() != 6 || !satellite_color::from_digits(hex->digits(), made)) {
            why = "a colour is exactly six hex digits, like xFF8800, and " + hex->written() + " is not";
            return false;
        }
        rgb = made.rgb;
        return true;
    }
    why = std::string("a colour is six hex digits, like xFF8800, or a satellite.variable.color, and this is ") +
          (given.is_string() ? "\"" + given.text_utf8() + "\", a string" : std::string(given.kind_name()));
    return false;
}

bool style_reaches_a_terminal()
{
    static const bool a_terminal = isatty(STDOUT_FILENO) != 0;
    return a_terminal;
}

bool colours_reach_a_terminal()
{
    static const bool colours = [] {
        const char *no_color = std::getenv("NO_COLOR");
        return no_color == nullptr || *no_color == '\0';
    }();
    return colours && style_reaches_a_terminal();
}

std::string for_the_screen(const std::string &line)
{
    if (colours_reach_a_terminal())
        return line;
    const bool any_style = style_reaches_a_terminal();
    std::string out;
    out.reserve(line.size());
    for (std::size_t at = 0; at < line.size();) {
        // ESC [ then digits and semicolons then m: one SGR sequence. Anything else --
        // ESC[H, ESC[2J, a lone ESC -- is not style, and goes out as it is.
        if (line[at] == '\033' && at + 1 < line.size() && line[at + 1] == '[') {
            std::size_t end = at + 2;
            while (end < line.size() && (std::isdigit(static_cast<unsigned char>(line[end])) || line[end] == ';')) ++end;
            if (end < line.size() && line[end] == 'm') {
                if (any_style) {
                    const std::string kept = without_colours(line.substr(at + 2, end - at - 2));
                    const bool was_a_reset = end == at + 2 || line.substr(at + 2, end - at - 2) == "0";
                    if (was_a_reset || !kept.empty())
                        out += "\033[" + (was_a_reset ? line.substr(at + 2, end - at - 2) : kept) + "m";
                }
                at = end + 1;
                continue;
            }
        }
        out += line[at++];
    }
    return out;
}

std::string screen_text(std::string line)
{
    if (colours_reach_a_terminal() || line.find('\033') == std::string::npos)
        return line;
    return for_the_screen(line);
}

std::string styled_line(const std::string &text, const TextStyle &style)
{
    if (!style.any() || !style_reaches_a_terminal())
        return text;
    const bool colours = colours_reach_a_terminal();

    // ONE SGR SEQUENCE, parameters joined by `;`, in 003's order -- bold, italic,
    // foreground, background -- so the bytes are 003's bytes.
    std::string parameters;
    const auto add = [&parameters](const std::string &part) {
        if (!parameters.empty()) parameters += ';';
        parameters += part;
    };
    if (style.bold) add("1");
    if (style.italic) add("3");
    const bool foreground = colours && style.has_foreground;
    const bool background = colours && style.has_background;
    if (foreground) add("38;2;" + decimal_triplet(style.foreground));
    if (background) add("48;2;" + decimal_triplet(style.background));
    if (parameters.empty())
        return text;

    // A COLOURED STRING INSIDE THE LINE ends its own colour with ESC[39m or ESC[49m,
    // and the line's colour comes back there rather than the terminal's own.
    std::string inside = text;
    if (foreground) inside = replaced(inside, kForegroundEnds, foreground_opens(style.foreground));
    if (background) inside = replaced(inside, kBackgroundEnds, background_opens(style.background));
    return wrapped(inside, "\033[" + parameters + "m", "\033[0m");
}

std::string coloured_text(const std::string &text, unsigned int rgb, bool behind)
{
    const std::string open = behind ? background_opens(rgb) : foreground_opens(rgb);
    const std::string &close = behind ? kBackgroundEnds : kForegroundEnds;
    return wrapped(replaced(text, close, open), open, close);
}

void set_console_colour(bool behind, bool set, unsigned int rgb)
{
    const std::lock_guard<std::mutex> held(console_lock);
    if (behind) {
        console_style.has_background = set;
        console_style.background = rgb;
    } else {
        console_style.has_foreground = set;
        console_style.foreground = rgb;
    }
    ever_set.store(true, std::memory_order_release);
}

bool console_colours_ever_set()
{
    return ever_set.load(std::memory_order_acquire);
}

TextStyle console_colours()
{
    if (!ever_set.load(std::memory_order_acquire))
        return TextStyle{};
    const std::lock_guard<std::mutex> held(console_lock);
    return console_style;
}

void set_terminal_colour(bool behind, bool set, unsigned int rgb)
{
    if (!colours_reach_a_terminal())
        return;
    if (set) {
        arrange_the_undoing();
        (behind ? background_changed : foreground_changed) = 1;
        std::cout << "\033]" << (behind ? "11;" : "10;") << osc_colour(rgb) << "\033\\" << std::flush;
    } else {
        (behind ? background_changed : foreground_changed) = 0;
        std::cout << (behind ? kBackgroundBack : kForegroundBack) << std::flush;
    }
}

void put_the_terminal_back()
{
    if (foreground_changed == 0 && background_changed == 0)
        return;
    std::cout << (foreground_changed != 0 ? kForegroundBack : "") << (background_changed != 0 ? kBackgroundBack : "")
              << std::flush;
    foreground_changed = 0;
    background_changed = 0;
}

void put_the_terminal_back_now()
{
    if (foreground_changed != 0) {
        const ssize_t wrote = write(STDOUT_FILENO, kForegroundBack, sizeof kForegroundBack - 1);
        (void)wrote;
        foreground_changed = 0;
    }
    if (background_changed != 0) {
        const ssize_t wrote = write(STDOUT_FILENO, kBackgroundBack, sizeof kBackgroundBack - 1);
        (void)wrote;
        background_changed = 0;
    }
}

} // namespace satellite004
