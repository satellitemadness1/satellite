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
#include "console_input/console_input.hpp"
#include "system_facts/version.hpp"
#include "interpreter/interp.hpp"
#include "satellite_library/library.hpp"
#include "satellite_string/satellite_string.hpp"
#include "system_facts/system.hpp"

// ---------------------------------------------------------------------------
// REPL side (runs inside the terminal)
// ---------------------------------------------------------------------------


#include "programs/main_internal.hpp"

// The read-eval-print loop, and the two things that end a line early:
// an exit word, and an abandoned multi-line block.
// Out of main.cpp, 2026-08-24. See programs/main_internal.hpp.

// exit, quit, and satellite.return(...) — three ways of saying the same thing
// to a prompt, and all three now say it.
//
// `exit` and `quit` were bare words only. `exit()` — which is what someone who
// has spent ten minutes typing satellite.directory.list() reaches for next —
// fell through to the evaluator and came back as an unknown name. A prompt
// that refuses the parenthesised form of its own quit command is arguing with
// the reflex it just spent ten minutes teaching.
//
// satellite.return(satellite) is how the language ends a program (§2), and at
// this prompt the session IS the program, so it ends that. The argument is
// read for an exit status rather than evaluated: the REPL is one line at a
// time and there is no capsule to return FROM, so the alternative was the
// parser's "return outside a capsule", which is true and unhelpful about a
// line whose meaning is obvious.
//
// Checked before evaluation, exactly like `help` and `run` below, and legal
// for the same reason: `exit` and `quit` are bare identifiers the user owns,
// and satellite.return at the TOP level of a prompt is not otherwise a
// sentence the language accepts.
bool exit_command(const std::string &line, int &status)
{
    size_t a = line.find_first_not_of(" \t");
    if (a == std::string::npos)
        return false;
    size_t b = line.find_last_not_of(" \t");
    const std::string s = line.substr(a, b - a + 1);

    if (s == "exit" || s == "quit" || s == "exit()" || s == "quit()") {
        status = 0;
        return true;
    }

    const std::string keyword = "satellite.return";
    if (s.compare(0, keyword.size(), keyword) != 0)
        return false;

    std::string rest = s.substr(keyword.size());
    // Bare `satellite.return`, the way `satellite.help` is reachable bare.
    if (rest.empty()) {
        status = 0;
        return true;
    }
    if (rest.front() != '(' || rest.back() != ')')
        return false;
    rest = rest.substr(1, rest.size() - 2);

    std::string arg;
    for (char c : rest)
        if (c != ' ' && c != '\t')
            arg += c;

    // satellite means success, which is what satellite.return(satellite) has
    // always meant. A whole number means itself, truncated the way every exit
    // status is. Anything else is success, because a prompt has nowhere to put
    // a value and saying so would be a lecture, not an answer.
    status = 0;
    if (!arg.empty() && arg.find_first_not_of("0123456789") == std::string::npos)
        status = static_cast<int>(strtol(arg.c_str(), nullptr, 10) & 0xff);
    return true;
}

// Both ways out of a half-typed block, in one place so they cannot report it
// differently. Counting the lines is the point: "abandoned" without a number
// leaves the reader wondering how much they lost.
void abandon_block(std::string &block, int &depth)
{
    const long lines = std::count(block.begin(), block.end(), '\n');
    printf("satellite: multi-line entry abandoned, %ld line%s discarded\n",
           lines, lines == 1 ? "" : "s");
    fflush(stdout);
    block.clear();
    depth = 0;
}

