// `satl --check <file>` -- every pass, and then the fifth. See
// programs/check_command.hpp. The two shared helpers moved to
// programs/source_report.cpp at M26.5, so that a binary wanting open_source()
// no longer links an arm that needs the whole language behind it.

#include "programs/check_command.hpp"

#include "error_reporter/report.hpp"
#include "program_diagnostics/diagnose.hpp"
#include "programs/built_program.hpp"
#include "programs/opening.hpp"

#include <string>
#include <vector>

namespace satellite {

// ALL FOUR PASSES SINCE M26.5, AND IT USED TO BE ONE. This arm parsed and
// stopped, which made it the weakest checker in a tree that had three stronger
// ones -- `--resolve` ran the name pass, a run ran the compiler, and only the
// run ever reached the thing most worth being told early.
//
// THE FAILURE THAT PAID FOR THE CHANGE was a 2,474-line program whose build
// phase takes seven minutes. `machine_gb.round().to_string()` passed `--check`,
// passed `--resolve`, and died at run time after the seven minutes -- three
// times in one afternoon. Every pass of the four already knew; nothing asked
// the later ones.
//
// SO THE RULE IS NOW THE SIMPLE ONE: everything satellite can find out about a
// program without running it, this arm finds out. The order is still
// built_program.hpp's and still matters -- nothing resolves from a tree that did
// not parse, nothing compiles from a tree that did not resolve -- which is why
// this calls that function rather than keeping a second copy of the order.
//
// AND `--resolve` IS NOT THIS AND IS NOT GONE. That arm PRINTS THE FRAMES, the
// way `--tokens` prints tokens and `--unparse` prints a program; it is a viewer
// and this is the checker. What moved here is the checking, which is the thing
// there should only be one of.
int check_command(const std::string &path)
{
    Built built;
    const bool whole = build_program(path, built);

    // THE FILE ITSELF IS THE OTHER FAILURE, and it is EXIT_USAGE rather than
    // EXIT_MALFORMED -- built_program.hpp's `opened` exists for this choice and
    // says so. A path that is not there is a mistake on the command line; a
    // file that will not parse is a mistake in a program.
    if (!built.opened)
        return EXIT_USAGE;

    // WHAT THE COMPILER KNEW AND NOBODY ASKED -- evaluate.hpp's `deferred`.
    // These are printed HERE and by nothing else: a run still reaches them or
    // does not, which is errors.def's S0720 note kept exactly as it was written.
    // Reported after the four passes because they are the last thing found and
    // the least urgent -- a program with a name it cannot bind has a worse
    // problem than a program using a piece of grammar M25 will finish.
    if (!built.program.deferred.empty())
        report(path, built.text, built.program.deferred);

    // THE FIFTH PASS, AND IT IS ASKED ONLY WHEN THERE IS A PROGRAM TO ASK
    // ABOUT. program_diagnostics/diagnose.hpp is not on the road to running
    // anything -- it is what a person would find by reading the file -- and it
    // reads RESOLVED types and COMPILED ops, so a tree that did not get that
    // far has nothing for it to read. Asking anyway would report a ring among
    // suits whose fields never bound, which is a sentence about the analyser's
    // own confusion rather than about the program.
    //
    // AND A FINDING HERE DOES NOT NECESSARILY MEAN NO. Most of what it answers
    // is SAT_WARNING: a ring of spacesuits leaks, and is also a thing somebody
    // may have built on purpose and reasoned about. codes.hpp's
    // `stops_the_work()` is what decides, rather than a comparison between
    // severities -- there is no order between them to compare.
    bool stopped = false;
    if (whole) {
        const std::vector<errors::Diagnostic> found = diagnostics::diagnose(
            built.parsed.ast, built.resolved, built.program);
        if (!found.empty())
            report(path, built.text, found);
        for (const errors::Diagnostic &one : found)
            if (errors::stops_the_work(errors::severity_of(one.code)))
                stopped = true;
    }

    // SILENCE STILL MEANS YES, AND THE STATUS IS STILL THE ANSWER. Both are the
    // contract this header opens with and neither changed: nothing reaches
    // stdout on any path, so a script may still use the exit code alone, and a
    // person who wants to SEE the file understood has `--tokens`, `--unparse`,
    // `--resolve` and `--satc` for that.
    //
    // A DEFERRED REFUSAL COUNTS AS A NO. It is a line that cannot run, and an
    // arm whose whole purpose is to answer before the run would be lying by
    // omission to exit 0 on one. That is the seven minutes, answered.
    if (!whole || stopped || !built.program.deferred.empty())
        return EXIT_MALFORMED;
    return EXIT_FINE;
}

} // namespace satellite
