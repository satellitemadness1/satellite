// `satl --compile` and `satl --call`. See programs/evaluate_commands.hpp.
//
// NEITHER OF THESE READS THE `.satc` CACHE, AND M9 IS WHERE THAT BECAME A RULE
// RATHER THAN A CHOICE. resolve_command.cpp already carries half of the
// argument: a tree read from a cache has spans into the `.satc`'s WORDS, which
// has "no comments, blank lines where the writer put them, and line 6 of that
// is not line 6 of the file the user wrote". Measured on hello_world.satl at
// this milestone: 12 source lines become 11 cached ones, and
// `satellite.return` moves from line 11 to line 10.
//
// WHAT resolve DOES ABOUT IT CANNOT BE DONE HERE. That arm reads from the
// cache, and if the pass FAILS it throws the answer away and parses the source
// again -- affordable because resolve is a pure function of a tree and running
// it twice costs a walk. An evaluator is not: it can raise a diagnostic at any
// moment during a run, and by then the run has happened. From M10 that means
// output already printed, and from M19 a file already written. "Run it again
// from the source" is not available to anything that has done something.
//
// SO A COMMAND THAT WILL REPORT AT RUN TIME PARSES THE SOURCE, and the cost is
// the trie walks M4.5's cache exists to save -- counted rather than timed in
// MILESTONES/M7.md §5, and small. M10 inherits this decision for `satl
// file.satl` itself, which is the first command where somebody will notice the
// cache not being used; MILESTONES/M9.md §4 says what the real fix is and why
// it is the `.satc` format's and not this arm's.
//
// AND NEITHER COMPILES A TREE THAT DID NOT RESOLVE, which is the rule
// resolve_command.cpp states one pass earlier for a tree that did not parse:
// half a program's names were never bound, so every call in it would be refused
// and twenty carets would bury the one name that is actually misspelled.

#include "programs/evaluate_commands.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dump.hpp"
#include "evaluator/evaluate.hpp"
#include "evaluator/machine.hpp"
#include "machine_limits/limits.hpp"
#include "name_resolver/resolve.hpp"
#include "parser/parser.hpp"
#include "programs/check_command.hpp"
#include "programs/opening.hpp"
#include "satellite_value/render.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace satellite {

namespace {

// Everything both arms need, or `ok` false with the complaint already printed.
struct Built {
    words::Words words;
    std::string text;
    Parse parsed;
    resolve::Resolved resolved;
    eval::Program program;
    bool opened = false;
    bool ok = false;
};

bool build(const std::string &path, Built &out)
{
    if (!open_source(path, out.text))
        return false;
    out.opened = true;

    const errors::Source against{path, out.text, &out.words};

    out.parsed = parse(out.text, out.words);
    if (!out.parsed.errors.empty())
        fputs(errors::render(out.parsed.errors, against).c_str(), stderr);
    if (!out.parsed.ok())
        return false;

    out.resolved = resolve::resolve(out.parsed.ast, out.words);
    if (!out.resolved.problems.empty())
        fputs(errors::render(out.resolved.problems, against).c_str(), stderr);
    if (!out.resolved.ok())
        return false;

    out.program = eval::compile(out.parsed.ast, out.resolved, out.words);
    if (!out.program.problems.empty())
        fputs(errors::render(out.program.problems, against).c_str(), stderr);
    out.ok = out.program.ok();
    return out.ok;
}

// THE POLICY IS READ HERE AND NOWHERE INSIDE THE EVALUATOR, which is the seam
// machine.hpp's Policy note draws. `satl` obeys the file beside its binary;
// tests/eval_test obeys itself, and that is what lets the depth fixtures run
// against the 8 MiB a login shell hands out.
eval::Policy from_the_limits()
{
    eval::Policy policy;
    policy.max_depth = limits::max_depth_bytes();
    policy.division_digits = limits::division_digits();
    return policy;
}

} // namespace

int compile_command(const std::string &path)
{
    Built built;
    if (!build(path, built))
        return built.opened ? EXIT_MALFORMED : EXIT_USAGE;

    fputs(eval::dump_text(path, built.parsed.ast, built.words, built.program)
              .c_str(),
          stdout);
    return EXIT_FINE;
}

int call_command(const std::vector<std::string> &args)
{
    const std::string &path = args[2];
    const std::string &name = args[3];

    Built built;
    if (!build(path, built))
        return built.opened ? EXIT_MALFORMED : EXIT_USAGE;

    // THE CAPSULE IS FOUND BY ITS PATH AND NOT BY ITS SPELLING, because
    // DESIGN §7.6 puts capsules in a table of their own and WORD_NUMBERS §3
    // says the parser interned the name when it first met it. Asking the run's
    // own numbering is the same lookup a call inside the program does.
    // UNDER `library` FIRST AND `satellite` SECOND, and the two are the two
    // ways a capsule can be declared. parser_declarations.cpp's top_level()
    // gives a user's capsule `satellite.library` as its owner -- WORD_NUMBERS
    // §3's worked example is that a user's first name there is `1 14 3` --
    // while a capsule named `satellite.main` is a LANGUAGE path looked up
    // rather than defined. Both are callable and neither is the other's parent.
    words::PathId path_id = built.words.find(words::NodeId::LIBRARY, name);
    if (path_id == words::kNoPath)
        path_id = built.words.find(words::NodeId::SATELLITE, name);
    const int which = path_id == words::kNoPath ? -1 : built.program.find(path_id);
    if (which < 0) {
        fprintf(stderr, "satl: %s declares no capsule called `%s`\n", path.c_str(),
                name.c_str());
        return EXIT_MALFORMED;
    }

    std::vector<Value> arguments;
    for (size_t i = 4; i < args.size(); i++) {
        Number value;
        if (!Number::parse(args[i], value)) {
            fprintf(stderr, "satl: `%s` is not a number\n", args[i].c_str());
            return EXIT_USAGE;
        }
        arguments.push_back(Value::number(std::move(value)));
    }

    eval::Machine machine(built.program.closures, built.parsed.ast,
                          from_the_limits());

    // THE GLOBALS FIRST, ALWAYS. DESIGN §7.2 reserves `satellite.library` for
    // shared state, so a capsule that reads one has to find it filled in --
    // which means the initialisers run before any capsule does, exactly as
    // resolve numbers them in a pass before the bodies.
    machine.run_top_level();

    Value answer;
    if (machine.ok())
        answer = machine.call(static_cast<uint32_t>(which), arguments);

    if (!machine.ok()) {
        fputs(errors::render(machine.problems(),
                             errors::Source{path, built.text, &built.words})
                  .c_str(),
              stderr);
        // A CEILING IS NOT A MALFORMED FILE. machine.hpp's Ending note is the
        // argument, and M6's watchdog is the other producer of this status.
        return machine.at_the_ceiling() ? EXIT_LIMIT : EXIT_MALFORMED;
    }

    // THE ANSWER GOES TO STDOUT AND THE COMPLAINTS TO STDERR, which is the
    // split every arm since `--tokens` has kept.
    fputs((text_of(answer) + "\n").c_str(), stdout);
    return EXIT_FINE;
}

} // namespace satellite
