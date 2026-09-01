// The opening information and the usage text. See programs/opening.hpp.
//
// The text lives in a .cpp rather than the header because it is the one part of
// this pair that will be edited every milestone, and a header edited every
// milestone recompiles everything that includes it. The declarations next door
// will not move for a long time.

#include "programs/opening.hpp"

#include "system_facts/version.hpp"

namespace satellite {

// The banner. Three things, in the order somebody starting satl for the first
// time needs them: what this is, how to run a file, and where to find the rest.
//
// HOW TO RUN A FILE IS THE FIRST LINE OF THE BODY, and it is spelled with a
// real filename rather than a metavariable. `satl <file>` is correct and
// `satl filename.satl` is copyable, and the second is what somebody staring at
// a fresh prompt actually needs. The angle brackets belong in usage_text(),
// where the reader has already decided to read a specification.
//
// The last paragraph is TEMPORARY and goes away at M10, when satl can run a
// file. It is here because a program that prints "satl filename.satl" and then
// refuses to do it has told the user something false, and the fix is to say so
// on the way in rather than only on the way out.
std::string opening_text()
{
    return "satellite " + version_line() + "\n"
           "\n"
           "    satl filename.satl     run a file\n"
           "    satl --help            every way to start it\n"
           "    satl --version         what this build is, and what built it\n"
           "\n"
           "This build does not interpret anything yet. It is milestone 1 of\n"
           "PLAN_ONE.md -- the binary, the build, and the way in. Running a\n"
           "file lands at M10.\n";
}

// Every way to start satl.
//
// The right column says what the left column DOES, not what it is called. A
// usage line that reads "--version    the version" has spent a line of the
// user's attention restating the flag.
//
// NO MEASUREMENTS IN HERE. An earlier draft explained the satl / satl-term
// split with the shared-object count and the millisecond figures behind it,
// which are true and are not what somebody reading a usage list wants. The
// numbers live in make_support/040-sources.mk, beside the decision they
// justify. A usage list says what the commands are.
std::string usage_text()
{
    return "usage: satl                       this opening information\n"
           "       satl <file> [args]         run a file            (M10)\n"
           "       satl --run <file> [args]   the same, spelled out (M10)\n"
           "       satl --repl                the prompt            (M22)\n"
           "       satl --words               every path the language has, and\n"
           "                                  the number it is\n"
           "       satl --words <path>        just that one, or why it is not\n"
           "                                  a path\n"
           "       satl --tokens <file>       what the lexer makes of a file\n"
           "       satl --unparse <file>      the same file, printed back out\n"
           "                                  of the parsed tree\n"
           "       satl --satc <file>         the same file as a .satc: its\n"
           "                                  words as their numbers, read\n"
           "                                  from the cache or written to it\n"
           "       satl --check <file>        say everything wrong with a file\n"
           "                                  and print nothing else\n"
           "       satl --resolve <file>      every capsule's frame, every name\n"
           "                                  in its slot, and every path in\n"
           "                                  the file beside its number\n"
           "       satl --compile <file>      the closure tree the evaluator\n"
           "                                  runs: every op, every frame, and\n"
           "                                  every part not built yet\n"
           "       satl --call <file> <name> [n...]\n"
           "                                  run one capsule and print what it\n"
           "                                  answered\n"
           "       satl --number <a> <op> <b> exact arbitrary-precision\n"
           "                                  arithmetic: two numbers and one\n"
           "                                  of + - * / %\n"
           "       satl --errors              every code satl can report, and\n"
           "                                  what each one says\n"
           "       satl --errors <code>       just that one\n"
           "       satl --limits              every machine limit satl is\n"
           "                                  holding to, and where each one\n"
           "                                  came from\n"
           "       satl --limits <file>       the same, from that\n"
           "                                  satellite_config.ini instead of\n"
           "                                  the one beside the binary\n"
           "       satl --watchdog [file]     hold open and let the memory\n"
           "                                  watchdog work -- it stops satl\n"
           "                                  when MEMORY_MAX is crossed\n"
           "       satl --version, -V         what this build is, and what\n"
           "                                  built it\n"
           "       satl --help, -h            this list\n"
           "       satl --no-window <...>     do not open a window when there\n"
           "                                  is no console to print to\n"
           "\n"
           "The gui terminal is a separate binary: satl-term, so that this one\n"
           "starts without a window toolkit behind it.\n"
           "\n"
           "STARTED WITHOUT A CONSOLE -- from a file manager, a desktop menu or\n"
           "a .satl file association -- satl hands itself to satl-term, because\n"
           "everything it prints would otherwise go where nobody is looking. It\n"
           "does NOT do that in a terminal, in a pipeline, with output\n"
           "redirected, or with no display; --no-window and SATL_NO_WINDOW=1\n"
           "turn it off outright.\n"
           "\n"
           "Milestones in brackets have not landed. satl says so when asked to\n"
           "do one rather than failing as though the command were wrong.\n"
           "\n"
           "satl reads a satellite_config.ini beside its own binary, if there\n"
           "is one, and holds itself to what it says. --limits prints what it\n"
           "settled on. A malformed one is refused rather than ignored, which\n"
           "is why it stops every command and not only these two.\n"
           "\n"
           "satl exits 0 when what was asked for happened, 1 when a file it was\n"
           "given is not what it has to be, 2 when this command line is not one\n"
           "it has, 3 when the request is right and the milestone has not\n"
           "landed, and 4 when satl stopped itself at a machine limit.\n";
}

} // namespace satellite
