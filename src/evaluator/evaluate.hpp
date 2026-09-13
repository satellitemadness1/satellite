#pragma once

// The evaluator -- PLAN M9, and the one door over this module.
//
// TWO STEPS AND THEY ARE DELIBERATELY SEPARATE. compile() turns a resolved tree
// into a closure tree and can fail with diagnostics; a Machine runs one. PLAN
// §2.3 says the compile is "the same pass as resolve, measured in microseconds"
// and that "from outside, `satl file.satl` is as interpreted as it ever was" --
// so the split is not a compile step a user waits for, it is the seam that lets
// `satl --compile` print a program without running it and lets one compiled
// program be run twice.
//
// WHAT THIS MILESTONE DOES NOT DO IS RUN A PROGRAM, and the boundary is PLAN
// §8's. M10 is "the milestone at which satellite executes anything at all" --
// the console with its printer thread, `satellite.main` and `satellite.return`
// -- and this one builds the machinery all of that dispatches through. So the
// consumers here are `satl --compile`, which prints the closure tree the way
// `--resolve` prints frames, and `satl --call`, which runs one capsule by name
// and prints the value it answered. The second is `satl --number`'s shape one
// milestone on: a way to see the thing that was built, with no console behind
// it and no `satellite.main` in front of it.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "evaluator/closure.hpp"
#include "evaluator/machine.hpp"
#include "name_resolver/resolve.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <utility>
#include <vector>

namespace satellite::eval {

// A compiled program and everything the compile could not do.
struct Program {
    Compiled closures;
    std::vector<errors::Diagnostic> problems;

    // WHAT THIS PROGRAM WILL REFUSE IF IT GETS THERE -- M26.5. Separate from
    // `problems` because these do not stop a compile and must not stop a run:
    // `ok()` deliberately does not look at them. A piece of grammar that parses
    // and does not run yet is a fact about the MILESTONE, not about the program,
    // and errors.def's S0720 note is the argument for why a run keeps its
    // nerve -- a construct in a branch nobody takes never happens.
    //
    // `satl --check` PRINTS THEM, which is the only behaviour that changed. The
    // compiler always knew; nothing asked it.
    std::vector<errors::Diagnostic> deferred;

    bool ok() const { return !errors::any_error(problems); }

    // Which compiled capsule this path is, or -1. `satl --call` needs it and so
    // does anything that runs a named capsule.
    int find(words::PathId path) const;
};

// Compile a resolved program. Never throws, and reports every problem it finds
// rather than the first -- the rule the lexer, the parser and resolve keep.
//
// NOTHING IS COMPILED FROM A TREE THAT DID NOT RESOLVE, which is the caller's
// job and is the same rule resolve_command.cpp states for a tree that did not
// parse: half a program's names were never bound, so every call in it would be
// refused and the carets would bury the one thing that is actually wrong.
Program compile(const Ast &ast, const resolve::Resolved &resolved, words::Words &words);

// ONE FILE OF A PROGRAM OF SEVERAL -- M25. The tree, what resolve said about
// it, the spaceship's name (empty for file 0), and which file each of its
// Include nodes loads.
struct Unit {
    const Ast *ast = nullptr;
    const resolve::Resolved *resolved = nullptr;
    std::string name;
    std::vector<std::pair<NodeIndex, uint32_t>> includes;
};

// Every file, compiled into ONE program -- one op arena, one capsule table, one
// `satellite.library`. `setup_order` is the order the files' globals are set up
// in at the start of a run: every file after the files it includes.
Program compile_run(const std::vector<Unit> &units, words::Words &words,
                    const std::vector<uint32_t> &setup_order);

} // namespace satellite::eval
