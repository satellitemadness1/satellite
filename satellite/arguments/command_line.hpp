#pragma once
// satl's command line (PLAN M0.5; the author delegated it, 2026-09-15: "`satl
// --run <file> [args...]` and `satl --repl` look good").
//
//     satl                                   the opening lines; exit 0
//     satl --version | -V                    the title lines; exit 0
//     satl --help | -h                       the title lines and every way to start; exit 0
//     satl [--debug] --run <file> [words...] run <file>
//     satl [--debug] <file> [words...]       the same, when <file> does not begin with -
//     satl [--debug] --repl                  the prompt (M0.6); until then 14 not_built_yet
//     satl --rebuild                         compose every setting into one binary and save it
//     satl --config [most]                   measure what this machine can do, once
//
// --debug IS THE ONLY OPTION AND IT COMES BEFORE THE COMMAND WORD. --version and
// --help are the whole command line. After the file EVERY word is the program's,
// --version, --debug and "" included, kept in order. --run takes the next word
// as the file whatever it is, so `satl --run -x.satl` runs -x.satl and satl needs
// no `--`. Anything else is refused by name with command_line_not_understood.
//
// TWO READINGS OF MINE, EACH ONE LINE TO REVERSE:
//   - `satl --debug` with nothing after it is bare `satl`: nothing to do is not an
//     error (003 main.cpp), and --debug has nothing to show.
//   - `satl --debug --debug x.satl` is --debug once. Saying it twice asks for the
//     same thing, so it is not refused.

#include <string>
#include <vector>

namespace satellite004 {

// APPENDED, NEVER INSERTED -- `rebuild` is 2026-09-18's and goes on the end for
// the same reason a word code does: nothing here should renumber when one is added.
enum class Command { opening, version, help, run, repl, rebuild, config };

struct CommandLine {
    Command command = Command::opening;
    bool debug = false;
    std::string file;                  // Command::run only
    std::vector<std::string> words;    // Command::run only: the program's own, in order

    // Command::config only: the most threads to probe, or 0 for the machine's
    // own ceiling less headroom. See run_config.hpp for why the cap exists.
    unsigned long long int most = 0;
};

// Answers success, or command_line_not_understood once it has said why on stderr.
signed long long int read_command_line(int argc, char **argv, CommandLine &into);

// What --help prints below the start-up block, and bare `satl` the first lines of.
std::string usage_lines();
std::string opening_lines();

} // namespace satellite004
