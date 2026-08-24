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
static const char *color_user = "";
static const char *color_cwd  = "";
static const char *color_off  = "";

static void choose_prompt_colors()
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

static void evaluate(const std::string &line, satellite::Console *console)
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

static satellite::Value parse_value(const std::string &text)
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
static void debug_command(const std::string &line)
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
    printf("satellite: unknown command %s (have :set, :get, :vars)\n",
           cmd.c_str());
}

// The shutdown threshold is an ordinary satellite.library variable, so
// :set system.min_free_mb <mb> retunes the guard live.
static void start_runtime()
{
    satellite::Library::instance().set("system", "min_free_mb", 4096);
    satellite::start_memory_watchdog();
}

// satl --run <file> [args...] — headless, straight to stdout. This is
// also what makes the interpreter testable without a display server.
static int run_file_mode(const std::string &path,
                         const std::vector<std::string> &args)
{
    start_runtime();

    // The one mode that gets a Console. Headless and straight to stdout is
    // exactly the case where output should appear WHILE the program runs
    // rather than after it, and it is the only mode whose stdout is certain to
    // be the place the program's output belongs — satl-term's REPL renders
    // into a window, and eval_line() reads its text back to echo it.
    //
    // Declared before the run and destroyed after `result` is printed, so the
    // printer thread outlives every write to it.
    satellite::Console console;
    satellite::InterpResult result = satellite::run_file(path, args, &console);

    // The displayed output already went through the Console; run_file drained
    // it before returning, so what is left here is the error report alone.
    fwrite(result.output.data(), 1, result.output.size(), stdout);
    fflush(stdout);
    return result.status;
}

// `run <file> [args]` at the prompt is `satl --run <file> [args]` at the
// shell -- same entry point, same argz, same errors. It exists because the
// window has no shell behind it: without this, a file could only be run by
// closing the window and going back to a terminal.
static void run_command(const satellite::RunCommand &command,
                        satellite::Console *console)
{
    if (!command.error.empty()) {
        fputs(command.error.c_str(), stdout);
        fflush(stdout);
        return;
    }

    // The PROMPT's Console, not one of its own, and that is what makes a pace
    // set at the prompt apply to the program the prompt runs: the Evaluator is
    // rebuilt per line and per run, and the printer is the one thing that
    // outlives both. It also means the file's output streams as it runs, the
    // same way `satl --run` has always behaved.
    satellite::InterpResult result =
        satellite::run_file(command.path, command.args, console);
    fwrite(result.output.data(), 1, result.output.size(), stdout);

    // A REPL has no exit status to carry a failure out to, so it says the
    // status out loud. The shell form gets this for free from the process.
    if (result.status != 0)
        printf("satellite: %s exited with status %d\n", command.path.c_str(),
               result.status);
    fflush(stdout);
}

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
static bool exit_command(const std::string &line, int &status)
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
static void abandon_block(std::string &block, int &depth)
{
    const long lines = std::count(block.begin(), block.end(), '\n');
    printf("satellite: multi-line entry abandoned, %ld line%s discarded\n",
           lines, lines == 1 ? "" : "s");
    fflush(stdout);
    block.clear();
    depth = 0;
}

static int run_repl()
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
           "poke satellite.library; shuts down if free memory < "
           "system.min_free_mb)\n",
           satellite::version_line().c_str());
    // Said out loud because there is no shell behind this prompt and no `cd`
    // to fall back on: a name nobody has read is a name nobody can guess.
    printf("type  help  for the whole language on one screen\n");
    printf("change directory: satellite.directory.change(\"some_dir\")   "
           "where you are: satellite.directory.current()\n");

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
            for (int i = 0; i < depth; i++)
                printf("    ");
        } else {
            // linux_username, cwd: type_here
            printf("%s%s%s, %s%s%s: ", color_user, satellite::username().c_str(),
                   color_off, color_cwd, satellite::cwd().c_str(), color_off);
        }
        fflush(stdout);

        int c;
        line.clear();
        while ((c = getchar()) != EOF && c != '\n')
            line += static_cast<char>(c);

        if (c == EOF) {         // Ctrl-D
            printf("\n");
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
            debug_command(line);
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

// satl --where — the resolved library directory and which of the three
// candidates in system.cpp produced it.
//
// It exists to make an install falsifiable. `make install` can then be checked
// without running a program, and someone whose install is broken has one
// command to run instead of a guess to make about which tier answered: the
// difference between "found it next to the binary" and "fell through to the
// compiled-in default" is the whole diagnosis, and the path alone does not say
// which happened.
static int where()
{
    satellite::LibraryPathSource source = satellite::LibraryPathSource::None;
    std::string path = satellite::library_path(&source);

    const char *from = "";
    switch (source) {
    case satellite::LibraryPathSource::Environment:
        from = "$SATELLITE_PATH (development override)";
        break;
    case satellite::LibraryPathSource::Relative:
        from = "alongside the binary (relocatable install)";
        break;
    case satellite::LibraryPathSource::Compiled:
        from = "compiled-in SATELLITE_LIB_DIR (packaged install)";
        break;
    case satellite::LibraryPathSource::None:
        from = "nothing — no candidate exists";
        break;
    }

    printf("library: %s\nfrom:    %s\n",
           path.empty() ? "(not found)" : path.c_str(), from);
    return 0;
}

static void usage()
{
    fprintf(stderr,
            "usage: satl                             repl on stdin/stdout\n"
            "       satl --repl                      the same, spelled out\n"
            "       satl --run <file> [args]         run a file on stdout\n"
            "       satl <file> [args]               same as --run\n"
            "       satl --where                     resolved library path\n"
            "\n"
            "at the prompt: run <file> [args]  (also spelled interpret, --run)\n"
            "the gui terminal is a separate binary: satl-term\n"
            "  satl --version            what this build is, and what built it\n");
}

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() > 1 && args[1] == "--repl")
        return run_repl();
    if (args.size() > 1 && args[1] == "--where")
        return where();
    // Before --help and before the bare-filename arm, so a file that happens
    // to be called --version cannot shadow the flag. Exit 0: asking a program
    // what it is, is not an error, and a packaging script that greps this is
    // entitled to a zero.
    if (args.size() > 1 && (args[1] == "--version" || args[1] == "-V")) {
        fputs(satellite::version_text("satl").c_str(), stdout);
        return 0;
    }
    if (args.size() > 1 && (args[1] == "-h" || args[1] == "--help")) {
        usage();
        return 2;
    }

    std::string file;
    std::vector<std::string> rest;

    if (args.size() > 1 && args[1] == "--run") {
        if (args.size() < 3) {
            usage();
            return 2;
        }
        file = args[2];
        rest.assign(args.begin() + 3, args.end());
    } else if (args.size() > 1 && !args[1].empty() && args[1][0] != '-') {
        file = args[1];
        rest.assign(args.begin() + 2, args.end());
    } else if (args.size() > 1) {
        fprintf(stderr, "satellite: unknown option %s\n", args[1].c_str());
        usage();
        return 2;
    }

    // No file means the repl. It used to mean the window, which is now a
    // separate binary that this one knows nothing about.
    if (file.empty())
        return run_repl();

    return run_file_mode(file, rest);
}
