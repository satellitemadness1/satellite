#pragma once

#include "satellite_value/value.hpp"

#include <string>
#include <vector>

// The whole pipeline behind one call: source in, displayable text out.
//
// This exists so every test from here on drives lexer -> parser -> AST -> eval
// -> satellite.library -> to_string without linking gtk4 or vte (DESIGN.md
// §11). The GUI is a front-end for this, not the other way round.

namespace satellite {

// Only ever held as a pointer here — see console.hpp for what it is and why
// the run modes differ about wanting one.
class Console;

struct InterpResult {
    // Everything the program displayed, followed by any error report. Errors
    // are text rather than a separate channel because that is what both the
    // REPL and a test want to look at.
    //
    // When a Console was supplied, the DISPLAYED part is not here — it has
    // already gone to stdout through the Console's printer thread, and this
    // holds the error report alone. The run drains the Console before
    // returning, so a caller that prints this last still gets the two in the
    // order they were written.
    std::string output;
    bool ok = false;

    // Process exit status. satellite.return(satellite) is success, so the
    // runtime singleton maps to 0; satellite.return(<number>) maps to that
    // number so a script can fail a shell pipeline deliberately.
    int status = 0;
};

// Command-line arguments as a satellite.container.list<satellite.variable.string>.
//
// Every argument is converted with encode_raw and NEVER with encode. An
// argument arrives already escaped by the shell, so running the backslash-name
// expansion over it a second time corrupts it: encode() turns --path=C:\home
// into --path=C:/home/madness, silently and with no error. That is exactly the
// defect §3.3 fixed for source text, and argv is the other place it bites.
List args_to_list(const std::vector<std::string> &args);

// Never throws. A syntax error is reported and nothing runs; a runtime error
// is reported after whatever output preceded it.
//
// `console` has the same meaning it has on run_program below, and the REPL is
// what wants one here: a Console outlives the single line this runs, so a
// setting that belongs to the printer — satellite.console.display(100ms) —
// stays set from one prompt to the next, while the Evaluator that received it
// does not survive the line at all.
InterpResult run_source(const std::string &source,
                        const std::string &ns = "main", bool echo = false,
                        Console *console = nullptr);

// Runs a whole program: top-level statements first, then satellite.main if the
// program defines one, with `args` bound to its parameter.
//
// `path` names the source in error messages, so a failure reads
// "hello.satl:3" rather than "line 3". Empty means the source came from no
// file — a REPL line, or a test — and the messages fall back to the bare line
// number, which is the truth in that case rather than a degradation.
//
// `console`, when given, streams displayed output to its printer thread as the
// program runs instead of accumulating all of it to be printed at the end. It
// must outlive the call. Null keeps the accumulating behaviour, which is what
// every test and the REPL want: they read the text back rather than watch it.
InterpResult run_program(const std::string &source,
                         const std::vector<std::string> &args = {},
                         const std::string &path = {},
                         Console *console = nullptr);

// Same, reading the source from a file. `path` becomes args[0], mirroring
// argv[0] — so a script sees its own name first, as it would in C.
InterpResult run_file(const std::string &path,
                      const std::vector<std::string> &args = {},
                      Console *console = nullptr);

// One REPL line, with expression echo on, so `x` renders as its value.
//
// With a Console the displayed text and the echo have already gone to stdout by
// the time this returns — drained, so nothing a caller prints afterwards can
// overtake them — and what comes back is the error report alone. Without one it
// is all of it, which is what every test that reads a line's output back needs.
std::string eval_line(const std::string &line, Console *console = nullptr);

// A REPL line that asks for a file to be run: `run <file> [args]`. The verb is
// also spelled `interpret` and `--run`, so whichever form a user already knows
// -- the shell's `satl --run f.satl`, or the word for what it does --
// works at the prompt too.
//
// Words split on whitespace, with '...' or "..." grouping a word that contains
// spaces. A backslash is NOT an escape here: what follows the verb is a path,
// and C:\home has to stay C:\home. That is the same rule args_to_list applies
// to argv, for the same reason.
struct RunCommand {
    // The line named the verb. False means it is ordinary satellite source and
    // the REPL should evaluate it; every other field is then meaningless.
    bool matched = false;

    std::string path;
    std::vector<std::string> args;   // everything after the file

    // Ready-to-print complaint when the verb was named but the rest of the line
    // is unusable (no file, unbalanced quote). Non-empty means do not run.
    std::string error;
};

RunCommand parse_run_command(const std::string &line);

} // namespace satellite
