// satl — the interpreter.
//
// This binary links NO GUI: the window lives in satl-term, which spawns
// this one into a PTY. The split is measured, not tidy-minded — linking gtk4
// and vte here pulled 119 shared objects that the dynamic linker loaded before
// main() on every `--run`, costing 23.4 ms against an interpreter whose own
// share of hello world is 0.3 ms. See window.cpp.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "console_output/console.hpp"
#include "system_facts/version.hpp"
#include "interpreter/interp.hpp"
#include "satellite_library/library.hpp"
#include "satellite_string/satellite_string.hpp"
#include "system_facts/system.hpp"

// ---------------------------------------------------------------------------
// REPL side (runs inside the terminal)
// ---------------------------------------------------------------------------


#include "programs/main_internal.hpp"

// The prompt: its colors, and the three debug entry points the repl offers.
// Out of main.cpp, 2026-08-24. See programs/main_internal.hpp.

// Prompt colors, decided at RUN TIME rather than compiled in, because the two
// places satl runs want opposite things and one answer is wrong in one of them.
//
// These were a fixed pair chosen against satl-term's light blue field (#90D5FF,
// see apply_colors in window.cpp): the username white, the path black. In the
// window that is right. Run from a console it is a guess about somebody else's
// background, and on a light one the guess put a WHITE username on a white
// terminal -- reported, and the reason this is now a decision instead of a
// constant.
//
// satl-term sets SATL_TERM when it spawns this binary into its PTY, so the one
// process that knows what the background is is the one that says so. Everywhere
// else nothing absolute can be correct, so nothing absolute is used: the
// username is BOLD and the path is plain, both in the terminal's own foreground
// color, which is legible on a light background and a dark one by construction
// rather than by luck.
//
// 24-bit SGR for the satl-term pair rather than the classic 30-37 range,
// because those eight indices are whatever palette VTE happens to ship -- its
// "white" is a light grey and its bold black is dark grey, so neither would be
// the color asked for.
const char *color_user = "";
const char *color_cwd  = "";
const char *color_off  = "";

void choose_prompt_colors()
{
    // Not a terminal, no escapes. `satl | cat` and `satl > log` used to get the
    // SGR sequences written into the pipe, where they are noise that a reader
    // has to strip -- and they were only ever there for a screen.
    if (!isatty(STDOUT_FILENO))
        return;

    // NO_COLOR, the convention every tool that emits color is expected to
    // honour: set, non-empty, and the answer is no. https://no-color.org
    if (const char *no_color = getenv("NO_COLOR"); no_color && *no_color)
        return;

    color_off = "\033[0m";

    // Inside satl-term the field is known exactly, so the exact pair is used.
    if (getenv("SATL_TERM")) {
        color_user = "\033[1;38;2;255;255;255m";
        color_cwd  = "\033[1;38;2;0;0;0m";
        return;
    }

    // A console. Bold, and no color at all -- the username stands out from the
    // path without either of them asserting a color the background may be.
    color_user = "\033[1m";
    color_cwd  = "";
}

void evaluate(const std::string &line, satellite::Console *console)
{
    std::string out = satellite::eval_line(line, console);
    if (out.empty())
        return;
    // fwrite rather than printf: the output is satellite text that has already
    // been decoded, and it may contain a '%'. The flush is required because
    // glibc line-buffers stdout on a tty.
    fwrite(out.data(), 1, out.size(), stdout);
    fflush(stdout);
}

satellite::Value parse_value(const std::string &text)
{
    if (text == "true")
        return true;
    if (text == "false")
        return false;
    if (text == "nil")
        return std::monostate{};
    satellite::Number number;
    if (satellite::Number::parse(text, number))
        return number;
    return satellite::make_string(satellite::encode(text));
}

// Temporary ':' commands for poking satellite.library until the parser
// exists. Not language syntax.
//
// `reader` may be null, which is every caller that has no line editor. Only
// :history needs one, and it says so rather than pretending there is nothing
// to show.
void debug_command(const std::string &line,
                   const satellite::LineReader *reader)
{
    auto &lib = satellite::Library::instance();
    std::istringstream in(line);
    std::string cmd;
    in >> cmd;

    if (cmd == ":vars") {
        for (const std::string &key : lib.list()) {
            auto v = lib.get_path(key);
            printf("satellite.library.%s = %s\n", key.c_str(),
                   v ? satellite::to_string(*v).c_str() : "nil");
        }
        return;
    }
    if (cmd == ":set") {
        std::string path, rest;
        in >> path;
        std::getline(in, rest);
        rest.erase(0, rest.find_first_not_of(' '));
        if (path.empty() || rest.empty()) {
            printf("usage: :set <function>.<var> <value>\n");
            return;
        }
        if (!lib.set_path(path, parse_value(rest)))
            printf("satellite: bad path (want <function>.<var>): %s\n",
                   path.c_str());
        return;
    }
    if (cmd == ":get") {
        std::string path;
        in >> path;
        auto v = lib.get_path(path);
        if (v)
            printf("%s\n", satellite::to_string(*v).c_str());
        else
            printf("satellite: no such variable: %s\n", path.c_str());
        return;
    }
    // :history — the previous entries, and WHERE THEY ARE KEPT.
    //
    // The path is printed rather than left to be discovered, and that is the
    // point of the command as much as the list is. A prompt that quietly
    // writes a file into somebody's home directory because they pressed the up
    // arrow is doing something behind their back; one that will say the path
    // when asked, and that honours $SATL_HISTORY=none, is not. See
    // console_input/history.hpp.
    if (cmd == ":history") {
        if (!reader) {
            printf("satellite: no line editor here, so no history\n");
            return;
        }
        const satellite::History &history = reader->history();
        const std::vector<std::string> &entries = history.entries();

        for (size_t i = 0; i < entries.size(); i++)
            printf("%5zu  %s\n", i + 1, entries[i].c_str());

        if (history.path().empty())
            printf("(%zu line%s, this session only — $SATL_HISTORY is off)\n",
                   entries.size(), entries.size() == 1 ? "" : "s");
        else
            printf("(%zu line%s, kept in %s — $SATL_HISTORY moves it, "
                   "$SATL_HISTORY=none turns it off)\n",
                   entries.size(), entries.size() == 1 ? "" : "s",
                   history.path().c_str());
        return;
    }

    printf("satellite: unknown command %s (have :set, :get, :vars, "
           ":history)\n",
           cmd.c_str());
}
