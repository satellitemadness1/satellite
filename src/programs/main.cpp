// satl -- the interpreter.
//
// THIS BINARY LINKS NO GUI. The window lives in satl-term (M11.A, built), and
// satellite.window.new() will reach a dlopen'd library (M13). The split is
// measured rather than tidy-minded: `ldd` on this satl lists 6 shared objects
// and on the first satellite's satl-term lists 79, and the dynamic linker loads
// every one of them before main() on every run. The first satellite measured
// what that costs at 25.9 ms with the link against 2.5 ms without.
//
// See make_support/040-sources.mk for this build's own startup numbers and for
// why the "119 shared objects" its predecessor's source claims is not repeated
// here.
//
// At milestone 1 there is no interpreter behind any of this. What there is: a
// binary that says what it is, says how to run a file, and refuses to pretend
// about the parts that have not been built. See PLAN_ONE.md, M1.

#include "programs/opening.hpp"
#include "programs/window_handover.hpp"
#include "satellite_words/dump.hpp"
#include "system_facts/version.hpp"

#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include <unistd.h>

namespace {

// Whether a path names something this process can read.
//
// R_OK and not F_OK, because "it is there" and "I may open it" are different
// answers and only the second one is useful to somebody about to be told their
// file cannot be run. access() rather than a stat of the mode bits, because
// access() asks the kernel the question with THIS process's real ids instead of
// reconstructing the answer from permissions and getting it wrong on an ACL.
bool readable(const std::string &path)
{
    return access(path.c_str(), R_OK) == 0;
}

// What satl says when the command line was right and the milestone is not here.
//
// SEPARATE FROM A USAGE ERROR, and that separation is the whole point of the
// function. `satl hello.satl` is a correct sentence; answering it with a usage
// dump teaches the user that they typed something wrong, which is false and
// sends them to reread a specification that already agrees with them.
//
// The file is checked even though nothing will be run with it, because the two
// failures a user is about to have are different and they should not have to
// guess which one they are in. A misspelled path reported as "not built yet"
// is a bug report waiting to be filed at M8.
int not_yet(const std::string &what, const std::string &file,
            const char *milestone)
{
    fprintf(stderr, "satl: %s is not built yet -- it lands at %s.\n",
            what.c_str(), milestone);

    if (!file.empty()) {
        if (readable(file))
            fprintf(stderr, "      %s was found and nothing was done with it.\n",
                    file.c_str());
        else
            fprintf(stderr, "      %s could not be read either, so check the "
                            "path before %s arrives.\n",
                    file.c_str(), milestone);
    }
    return satellite::EXIT_NOT_YET;
}

// A usage failure: the command line did not name something satl can do.
//
// Usage goes to STDERR here and to stdout in the --help arm, and that is not an
// inconsistency. `satl --help | less` is someone reading the list on purpose
// and it belongs on stdout; a complaint about a bad command line belongs on
// stderr, where it survives a pipe that was set up for output that will now
// never come.
int usage_error(const std::string &complaint)
{
    fprintf(stderr, "satl: %s\n", complaint.c_str());
    fputs(satellite::usage_text().c_str(), stderr);
    return satellite::EXIT_USAGE;
}

} // namespace

int main(int argc, char **argv)
{
    // BEFORE ANY ARGUMENT IS READ, AND THAT IS THE POINT. If this process was
    // started with no console -- a file manager, a .desktop entry, a desktop
    // menu -- then every line below prints into nothing, INCLUDING the usage
    // text and the error messages. Handing over first means the person sees
    // whatever satl was going to say, rather than seeing the window only in the
    // cases somebody remembered to route through it.
    //
    // It returns here on a terminal, in a pipeline, with output redirected,
    // with no display, without a satl-term to hand to, or when SATL_NO_WINDOW
    // is set -- six refusals, all named in window_handover.cpp. --no-window is
    // the seventh and is handled below, because it is a flag and flags are this
    // function's business.
    if (argc < 2 || std::string(argv[1]) != "--no-window")
        satellite::hand_over_to_the_window(argv);

    // --no-window IS CONSUMED HERE AND EXISTS NOWHERE BELOW. Filtered out of the
    // arguments rather than skipped over at each use: the arms further down
    // index args[2] for their operands, so carrying the flag would make every
    // one of them off by one in exactly the case nobody tests. `satl
    // --no-window` alone is therefore `satl` alone, which is what it reads as.
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int i = 0; i < argc; ++i) {
        if (i == 1 && std::string(argv[i]) == "--no-window")
            continue;
        args.emplace_back(argv[i]);
    }

    // NOTHING TO DO IS NOT AN ERROR. satl started with no arguments shows the
    // opening information, which is what says how to run a file.
    //
    // At M11.B this arm gains the prompt, and the banner it prints first is this
    // same opening_text() -- which is why that function returns a string rather
    // than printing one.
    if (args.size() == 1) {
        fputs(satellite::opening_text().c_str(), stdout);
        return satellite::EXIT_FINE;
    }

    const std::string &first = args[1];

    // BEFORE the bare-filename arm, so that a file which happens to be called
    // --version cannot shadow the flag. Exit 0: asking a program what it is, is
    // not an error, and a packaging script that greps this is entitled to a
    // zero.
    if (first == "--version" || first == "-V") {
        fputs(satellite::version_text("satl").c_str(), stdout);
        return satellite::EXIT_FINE;
    }

    // Also before the bare-filename arm, and also exit 0. The first satellite
    // exited 2 here; see the note on ExitStatus for why that changed.
    if (first == "-h" || first == "--help") {
        fputs(satellite::usage_text().c_str(), stdout);
        return satellite::EXIT_FINE;
    }

    // THE REGISTRY'S CONSUMER, AND THE REASON M2 HAS ONE. PLAN M2 asks for
    // this by name because the first satellite shipped three commits where its
    // word registry had no reader at all, and four defects accumulated behind a
    // guarantee nothing was checking.
    //
    // NOT AN ARM THAT SAYS "not built yet", which every other unfinished thing
    // here does. The numbering IS built, so this answers -- and what it prints
    // ends by saying that almost nothing it lists runs yet, because a dump of
    // 254 paths with no such line would read as a feature list.
    if (first == "--words") {
        if (args.size() < 3) {
            fputs(satellite::words::dump_text().c_str(), stdout);
            return satellite::EXIT_FINE;
        }
        // A path that resolves is an answer and goes to stdout; a path the
        // language does not have is a command line that named something satl
        // cannot do, so it goes to stderr with the same status a bad option
        // gets. That is the split the --help arm already makes, applied to an
        // operand instead of to a flag.
        bool resolved = false;
        const std::string report = satellite::words::walk_text(args[2], resolved);
        fputs(report.c_str(), resolved ? stdout : stderr);
        return resolved ? satellite::EXIT_FINE : satellite::EXIT_USAGE;
    }

    if (first == "--repl")
        return not_yet("the prompt", std::string(), "M11.B");

    // --run takes an operand, so a missing one is a real usage error rather
    // than a milestone that has not landed: `satl --run` with nothing after it
    // is wrong at M8 too.
    if (first == "--run") {
        if (args.size() < 3)
            return usage_error("--run needs a file after it");
        return not_yet("running a file", args[2], "M8");
    }

    // A bare word that is not a flag is a filename. Checked LAST of the arms
    // that can match a word, which is what the ordering above is for.
    if (!first.empty() && first[0] != '-')
        return not_yet("running a file", first, "M8");

    return usage_error("unknown option " + first);
}
