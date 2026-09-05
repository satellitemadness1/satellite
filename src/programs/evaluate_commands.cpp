// `satl --compile` and `satl --call`. See programs/evaluate_commands.hpp.
//
// THE FOUR PASSES MOVED OUT AT M10 AND THE ARGUMENT MOVED WITH THEM.
// programs/built_program.hpp is now where a source becomes a compiled program
// and where the rules about that live -- the pass order, and why nothing on
// this road reads the `.satc` cache. Both were written here at M9 with one
// consumer; `satl file.satl` is the third, and the sentence M9 wrote about the
// cache -- "M10 inherits this decision for `satl file.satl` itself, which is
// the first command where somebody will notice" -- is easier to keep true when
// the decision is in one function rather than quoted into three.

#include "programs/evaluate_commands.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/dump.hpp"
#include "evaluator/machine.hpp"
#include "programs/built_program.hpp"
#include "programs/opening.hpp"
#include "satellite_random/handlers.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_system/handlers.hpp"
#include "satellite_time/handlers.hpp"
#include "satellite_value/render.hpp"
#include "satellite_words/words.hpp"
#include "system_facts/interrupt.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace satellite {

int compile_command(const std::string &path)
{
    Built built;
    if (!build_program(path, built))
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
    if (!build_program(path, built))
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

    // THE SCALARS AND NOT THE CONSOLE, which is still M9's boundary for this
    // arm: `--call` runs one capsule with no `satellite.main` in front of it
    // and no printer behind it, so `display` under it answers S0721 -- but a
    // capsule that trims a string or rounds a number is squarely what the arm
    // is FOR. The clock and the dice joined at M13 by the same test: a
    // capsule that draws a die or paces itself needs no printer either.
    // Ctrl-C is installed for the same reason it is in run_command:
    // this is an entry point that runs user code, and a loop under `--call`
    // is as interruptible as one under `satl file.satl`.
    scalars::install_handlers();
    random::install_handlers();
    time::install_handlers();
    system::install_handlers();
    install_interrupt_handler();
    clear_interrupt();

    eval::Machine machine(built.program.closures, built.parsed.ast,
                          policy_from_the_limits());

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
        // A CEILING IS NOT A MALFORMED FILE, AND NEITHER IS A CTRL-C.
        // machine.hpp's Ending note is the argument for both: M6's watchdog is
        // the other producer of 4, and run_command answers the same 130 --
        // interrupt.hpp's number -- for the same key.
        if (machine.ending() == eval::Ending::Interrupted)
            return INTERRUPT_EXIT_STATUS;
        return machine.at_the_ceiling() ? EXIT_LIMIT : EXIT_MALFORMED;
    }

    // THE ANSWER GOES TO STDOUT AND THE COMPLAINTS TO STDERR, which is the
    // split every arm since `--tokens` has kept.
    fputs((text_of(answer) + "\n").c_str(), stdout);
    return EXIT_FINE;
}

} // namespace satellite
