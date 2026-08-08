// satl — the interpreter.
//
// This binary links NO GUI: the window lives in satl-term, which spawns
// this one into a PTY. The split is measured, not tidy-minded — linking gtk4
// and vte here pulled 119 shared objects that the dynamic linker loaded before
// main() on every `--run`, costing 23.4 ms against an interpreter whose own
// share of hello world is 0.3 ms. See window.cpp.

#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

#include "interp.hpp"
#include "library.hpp"
#include "satellite_string.hpp"
#include "system.hpp"

// ---------------------------------------------------------------------------
// REPL side (runs inside the terminal)
// ---------------------------------------------------------------------------

// Prompt colors, matching the window's palette (see apply_colors): the
// username white, the path black, both against the light blue field.
//
// 24-bit SGR rather than the classic 30-37 range, because those eight indices
// are whatever palette VTE happens to ship -- its "white" is a light grey and
// its bold black is dark grey, so neither would be the color asked for.
static const char *const COLOR_USER = "\033[1;38;2;255;255;255m";
static const char *const COLOR_CWD  = "\033[1;38;2;0;0;0m";
static const char *const COLOR_OFF  = "\033[0m";

static void evaluate(const std::string &line)
{
    std::string out = satellite::eval_line(line);
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
    satellite::InterpResult result = satellite::run_file(path, args);
    fwrite(result.output.data(), 1, result.output.size(), stdout);
    fflush(stdout);
    return result.status;
}

// `run <file> [args]` at the prompt is `satl --run <file> [args]` at the
// shell -- same entry point, same argz, same errors. It exists because the
// window has no shell behind it: without this, a file could only be run by
// closing the window and going back to a terminal.
static void run_command(const satellite::RunCommand &command)
{
    if (!command.error.empty()) {
        fputs(command.error.c_str(), stdout);
        fflush(stdout);
        return;
    }

    satellite::InterpResult result =
        satellite::run_file(command.path, command.args);
    fwrite(result.output.data(), 1, result.output.size(), stdout);

    // A REPL has no exit status to carry a failure out to, so it says the
    // status out loud. The shell form gets this for free from the process.
    if (result.status != 0)
        printf("satellite: %s exited with status %d\n", command.path.c_str(),
               result.status);
    fflush(stdout);
}

static int run_repl()
{
    start_runtime();

    printf("satellite 0.1  (run <file> interprets a file; :set / :get / :vars "
           "poke satellite.library; shuts down if free memory < "
           "system.min_free_mb)\n");
    // Said out loud because there is no shell behind this prompt and no `cd`
    // to fall back on: a name nobody has read is a name nobody can guess.
    printf("type  help  for the whole language on one screen\n");
    printf("change directory: satellite.directory.change(\"some_dir\")   "
           "where you are: satellite.directory.current()\n");

    std::string line;
    for (;;) {
        // linux_username, cwd: type_here
        printf("%s%s%s, %s%s%s: ", COLOR_USER, satellite::username().c_str(),
               COLOR_OFF, COLOR_CWD, satellite::cwd().c_str(), COLOR_OFF);
        fflush(stdout);

        int c;
        line.clear();
        while ((c = getchar()) != EOF && c != '\n')
            line += static_cast<char>(c);

        if (c == EOF) {         // Ctrl-D
            printf("\n");
            return 0;
        }
        if (line == "exit" || line == "quit")
            return 0;
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
            evaluate("satellite.help");
            continue;
        }

        // Checked before evaluation: `run` is not satellite syntax (every name
        // in the language is satellite.-prefixed), so nothing legal is shadowed.
        if (satellite::RunCommand command = satellite::parse_run_command(line);
            command.matched) {
            run_command(command);
            continue;
        }

        evaluate(line);
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
            "the gui terminal is a separate binary: satl-term\n");
}

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() > 1 && args[1] == "--repl")
        return run_repl();
    if (args.size() > 1 && args[1] == "--where")
        return where();
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
