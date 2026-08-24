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

// Starting the runtime, and the two ways a program is run: a file named on
// the command line, and a `run` typed at the prompt.
// Out of main.cpp, 2026-08-24. See programs/main_internal.hpp.

// The shutdown threshold is an ordinary satellite.library variable, so
// :set system.min_free_mb <mb> retunes the guard live.
void start_runtime()
{
    satellite::Library::instance().set("system", "min_free_mb", 4096);
    satellite::start_memory_watchdog();
}

// satl --run <file> [args...] — headless, straight to stdout. This is
// also what makes the interpreter testable without a display server.
int run_file_mode(const std::string &path,
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
void run_command(const satellite::RunCommand &command,
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
