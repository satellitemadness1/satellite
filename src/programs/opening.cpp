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
// The last paragraph is TEMPORARY and goes away at M8, when satl can run a
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
           "file lands at M8.\n";
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
           "       satl <file> [args]         run a file            (M8)\n"
           "       satl --run <file> [args]   the same, spelled out (M8)\n"
           "       satl --repl                the prompt            (M11)\n"
           "       satl --version, -V         what this build is, and what\n"
           "                                  built it\n"
           "       satl --help, -h            this list\n"
           "\n"
           "The gui terminal is a separate binary: satl-term, so that this one\n"
           "starts without a window toolkit behind it.\n"
           "\n"
           "Milestones in brackets have not landed. satl says so when asked to\n"
           "do one rather than failing as though the command were wrong.\n";
}

} // namespace satellite