int run_repl()
{
    start_runtime();
    choose_prompt_colors();

    // One Console for the whole session. It is what gives the prompt a printer
    // thread — so satellite.console.display(100ms) has something to pace, and
    // the setting survives from one line to the next, which an Evaluator that
    // is rebuilt per line could never do. Declared before the loop and
    // destroyed after it, so the thread outlives every write to it.
    satellite::Console console;

    // The banner reads its number from the same place --version does, so the
    // prompt and the flag cannot drift into disagreeing about what is running.
    printf("satellite %s  (run <file> interprets a file; :set / :get / :vars "
           "poke satellite.library; :history lists past lines and says where "
           "they are kept; shuts down if free memory < system.min_free_mb)\n",
           satellite::version_line().c_str());
    // Said out loud because there is no shell behind this prompt and no `cd`
    // to fall back on: a name nobody has read is a name nobody can guess.
    printf("type  help  for the whole language on one screen\n");
    printf("change directory: satellite.directory.change(\"some_dir\")   "
           "where you are: satellite.directory.current()\n");

    // The line editor, and the history it loads and saves. Declared before the
    // loop and destroyed after it, so the history file is written once on the
    // way out rather than once per line.
    satellite::LineReader reader;

    std::string line;

    // Multi-line entry. `block` is the accumulated source with REAL newlines in
    // it -- not spaces -- because expect_statement_end() decides that two
    // statements are illegally on one line by comparing their tokens' line
    // numbers, so joining with anything else turns a correct capsule into
    // "unexpected Word(satellite) after the end of a statement". It also makes
    // the line numbers in an error report count the lines the user typed.
    std::string block;
    int depth = 0;
    bool hinted = false;

    for (;;) {
        // The prompt is now BUILT and handed to the line editor rather than
        // printed here, and that is what the arrow keys cost. A redraw has to
        // rewrite the prompt -- moving the cursor left over a wrapped line
        // means knowing how many columns the prompt took -- so a prompt this
        // loop printed for itself would leave the editor unable to measure it.
        // The text is identical either way; only who writes it moved.
        std::string prompt;

        if (depth > 0) {
            // A continuation line prints INDENTATION AND NO MARKER, and that is
            // the load-bearing choice rather than a plain one.
            //
            // The tty echoes what the user types immediately after whatever we
            // last wrote, so the screen column of their first character is
            // exactly the width of what we printed. The same indentation is
            // stored into `block` below, so the stored column and the screen
            // column stay equal -- and an error caret, which is a byte offset
            // into the stored line, lands under the character the reader is
            // actually looking at. A "... " marker would print four columns the
            // source does not contain and put every caret in the block wrong.
            //
            // The drain is the same barrier §19.5 uses before reading input: the
            // prompt goes straight to stdout while program output goes through
            // the Console's printer thread, so without it a queued line could
            // land after the indentation we just wrote.
            console.drain();
            prompt.assign(static_cast<size_t>(depth) * 4, ' ');
        } else {
            // linux_username, cwd: type_here
            console.drain();
            prompt = std::string(color_user) + satellite::username() +
                     color_off + ", " + color_cwd + satellite::cwd() +
                     color_off + ": ";
        }

        const satellite::LineStatus status = reader.read(prompt, line);

        if (status == satellite::LineStatus::Interrupted) {
            // Ctrl-C at the prompt cancels the LINE, never the session. That
            // is the whole difference between this and what happened before:
            // with no handler and no raw mode, the terminal's INTR killed the
            // process and took the session with it, which is not what anybody
            // means by it at a prompt.
            //
            // Inside a block it abandons the block, which is exactly what
            // :cancel and Ctrl-D already do -- three ways out of a half-typed
            // capsule, all of them saying how many lines went.
            if (depth > 0)
                abandon_block(block, depth);
            continue;
        }

        if (status == satellite::LineStatus::EndOfFile) {   // Ctrl-D
            if (depth > 0) {
                // Inside a block, Ctrl-D ABANDONS the block and stays at the
                // prompt. Exiting instead would drop a half-typed capsule with
                // no diagnostic at all: the work is gone either way, and this
                // way the person is told it went.
                abandon_block(block, depth);
                continue;
            }
            return 0;
        }

        // Remembered BEFORE anything decides what the line means, so that a
        // line with a syntax error, an abandoned block's lines and a `:`
        // command are all recallable. A history that only kept what worked
        // would be missing exactly the lines somebody wants back to fix.
        reader.remember(line);

        // Inside a block, before every other prompt convenience: `run`, `help`
        // and the `:` commands are not satellite syntax, and a capsule body is.
        if (depth > 0) {
            if (line == ":cancel") {
                abandon_block(block, depth);
                continue;
            }

            // The indentation goes into the SOURCE and not merely onto the
            // screen -- see the note above the prompt.
            const satellite::BlockScan scan = satellite::scan_block(line);

            for (int i = 0; i < depth; i++)
                block += "    ";
            block += line;
            if (scan.opens_body) {
                // A nested head gets its brace on the same terms the outermost
                // one did. This arm exists because the first version supplied
                // the brace only at depth 0, so a satellite.statement.if typed
                // INSIDE a capsule -- which is where nearly every one of them
                // lives -- opened a level the source never contained.
                block += " {";
                printf("{\n");
            }
            block += '\n';

            depth += scan.depth;
            if (depth <= 0) {
                evaluate(block, &console);
                block.clear();
                depth = 0;
            }
            continue;
        }
        if (int status = 0; exit_command(line, status))
            return status;
        if (line.empty())
            continue;
        if (line[0] == ':') {
            debug_command(line, &reader);
            continue;
        }
        // Bare `help`, routed to satellite.help. It belongs HERE and not in the
        // language, because §1 says a bare identifier names something the USER
        // owns -- putting `help` in the grammar would quietly make it the
        // language's second reserved word. As a prompt convenience it shadows
        // nothing: it joins run, :set, :get and :vars, none of which are
        // satellite syntax either.
        //
        // And it defers to a real variable of that name, so someone who wrote
        // `satellite.variable.number help = 3` still gets 3. The person who
        // named a variable `help` meant it; the person who typed `help` into a
        // prompt is asking a question.
        if (line == "help" &&
            !satellite::Library::instance().get("main", "help")) {
            evaluate("satellite.help", &console);
            continue;
        }

        // Checked before evaluation: `run` is not satellite syntax (every name
        // in the language is satellite.-prefixed), so nothing legal is shadowed.
        if (satellite::RunCommand command = satellite::parse_run_command(line);
            command.matched) {
            run_command(command, &console);
            continue;
        }

        // Does this line open a body the prompt should keep reading into?
        //
        // Last, so that nothing above it changes meaning: `run`, `help` and the
        // `:` commands are all matched first and none of them can open a brace.
        if (satellite::BlockScan scan = satellite::scan_block(line);
            !scan.lex_error && scan.depth > 0) {
            block = line;
            if (scan.opens_body) {
                // Supply the brace AND SHOW IT. A prompt that silently rewrites
                // what somebody typed is a prompt they cannot reason about, and
                // the echoed '{' is also what makes the indentation below read
                // as the inside of a block rather than as a hung program.
                block += " {";
                printf("{\n");
            }
            block += '\n';
            depth = scan.depth;

            if (!hinted) {
                printf("  (multi-line: the block ends at its closing brace; "
                       ":cancel or Ctrl-D abandons it)\n");
                hinted = true;
            }
            continue;
        }

        evaluate(line, &console);
    }
}
