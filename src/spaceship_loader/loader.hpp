#pragma once

#include "abstract_syntax_tree/ast.hpp"
#include "syntax_parser/parser.hpp"

#include <string>
#include <vector>

// §16's load phase: the stage between parse and resolve that turns a program
// spanning several spaceships into one Program.
//
// A spaceship is a file of satellite source. `satellite.include(helper)` names
// one and `satellite.include("object/helper.satl")` locates one; either way the
// loader finds it, parses it, and merges its declarations into the includer's —
// and only then does resolve() run, over the merged whole.
//
// THAT MERGE IS THE WHOLE DESIGN, and it is why this file is small. resolve()
// already handles forward references and mutual recursion across a program
// (§6's collect-then-walk), so a capsule in one spaceship calling a capsule in
// another needs no new machinery: after the merge it is the same problem
// resolve() was built for. The evaluator changes not at all. Nothing here
// knows what a capsule means — it concatenates declarations and gets out of
// the way.
//
// What the loader owns, and nothing else does:
//
//   - finding a spaceship by name (the search order below) or by quoted path
//   - reading and parsing it, with its own SourceMap id so its errors name it
//   - loading each one ONCE, keyed by canonical path
//   - the order the merged items end up in

namespace satellite {

// A load carries parse errors from every spaceship it touched, plus its own
// (a name that resolves to no file). Both are ParseError because both are
// found before anything runs and both render the same way — a load failure is
// a parse-time failure with a span, not a new category.
struct LoadResult {
    // Every spaceship merged into one, includes before the includer.
    Program program;

    // One entry per spaceship actually loaded, indexed by Span::file. The
    // entry point is id 0. This is what format_error needs to render an error
    // against the text it really came from.
    SourceMap sources;

    std::vector<ParseError> errors;

    bool ok() const { return errors.empty(); }
};

// Parse `source` and everything it includes.
//
// `path` is the entry point's own path, which does two things: it names the
// entry point in error messages, and its directory is the first place an
// include is looked for. Empty means the source came from no file — a REPL
// line — and includes are then resolved from the working directory, which is
// the only honest answer when there is no including file to be relative to.
LoadResult load(const std::string &source, const std::string &path = {});

// Where a spaceship named `name` is looked for, in order. Exposed because
// "cannot find spaceship helper" is not a useful error without it, and because
// a test should check the search order rather than trust it.
//
// `from_dir` is the including spaceship's directory and comes first: that is
// what lets a project's own spaceships find each other with no configuration,
// which is the case that has to be frictionless. It is skipped for a
// language-owned name (`satellite.window`), because a user directory that
// could shadow the language would undo §1's whole naming rule.
std::vector<std::string> search_paths(const std::string &name,
                                      const std::string &from_dir,
                                      bool language_owned);

// Where a QUOTED include is looked for, in order:
//
//   satellite.include("object/forge_object.satl")
//
// Exposed for the same two reasons search_paths is: the not-found error is
// built out of it, and a test should check the order rather than trust it.
//
// `spelling` is the path as the program wrote it, escapes already expanded.
// It is resolved RELATIVE TO THE INCLUDING SPACESHIP — see the reasoning in
// loader.cpp, which is that a project's files include each other by their
// positions in a tree, and running the program from elsewhere does not move
// them. An absolute spelling is taken as written. `..` is left to the kernel;
// canonical() collapses it for the include-once key, so the same file reached
// by two different relative spellings is still loaded once.
std::vector<std::string> path_candidates(const std::string &spelling,
                                         const std::string &from_dir);

} // namespace satellite
