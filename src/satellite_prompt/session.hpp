#pragma once

// One prompt session: what has been typed, and what running it means.
//
// A TYPED LINE IS NOT A PROGRAM, WHICH IS THE FACT EVERYTHING HERE IS SHAPED
// AROUND. S0204 says what a file holds -- `satellite.include`,
// `satellite.capsule`, `satellite.spacesuit` and `satellite.library.<name>` --
// and a line like `satellite.console.display("hi")` is none of them. So the
// session WRAPS what was typed:
//
//     satellite.include(satellite)
//     <every top-level form typed so far>
//     satellite.capsule satellite.main()
//     {
//         <the line just typed>
//     }
//
// THE WRAPPER TAKES NO PARAMETER AND WRITES NO RETURN, and both are measured
// rather than copied from DESIGN §3. `satellite.capsule satellite.main()` is
// what example/scalars.satl declares and it runs; the empty
// `satellite.container.list<satellite.variable.string> arguments` that hello
// world writes is a parameter M16 built and M20 will fill, and a prompt that
// declared it would be handing every typed line an empty list nothing reads.
// `satellite.return(satellite)` is likewise optional -- a capsule that falls off
// its end has finished.
//
// WHERE A LINE GOES IS DECIDED BY block.hpp, and the two halves accumulate
// differently. A top-level form is KEPT: a capsule declared at the prompt is
// still there on the next line, because every line rebuilds the whole program
// from the forms typed so far. A statement is NOT: it runs once, inside a fresh
// `satellite.main`, and the locals it declared go with it. That boundary is
// PLAN M22's scope drawn where the language already draws it, and
// MILESTONES/M22.md says what it costs.
//
// THE LINE NUMBERS ARE REBASED BEFORE ANYTHING IS PRINTED. The wrapper puts
// three lines above what the user typed, so a mistake on their line 1 is on the
// text's line 4. Every diagnostic carries its own line (`Span::line`), so the
// session subtracts the prologue from each one and renders afterwards -- which
// is why build_source() is asked not to report.

#include "programs/built_program.hpp"

#include <string>
#include <vector>

namespace satellite::prompt {

class Session {
public:
    // Run one accepted entry -- one line, or several if it opened a block.
    // Answers false only when the session should end.
    bool run(const std::string &entry);

    // Run a file, the way `satl <file>` would. This is what `run <file>` at the
    // prompt does, and it is the shape M18's help store is waiting for: a
    // program runs, and afterwards its variable names and types are still
    // known. THE STORE ITSELF IS NOT HERE -- see MILESTONES/M22.md §6 and
    // PLAN §8's M18 entry, which owns both the store and its only reader.
    bool run_file(const std::string &path);

    // The top-level forms typed so far, oldest first.
    const std::vector<std::string> &top_level() const { return top_level_; }

private:
    std::string wrap(const std::string &body) const;
    void report(Built &built, const std::string &name, int above) const;

    std::vector<std::string> top_level_;

    // Every `satellite.library.<name>` this session has DECLARED. The first
    // mention of a name is a declaration and goes to the top level; every later
    // one is an assignment and goes inside main, which is the only way a global
    // made at the prompt can ever be changed. See block.hpp's `library_name`.
    std::vector<std::string> globals_;
};

// How many lines the wrapper writes above the typed body. Public because
// tests/prompt_test asserts the rebasing against it rather than against 3.
inline constexpr int kPrologueLines = 3;

} // namespace satellite::prompt
