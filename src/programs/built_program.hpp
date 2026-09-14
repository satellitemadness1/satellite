#pragma once

// A file, read and parsed and resolved and compiled -- what every arm that
// needs a runnable program shares.
//
// SPLIT OUT AT M10 BY SUBJECT, THE SAME SEAM programs/source_file.hpp WAS SPLIT
// ON. That file's header names this one before it existed: getting the bytes
// off the disk is "what M3's --tokens, M4's parser, M4.5's .satc reader and
// M10's runner all need the same answer to". This is the layer above it and the
// same argument one pass along -- `--compile`, `--call` and `satl file.satl`
// each need a source turned into an `eval::Program`, and three copies of those
// twenty lines would be three places the ORDER of the passes is decided.
//
// AND THE ORDER IS THE POINT RATHER THAN THE TYPING. Nothing is compiled from a
// tree that did not resolve, and nothing is resolved from a tree that did not
// parse -- because half a program's names were never bound, so every call in it
// would be refused and twenty carets would bury the one name that is actually
// misspelled. resolve_command.cpp states the rule one pass earlier and
// evaluate_commands.cpp stated it here; now there is one function that keeps
// it.
//
// NOTHING HERE READS THE `.satc` CACHE, AND THAT IS A RULE RATHER THAN AN
// OVERSIGHT. A tree read from a cache has spans into the `.satc`'s WORDS, where
// "line 6 of that is not line 6 of the file the user wrote" -- measured on
// hello_world.satl at M9, where 12 source lines become 11 and
// `satellite.return` moves from line 11 to line 10. resolve can afford to read
// one and re-parse on failure, because resolve is a pure function of a tree; an
// evaluator cannot, because it can raise a diagnostic at any moment during a
// run and by then the run has HAPPENED -- output already printed at M10, a file
// already written at M19. "Run it again from the source" is not available to
// anything that has done something. MILESTONES/M9.md §4 says what the real fix
// is and that it belongs to the `.satc` format.

#include "abstract_syntax_tree/ast.hpp"
#include "evaluator/evaluate.hpp"
#include "evaluator/machine.hpp"
#include "name_resolver/resolve.hpp"
#include "parser/parser.hpp"
#include "satellite_words/words.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace satellite {

// ONE SPACESHIP A RUN LOADED -- PLAN M25, 2026-09-13. Everything the four passes
// produced for it, kept for the same reason Built keeps its own: the op arena
// indexes this tree, and the renderer quotes this text. Held by pointer in
// Built so that neither moves once something points into it.
struct LoadedSpaceship {
    std::string path;        // as the including file's directory spells it
    std::string canonical;   // the same file however it was reached
    std::string name;        // `ship`
    std::string text;
    words::PathId node = words::kNoPath;  // `satellite.library.ship`
    Parse parsed;
    resolve::Resolved resolved;
};

// Everything the four passes produced, kept together because they refer to each
// other.
//
// THE NUMBERING, THE TREE AND THE CLOSURES ALL LIVE IN ONE OBJECT AND THEY HAVE
// TO. A user's PathId is valid inside one run (words_runtime.hpp), an op's
// operands index the arena beside it, and a Machine holds references to both --
// so an arm that let any of the three die would be asking about numbers and
// indices that no longer mean anything. tests/eval_test/eval_test.hpp says the
// same thing about its own fixture and for the same reason.
struct Built {
    words::Words words;
    std::string text;
    Parse parsed;
    resolve::Resolved resolved;
    eval::Program program;

    // THE OTHER FILES, BY FILE ID LESS ONE -- M25. Empty for a program that
    // includes nothing, which then builds exactly as it did before includes.
    std::vector<std::unique_ptr<LoadedSpaceship>> ships;
    std::vector<errors::Source> others;

    // HOW MANY OF FILE 0's TOP-LEVEL SPACESHIP INCLUDES ALREADY RAN -- M25, and
    // only M22's prompt sets it. The prompt rebuilds the whole program for every
    // line typed, with every top-level form kept from the lines before; the
    // first N includes at the top were typed on earlier lines and ran then, so
    // they are loaded for their names and compiled to nothing. The author:
    // launch code runs "when the interpreter hits satellite.include(filename)
    // ... then and only then" -- not again on every later line.
    uint32_t already_included = 0;

    // What every diagnostic about this program is rendered against: this file,
    // and every spaceship's own text for a span that says it is in one.
    errors::Source source(std::string_view name) const
    {
        return errors::Source{name, text, &words, ships.empty() ? nullptr : &others};
    }

    // Every spaceship's path, by file id less one -- for the warning log.
    std::vector<std::string> other_paths() const
    {
        std::vector<std::string> out;
        for (const auto &ship : ships)
            out.push_back(ship->path);
        return out;
    }

    // WHETHER THE FILE OPENED, WHICH IS A DIFFERENT FAILURE FROM THE REST. A
    // path that could not be read is EXIT_USAGE -- the command line named
    // something that is not there -- and a file that would not parse is
    // EXIT_MALFORMED. Every caller makes that choice and none of them should
    // have to work out which happened.
    bool opened = false;
    bool ok = false;
};

// Read, parse, resolve and compile. Every problem is printed to stderr through
// the one renderer as it is found; the answer is whether there is a program.
bool build_program(const std::string &path, Built &out);

// The same, over text already in `out.text` -- M22's prompt, which builds a
// program out of what somebody typed and never writes it anywhere. `name` heads
// each diagnostic in place of a path.
// `report` false hands the diagnostics back in `out` instead of printing them,
// which is what M22's prompt needs in order to rebase their line numbers onto
// the line the person actually typed.
bool build_source(const std::string &name, Built &out, bool report = true);

// The two numbers the evaluator obeys and does not choose, from the file beside
// the binary -- and, since M11, the Ctrl-C flag it watches and does not own.
//
// READ HERE AND NOWHERE INSIDE THE EVALUATOR, which is the seam
// evaluator/machine.hpp's Policy note draws: `satl` obeys machine_limits and
// listens to system_facts, tests/eval_test obeys itself, and that is what lets
// the depth fixtures link no machine_limits at all and the interrupt fixtures
// raise no signal.
eval::Policy policy_from_the_limits();

} // namespace satellite
