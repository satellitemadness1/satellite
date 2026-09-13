// Running a satellite program. See programs/run_command.hpp for what this
// milestone is and what it deliberately does not reach.

#include "programs/run_command.hpp"

#include "error_reporter/report.hpp"
#include "error_reporter/warning_log.hpp"
#include "evaluator/machine.hpp"
#include "programs/arms.hpp"
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
#include "satellite_thread/handlers.hpp"
#include "satellite_thread/thread_handle.hpp"
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

    // ONE CALL AND NOT ELEVEN -- programs/arms.hpp. The eleven were here until
    // 2026-09-12, and the copy of them in the prompt was missing threads.
    arms::install_for(arms::Arm::Run);

    // CTRL-C, BEFORE ANYTHING RUNS. Installed here because this is an entry
    // point that runs a program -- interrupt.hpp's rule -- and CLEARED here
    // because a signal that arrived while satl was still compiling belongs to
    // this run, not to a walk that has not started; without the clear, a
    // Ctrl-C during a slow build would stop the program at its first
    // statement and look like a bug in the program.
    install_interrupt_handler();
    clear_interrupt();
    errors::log::set_program(path);

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

    // CLOSE EVERYTHING, AND IT COMES BEFORE THE CONSOLE'S SHUTDOWN -- M23, and
    // the author's decision of 2026-09-12 in their own words: "whenever the
    // program reaches `satellite.return(satellite)`, close everything."
    //
    // WHAT "CLOSE" MEANS HERE IS STOP AND JOIN, NOT WAIT. A thread nobody
    // joined is told to stop -- it notices at its next statement boundary,
    // through the same hook M11's Ctrl-C uses -- and then it IS joined, so no
    // detached walk is left reading an op arena that is about to go out of
    // scope with `built`. Waiting instead would hang a program whose thread
    // loops forever; abandoning instead would be a use-after-free.
    //
    // BEFORE out.shutdown() BECAUSE A THREAD CAN PRINT. The console's four
    // steps are drain, flush, stop, join, and a thread still walking after the
    // queue was drained would push a line into a printer that had gone. So the
    // walks stop first and the printer stops second, which is the same
    // ordering argument the comment below makes about stdout and stderr, one
    // producer further back.
    //
    // AND WHAT COMES BACK IS THE ERRORS OF THREADS NOBODY WAITED FOR. A thread
    // that refused on its own and was never joined has a diagnostic that would
    // otherwise be lost, and a program losing an error silently is what DESIGN
    // §1.1 will not have. A thread THIS call stopped has none worth reporting:
    // the program was over.
    const std::vector<errors::Diagnostic> abandoned = thread::close_all();

    out.shutdown();

    // THE WARNINGS FIRST, BECAUSE THEY HAPPENED FIRST -- a second join() that
    // warned and then an error later in the run read in the order they came.
    // They change no status: the run did what it was written to do.
    const std::vector<errors::Diagnostic> warned = errors::log::take();
    if (!warned.empty())
        fputs(errors::render(warned, against).c_str(), stderr);

    if (!machine.ok())
        fputs(errors::render(machine.problems(), against).c_str(), stderr);
    else if (!abandoned.empty())
        fputs(errors::render(abandoned, against).c_str(), stderr);

    // A THREAD'S REFUSAL IS THE PROGRAM'S REFUSAL, which is the one place this
    // milestone changes what a status means. `status_of` reads the main walk's
    // ending and the main walk finished; a capsule that divided by zero on a
    // thread is still a program that was wrong, and a script that tested the
    // status would otherwise be told it succeeded.
    if (machine.ok() && !abandoned.empty())
        return EXIT_MALFORMED;
    return status_of(machine);
}

} // namespace satellite
