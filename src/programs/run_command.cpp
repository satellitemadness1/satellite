// Running a satellite program. See programs/run_command.hpp for what this
// milestone is and what it deliberately does not reach.

#include "programs/run_command.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/machine.hpp"
#include "programs/built_program.hpp"
#include "programs/opening.hpp"
#include "satellite_arguments/arguments.hpp"
#include "satellite_console/console.hpp"
#include "satellite_console/handlers.hpp"
#include "satellite_random/handlers.hpp"
#include "satellite_containers/handlers.hpp"
#include "satellite_directory/handlers.hpp"
#include "satellite_file/handlers.hpp"
#include "satellite_help/handlers.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_system/handlers.hpp"
#include "satellite_time/handlers.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"
#include "system_facts/interrupt.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace satellite {

namespace {

// Which compiled capsule `satellite.main` is, or -1.
//
// BY ITS PATH AND NOT BY ITS SPELLING, which is the same lookup a call inside
// the program does. `satellite.main` is `1 3` -- a LANGUAGE path, looked up
// rather than defined, which is what capsule_name's second production in
// DESIGN §6 means by "reserved; currently only `main`". A user's own capsule is
// under `satellite.library` instead (WORD_NUMBERS §3), so there is no spelling
// a program can choose that collides with this.
int find_main(const Built &built)
{
    const words::PathId path =
        static_cast<words::PathId>(words::NodeId::MAIN);
    return built.program.find(path);
}

// What a finished run exits with.
//
// THE ENDING DECIDES IT AND THE PROGRAM'S ANSWER DOES NOT, which is a decision
// this milestone had to take and is the kind PLAN §8 asks to be written down
// rather than discovered. The tempting shape is C's: `satellite.main` returns a
// number and the process exits with it. Three things are wrong with it here.
//
// FIRST, THE NUMBERS ARE ALREADY SPOKEN FOR. programs/opening.hpp's four
// statuses "are assigned by what a failure IS and not by the order they were
// invented in", and a program answering 2 would be reported as "the command
// line did not name something satl can do" -- exactly the disagreement that
// enum exists to prevent, arriving from a direction it cannot see.
//
// SECOND, DESIGN SAYS WHAT THE ANSWER IS FOR. §3: "return the runtime (that
// is, success)". `satellite.return(satellite)` `1 15 1` is how a program says
// it succeeded, and it says it with the runtime singleton rather than with a
// number -- so the language already has a success signal and it is not
// numeric. A satellite number is arbitrary-precision and exact (§8.1); an exit
// status is eight bits. Truncating one into the other is the sort of silent
// conversion §1.1 refuses.
//
// THIRD, NOTHING ELSE COULD READ IT. `satellite.return(value)` `1 15 2` from a
// capsule is read by its CALLER, and `satellite.main` has none inside the
// language. Until something does -- M25's second file, or an Orbit at M28 --
// main's answer has exactly one honest destination, which is nowhere.
//
// SO: 0 when the run finished, 4 when a ceiling stopped it, 1 when the program
// was wrong. THE AUTHOR CAN OVERRULE THIS IN ONE LINE and MILESTONES/M10.md
// carries it as an open item, which is the shape M6's `_exit(2)`-versus-
// `_exit(4)` argument took and is why that one was settled rather than assumed.
//
// AND 130 WHEN A PERSON STOPPED IT -- M11's fourth Ending, and the number is
// the shell's own: 128 + SIGINT, the same status the escalation path inside
// the handler exits with, so a script reads one number however hard Ctrl-C
// had to be pressed. machine.hpp's Ending note carries the argument that an
// interrupt is neither a malformed program nor a machine limit.
int status_of(const eval::Machine &machine)
{
    if (machine.ok())
        return EXIT_FINE;
    if (machine.ending() == eval::Ending::Interrupted)
        return INTERRUPT_EXIT_STATUS;
    return machine.at_the_ceiling() ? EXIT_LIMIT : EXIT_MALFORMED;
}

} // namespace

