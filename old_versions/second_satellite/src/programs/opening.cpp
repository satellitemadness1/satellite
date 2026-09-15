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
// THE LAST PARAGRAPH WENT AWAY AT M10, WHICH IS WHAT IT WAS THERE FOR. From M1
// to M10 it said "This build does not interpret anything yet ... Running a file
// lands at M10", because a program that prints "satl filename.satl" and then
// refuses to do it has told the user something false. satl runs a file now, so
// the honest thing to say is what it can and cannot do with one -- and that
// sentence is a different sentence every milestone until M28, which is why it
// names the two commands that answer the question rather than trying to hold a
// list.
std::string opening_text()
{
    return "satellite " + version_line() + "\n"
           "\n"
           "    satl filename.satl     run a file\n"
           "    satl --help            every way to start it\n"
           "    satl --version         what this build is, and what built it\n"
           "\n"
           "A program starts at satellite.main() and prints through\n"
           "satellite.console.display. Much of the language does not run yet:\n"
           "satl --compile filename.satl lists every part of a file it cannot\n"
           "do and names the milestone that will, and satl --errors explains\n"
           "any code it reports.\n";
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
           "       satl <file> [args]         run a file: satellite.main()\n"
           "       satl --run <file> [args]   the same, spelled out\n"
           "       satl --repl                the prompt: type satellite at\n"
           "                                  it, `run <file>` to run one,\n"
           "                                  `exit` to leave\n"
           "       satl --words               every path the language has, and\n"
           "                                  the number it is\n"
           "       satl --words <path>        just that one -- its number, its\n"
           "                                  children and the next number\n"
           "                                  free under it -- or why it is not\n"
           "                                  a path\n"
           "       satl --tokens <file>       what the lexer makes of a file\n"
           "       satl --unparse <file>      the same file, printed back out\n"
           "                                  of the parsed tree\n"
           "       satl --satc <file>         the same file as a .satc: its\n"
           "                                  words as their numbers, read\n"
           "                                  from the cache or written to it\n"
           "       satl --check <file>        say everything wrong with a file\n"
           "                                  without running it, and print\n"
           "                                  nothing else\n"
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
           "landed, 4 when satl stopped itself at a machine limit, and 130 --\n"
           "the shell's own 128 + SIGINT -- when Ctrl-C stopped a running\n"
           "program at a statement boundary.\n"
           "\n"
           "A program's own exit status is one of those five and not something\n"
           "it returns: satellite.return(satellite) says it succeeded, and satl\n"
           "exits 0 because the run finished rather than because of the value.\n"
           "[args] after the file are not readable from a program until M20.\n";
}

} // namespace satellite
