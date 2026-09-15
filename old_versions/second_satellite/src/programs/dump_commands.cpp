// The two registry dumps. See programs/dump_commands.hpp for what a registry
// dump is and why it is not a file arm.
//
// MOVED OUT OF main.cpp AT M7 AND OTHERWISE UNCHANGED, comments included --
// every paragraph below was written by the milestone that added the arm, and
// rewriting them while moving them would have thrown four milestones' reasons
// away in a commit about line count.

#include "programs/dump_commands.hpp"

#include "error_reporter/dump.hpp"
#include "machine_limits/dump.hpp"
#include "programs/opening.hpp"
#include "satellite_words/dump.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>

namespace satellite {

// THE REGISTRY'S CONSUMER, AND THE REASON M2 HAS ONE. PLAN M2 asks for this by
// name because the first satellite shipped three commits where its word
// registry had no reader at all, and four defects accumulated behind a
// guarantee nothing was checking.
//
// NOT AN ARM THAT SAYS "not built yet", which every other unfinished thing does.
// The numbering IS built, so this answers -- and what it prints ends by saying
// that almost nothing it lists runs yet, because a dump of 254 paths with no
// such line would read as a feature list.
int words_command(const std::string &key)
{
    if (key.empty()) {
        fputs(words::dump_text().c_str(), stdout);
        // M6's DONE-WHEN, PRINTED HERE RATHER THAN IN
        // satellite_words/dump.cpp. 040-sources.mk keeps that module cheap to
        // link -- "a future .satc reader or disassembler can read the numbering
        // without linking anything" -- and making the registry's printer depend
        // on a thread pool would spend that property on a sentence.
        fputs(limits::walk_note_text(words::kNodeCount).c_str(), stdout);
        return EXIT_FINE;
    }

    // A path that resolves is an answer and goes to stdout; a path the language
    // does not have is a command line that named something satl cannot do, so it
    // goes to stderr with the same status a bad option gets. That is the split
    // the --help arm already makes, applied to an operand instead of to a flag.
    bool resolved = false;
    const std::string report = words::walk_text(key, resolved);
    fputs(report.c_str(), resolved ? stdout : stderr);
    return resolved ? EXIT_FINE : EXIT_USAGE;
}

// THE CODE REGISTRY'S CONSUMER, and it is `--words` one registry later. A code
// exists so that somebody can look it up, so a code registry with no way to look
// a code up is not a smaller version of the feature -- it is none of it.
int errors_command(const std::string &key)
{
    if (key.empty()) {
        fputs(errors::dump_text().c_str(), stdout);
        return EXIT_FINE;
    }

    // A code that resolves is an answer and goes to stdout; a code satl does not
    // have is a command line naming something satl cannot do, so it goes to
    // stderr with the same status a bad option gets. That is exactly the split
    // `satl --words <path>` makes, applied one registry on.
    bool known = false;
    const std::string report = errors::explain_text(key, known);
    fputs(report.c_str(), known ? stdout : stderr);
    return known ? EXIT_FINE : EXIT_USAGE;
}

} // namespace satellite