int run_command(const std::vector<std::string> &args, size_t file_at)
{
    const std::string &path = args[file_at];

    Built built;
    if (!build_program(path, built))
        return built.opened ? EXIT_MALFORMED : EXIT_USAGE;

    const errors::Source against{path, built.text, &built.words};

    const int which = find_main(built);
    if (which < 0) {
        // NO `satellite.main` IS S0402 AND NOT A PLAIN SENTENCE, which is the
        // one place this arm differs from `--call`'s "declares no capsule
        // called `x`". A capsule NAME on a command line is the user's word and
        // has no row; `satellite.main` is the language's, it is where every
        // program starts, and errors.def is where a sentence a person may have
        // to look up belongs.
        fputs(errors::render(
                  errors::make<errors::Code::FILE_NO_MAIN>(errors::kNowhere),
                  against)
                  .c_str(),
              stderr);
        return EXIT_MALFORMED;
    }

    const eval::Capsule &main = built.program.closures.capsules()[which];

    // THE PARAMETER IS THE OBJECT SINCE M20, WHICH IS WHAT THIS MILESTONE
    // OWED THE NAME. M16 built the type and handed slot 0 an empty
    // `satellite.container.list`, and the note this replaces said exactly what
    // was still missing: "the empty list is what M16 owed the slot, and the
    // object is what M20 owes the name". Both halves are now the same value.
    //
    // BUILT BEFORE THE HANDLERS AND BEFORE THE MACHINE, because it is what the
    // command line IS and nothing else in this function may read a stale one.
    // It costs one `getcwd()` -- satellite_arguments/arguments.hpp carries why
    // that one fact cannot be deferred with the other thirty-two.
    //
    // THE WORDS ARE args[file_at...] AND NOT args[0...], which is the one
    // place this arm decides something about the object rather than passing it
    // through. `satl example/hello.satl one two` must give the program three
    // words with the FILE as `program`, not five with `satl` as `program` and
    // the flag it was run under as an argument -- so the command line the
    // object holds starts where the file does. That is v1's answer kept:
    // argv[0] is the script (its interp.hpp says so), and a program that wants
    // the interpreter has `arguments.interpreter` for it.
    arguments::start(std::vector<std::string>(args.begin() +
                                                  static_cast<long>(file_at),
                                              args.end()));

    // NAMED `parameters` AND NOT `arguments`, which is not a style choice: a
    // local called `arguments` shadows the namespace this arm now calls into,
    // and `arguments::object()` stops compiling on the line below it.
    std::vector<Value> parameters;
    if (main.parameters != 0)
        parameters.push_back(arguments::object());

    console::install_handlers();
    scalars::install_handlers();
    containers::install_handlers();
    random::install_handlers();
    time::install_handlers();
    system::install_handlers();
    help::install_handlers();
    file::install_handlers();
    directory::install_handlers();
    arguments::install_handlers();

    // CTRL-C, BEFORE ANYTHING RUNS. Installed here because this is an entry
    // point that runs a program -- interrupt.hpp's rule -- and CLEARED here
    // because a signal that arrived while satl was still compiling belongs to
    // this run, not to a walk that has not started; without the clear, a
    // Ctrl-C during a slow build would stop the program at its first
    // statement and look like a bug in the program.
    install_interrupt_handler();
    clear_interrupt();

    // THE PRINTER STARTS HERE, WHERE THE COST CAN BE ATTRIBUTED. The console
    // starts it for itself on the first line queued -- console.cpp's push() --
    // so this is not what makes it correct; it is what makes the thread a
    // measured part of running a program rather than a surprise inside the
    // first `display`. PLAN §9's rule, and §4.3's startup floor is why no arm
    // above this line may reach it.
    console::Console &out = console::Console::the();
    out.start();

    eval::Machine machine(built.program.closures, built.parsed.ast,
                          policy_from_the_limits());

    // THE GLOBALS FIRST, ALWAYS. DESIGN §7.2 reserves `satellite.library` for
    // shared state, so a capsule that reads one has to find it filled in --
    // which means the initialisers run before any capsule does, exactly as
    // resolve numbers them in a pass before the bodies.
    machine.run_top_level();
    if (machine.ok())
        machine.call(static_cast<uint32_t>(which), parameters);

    // THE FOUR STEPS, AND THEY COME BEFORE THE DIAGNOSTICS. drain, flush, stop,
    // join -- see satellite_console/console.hpp. A program that printed three
    // lines and then divided by zero must show the three lines ABOVE the
    // caret, and stdout and stderr are two streams with two buffers: without
    // this the error is written while the output is still in a queue, and a
    // terminal shows them in the wrong order.
    out.shutdown();

    if (!machine.ok())
        fputs(errors::render(machine.problems(), against).c_str(), stderr);

    return status_of(machine);
}

} // namespace satellite
