// satellite-004 -- the interpreter built from numbered libraries: `satl`.
//
//     build/satl                                  the opening lines
//     build/satl [--debug] <program.satl> [words...]
//     build/satl [--debug] --run <file> [words...]
//     build/satl [--debug] --repl                 the prompt (M0.6; 14 until then)
//     build/satl --version | -V                   the title lines: version, revision, build, compiler
//     build/satl --help | -h                      the start-up block and every way to start
//
// arguments/command_line.hpp has the rules. THE BINARY IS NAMED satl SINCE M0.5
// (the author, 2026-09-15), and build/satellite-004 is a link to it.
//
// THE START-UP BLOCK (version.hpp) goes to stdout for --version and --help, and
// to STDERR every time a program starts, so a program's stdout stays exactly
// what the program wrote. arguments.startup_display = false in
// satellite/config/satellite_config.hpp turns the start-up copy off.
//
// Start-up reads the author's satellite_config.hpp, parks
// arguments.threads_startup threads (threads/startup_threads.hpp), loads every compiled library in build/satellite-numbers/ into the
// number index; then the .satl file is loaded, checked, TURNED INTO 16-BIT
// TOKENS (bytecode/bytecode_registry.hpp), turned into calls with their
// functions already chosen, and run. The exit status is the machine code
// the program stopped on: 0 when it ran to the end, and 255 for a code an exit
// status cannot hold (machine/exit_status.hpp).
//
// (the author, 2026-09-16) "First start 256 threads, then load the tiny C++
// libraries, then convert the .satl to 16-bit." That order is the order below,
// and the tokens are the first thing built out of a program.

#include "arguments/arguments.hpp"
#include "bytecode/sate_file.hpp"
#include "bytecode/window_calls.hpp"
#include "bytecode/bytecode_registry.hpp"
#include "bytecode/function_table.hpp"
#include "bytecode/program_walk.hpp"
#include "bytecode/word_counts.hpp"
#include "bytecode/statement_ring.hpp"
#include "config/config_file.hpp"
#include "config/feature_register.hpp"
#include "config/feature_switch.hpp"
#include "config/rebuild.hpp"
#include "config/run_config.hpp"
#include "config/run_feedback.hpp"
#include "machine/s_codes.hpp"
#include "machine/critical_report.hpp"
#include "machine/exit_status.hpp"
#include "machine/machine_codes.hpp"
#include "machine/machine_state.hpp"
#include "../satellite-numbers/call_number.hpp"
#include "satl/satl_file.hpp"
#include "satl/session.hpp"
#include "threads/startup_threads.hpp"
#include "version/version.hpp"

#include <iostream>
#include <string>
#include <vector>

#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdlib>
#include <new>
#include <cstring>
#include <unistd.h>

namespace {

// The folder the libraries were built into: satellite-numbers/ beside this
// executable -- build/satl's, or an installed satl's (PLAN M0.5: the three are
// built into one folder and installed as one folder).
//
// NEVER THE CURRENT DIRECTORY (ERROR #8). This used to fall back to
// "satellite-numbers" when /proc/self/exe could not be read, which ran whatever
// libraries the folder satl was started in held. It also read into PATH_MAX
// bytes and took a full buffer as the whole path. Now the buffer grows until the
// path fits, and a path the kernel will not give -- a satl installed deeper than
// it can name, ENAMETOOLONG -- answers empty, with errno saying why.
std::string numbers_folder()
{
    std::string path(PATH_MAX, '\0');
    for (;;) {
        const ssize_t length = readlink("/proc/self/exe", path.data(), path.size());
        if (length <= 0)
            return std::string();
        if (static_cast<std::size_t>(length) < path.size()) {
            path.resize(static_cast<std::size_t>(length));
            break;
        }
        path.resize(path.size() * 2);
    }
    return path.substr(0, path.rfind('/')) + "/satellite-numbers";
}

// Every argument, one line each, while debug mode is on.
void display_arguments(const satellite004::Arguments &arguments, satellite004::MachineState &state)
{
    for (const satellite004::Argument &argument : arguments.all())
        state.set(argument.name + " = " + satellite004::describe(argument), satellite004::success);
}

} // namespace

// Everything satl does, answering the machine code it stopped on. main() turns
// that code into an exit status, in one place.
signed long long int run_satl(int argc, char **argv)
{
    using namespace satellite004;

    // The author's satellite_config.hpp first: the title lines need its numbers.
    Arguments arguments;
    signed long long int code = arguments.gather_config();
    if (stops_the_program(code))
        return code;

    CommandLine command_line;
    code = read_command_line(argc, argv, command_line);
    if (stops_the_program(code))
        return code;

    if (command_line.command == Command::version || command_line.command == Command::help ||
        command_line.command == Command::opening) {
        if (command_line.command == Command::version)
            std::cout << title_lines(arguments);
        else if (command_line.command == Command::help)
            std::cout << startup_block(arguments) << usage_lines();
        else
            // "Nothing to do is not an error" (003 main.cpp), so bare satl is 0.
            std::cout << startup_block(arguments) << opening_lines();
        std::cout.flush();
        if (!std::cout)
            return report_error("satl(output): the output refused the lines", display_error);
        return success;
    }

    // `satl --rebuild`, BEFORE THE CONFIG NOTICE BELOW. It is the command that
    // FIXES a missing config.ini, so telling somebody to reinstall on their way
    // into it would be advice against the thing they are already doing.
    if (command_line.command == Command::rebuild)
        return run_rebuild();

    // `satl --config`, for the same reason and one more: it writes machine.conf
    // and reads nothing out of config.ini, so a missing config.ini has no
    // bearing on it at all.
    if (command_line.command == Command::config)
        return run_config(command_line.most);

    // `satl --feedback`, and it reads config.ini for nothing at all: the book is
    // its own file and the command only prints it.
    if (command_line.command == Command::feedback)
        return run_feedback();

    // THE LASTING SETTINGS, AND SAYING SO WHEN THEY ARE NOT THERE. Checked here
    // and not above, so `satl --version`, `satl --help` and a bare `satl` stay
    // quiet: those three answer a question about satl itself and do not run a
    // program, and a person asking the version does not need to be told about a
    // file no part of that answer reads.
    //
    // THE RUN CARRIES ON. Every setting has its own default, so this costs
    // nothing but the telling -- see config_file.hpp's exists() for why it is a
    // notice and why satl does not quietly create the file to make it go away.
    if (!config_file::exists()) {
        const std::string where = config_file::path();
        CriticalReport missing;
        missing.code = "S010";
        missing.name = "CONFIG_FILE_MISSING";
        missing.description =
            "no config.ini, so every setting is its built-in default. satl --rebuild writes one";
        missing.directory = where.empty() ? std::string("$HOME is not set, so there is no ~/.satl")
                                          : where;
        // A NOTICE AND NOT A REPORT, on the author's severity ruling. The run
        // carries on and nothing is lost, so it does not get two rules of dashes.
        print_notice(missing);
    }

    // THE ONE VALUE, READ ONCE. This is the whole per-run cost of the feature
    // system: one line out of config.ini and one integer. Everything that hangs
    // off it -- the last-known store, the frame stack, the statement ring --
    // tests THIS, and SATELLITE_ERROR Part 10 measured what that costs.
    const RegisterReading reading = start_register();
    const FeatureRegister features = reading.features;

    // THE KEY IS THERE AND CANNOT BE READ, which is not the same as absent and
    // must not print as it (Part 7, rule 3). A fresh install has no register and
    // wants no noise; a damaged one is a thing somebody has to fix.
    if (reading.unreadable) {
        CriticalReport damaged;
        damaged.code = "S012";
        damaged.name = "REGISTER_NOT_READABLE";
        damaged.description =
            "config.ini has a feature register and it is not a binary satl can read, so this run "
            "is using the built-in default for every feature. Run satl --rebuild to write a good "
            "one from your settings.";
        damaged.directory = config_file::path();
        damaged.syntax = std::string(kRegisterKey) + " = " + reading.said;
        damaged.caret_at = std::string(kRegisterKey).size() + 3;
        damaged.caret_note = "a satellite binary was expected here -- b then 1s and 0s, as "
                             "satl --rebuild writes it";
        print_critical(damaged);
    }

    // SOMEBODY CHANGED A SETTING AND HAS NOT REBUILT, which is the one trap this
    // design has: the named keys are what a person edits and what a program
    // writes, and `features` is what satl READS. They can disagree, and a person
    // whose `arguments.access = satellite.bool.false` seemed to do nothing is
    // owed the reason rather than left to find it.
    if (reading.disagrees) {
        CriticalReport stale;
        stale.code = "S011";
        stale.name = "REGISTER_IS_STALE";
        stale.description =
            "a setting changed since satl --rebuild last ran, so this run uses the saved register "
            "and not the setting. satl --rebuild composes them again";
        stale.directory = config_file::path();
        print_notice(stale);
    }

    MachineState state;
    code = arguments.gather(command_line);
    if (stops_the_program(code))
        return code;
    if (arguments.flag("arguments.startup_display"))
        std::cerr << startup_block(arguments);

    state.debug_mode = arguments.flag("arguments.debug_mode");
    // THE REGISTER REACHES THE INTERPRETER HERE, and this one line is what makes
    // every bit readable everywhere: `MachineState &state` is already threaded
    // through the walker, the expression reader and every call.
    state.features = features;
    // AND THE CONFIG ROWS, the same way and for the same reason: `arguments` lives
    // until run_satl returns, which is after every program and prompt line it runs.
    state.arguments = &arguments;
    state.set("satellite " + version_line(arguments) + " (starting)", success);
    state.set("arguments(gathered)", success);

    // THE REGISTER, SPELLED OUT -- SATELLITE_ERROR Part 10's rule 4. A run
    // gathered with half the features off has holes in it, and a person reading
    // the output has no way to tell a section that was empty from one that was
    // never collected. So the bits AND the names, whenever anything is on.
    if (state.debug_mode) {
        state.set(std::string("features = ") + written(features) +
                      (reading.found      ? " (from config.ini)"
                       : reading.unreadable ? " (built-in defaults; the saved one could not be read)"
                                            : " (built-in defaults; none saved)"),
                  success);
        for (unsigned i = 0; i < kFeatureCount; ++i)
            if (features.on(static_cast<Feature>(i)))
                state.set(std::string("features.") + feature_facts()[i].name + " = true" +
                              (feature_facts()[i].built ? "" : " (listed, NOT BUILT YET)"),
                          success);
    }
    if (state.debug_mode)
        display_arguments(arguments, state);

    // The start-up threads, warm before anything else loads (the author, 2026-09-15).
    // Never more than arguments.threads_max; a refused thread is reported, and the
    // program still runs on the threads that started.
    satellite_number asked = arguments.number("arguments.threads_startup");
    if (arguments.find("arguments.threads_max") != nullptr &&
        satellite_number::compare(asked, arguments.number("arguments.threads_max")) > 0) {
        const satellite_number &threads_max = arguments.number("arguments.threads_max");
        // Shown every time, not only in debug mode: the author asked for more threads than start.
        report_error("threads.startup(capped): arguments.threads_startup " + asked.to_text() +
                         " is more than arguments.threads_max " + threads_max.to_text() + ", so " +
                         threads_max.to_text() + " threads start",
                     success);
        asked = threads_max;
    }
    // THE ROW IS A satellite_number AND A THREAD IS COUNTED BY THE MACHINE. A
    // maximum is a ceiling, never an instruction (DESIGN §1.2): a row longer than
    // an unsigned long long is more threads than any machine can start, so it asks
    // for all it can count, and StartupThreads reports the machine's refusal the
    // way it reports any count the machine will not give. Both rows are checked
    // not negative in gather_config, so a one-limb value is the count itself.
    const unsigned long long int startup = asked.fits_one_limb() ? asked.limb(0) : ~0ull;

    // Declared BEFORE the threads, so they are destroyed after the threads have
    // run every queued job and stopped: a job may point into them (PLAN M7).
    NumberIndex index;
    BytecodeRegistry bytecode_registry;
    BytecodeFilenames bytecode_filenames;
    FunctionTable functions;

    // THE TOPOLOGY (the author, 2026-09-16): main starts ONE thread, and that one
    // starts the 256. Main does not wait for them -- it loads the number index
    // and builds the function table while they come up, which is the first time
    // this interpreter does two things at once. Parking 256 costs ~12 ms and
    // neither of those two jobs needs a thread.
    StartupThreads threads;
    threads.start_in_background(startup);

    const std::string folder = numbers_folder();
    if (folder.empty())
        return report_error(std::string("vector.number.index(error): satl cannot read its own path (/proc/self/exe: ") +
                                std::strerror(errno) + "), so it cannot find the satellite-numbers/ beside it",
                            vector_loading_error);
    code = index.load(folder, state);
    if (stops_the_program(code))
        return code;
    // A word's 16-bit code straight to its library: table[code], one load, no
    // search. call_number.hpp promises a name is never looked up while a
    // program runs, and this is how that is kept now a word is a code.
    code = functions.build(index, state);
    if (stops_the_program(code))
        return code;

    // The threads are needed from here: load_program tokenises on them. This is
    // where main pays whatever is LEFT of the 12 ms, which is usually none of it.
    code = threads.wait_until_warm(state);
    if (stops_the_program(code))
        return code;

    // THE PROMPT (PLAN M0.6), and everything it needs is now up: the index is
    // loaded, the function table is built, and the threads that tokenise a typed
    // line are warm. There is no file to load and no main to find -- a typed line
    // is its own row, checked and walked by the same two functions a capsule's
    // body is (session.hpp).
    if (command_line.command == Command::repl)
        return run_session(arguments, functions, threads, state);

    state.set("satellite(loading)", satellite_loading_successful);

    // THE PROGRAM IS 16-BIT TOKENS, AND THAT IS WHAT RUNS (the author,
    // 2026-09-16: "we specifically build this into the interpreter"). The main
    // .satl and every file its includes name become one row each, tokenised on
    // the threads that are already warm; then main is walked straight out of
    // those codes. No tree is built and nothing is allocated to run a line.
    code = load_program(arguments.text("arguments.file"), threads, startup,
                        bytecode_registry, bytecode_filenames, state);
    if (stops_the_program(code))
        return code;

    // WHAT A REPORT READS TO SAY WHERE. SATELLITE_ERROR E6: set once, here,
    // after the program is loaded and before anything runs, and never written
    // again. Both outlive the run -- they are this function's own locals and the
    // walker returns into it -- so the pointers cannot dangle while a report
    // could still be raised.
    state.program = &bytecode_registry;
    state.program_files = &bytecode_filenames;

    // THE ONE FILE (the author, 2026-09-16). `.satc`, `.satb` and `.sati` are
    // skipped: the 16-bit bytecode is already the numbered program, already
    // carries its strings inline and counted, and already reserves combine's
    // batch marks. `arguments.sate` is the only flag, and saving is a side
    // effect of a run rather than a pass of its own -- the codes are in memory
    // by here whether they are written or not.
    if (arguments.flag("arguments.sate")) {
        const signed long long int written =
            write_sate_file(sate_path_of(arguments.text("arguments.file")),
                            bytecode_registry, bytecode_filenames, state);
        if (stops_the_program(written))
            return written;
    }

    // No globals: a file must say it is runnable and must have a main to begin
    // in and a return to end in.
    code = file_can_run(bytecode_registry.front(), bytecode_filenames.front(), state);
    if (stops_the_program(code))
        return code;

    // THE CHECKER IS OWED, AND IT IS THE NEXT PIECE. check.sh asserts "nothing
    // ran before the refusal", and the prototype earned that by choosing every
    // call's function up front (compile_satl). This path discovers as it goes,
    // so four of check.sh's 32 now fail: a line with no scenario, "a" + "b", a
    // number too large, and nothing-ran-first. They fail HONESTLY -- the
    // programs they check really are unchecked here, and the bytecode has no
    // pre-pass yet.
    //
    // compile_satl cannot be borrowed for the verdict: it is the PROTOTYPE's
    // checker and refuses a user's own capsule and every include spelling past
    // the first, so it rejected test_programs/hello_world.satl outright (13).
    // The check belongs on the bytecode, walking every capsule body before main
    // is entered, and that is PLAN work rather than a five-line change.
    const CapsuleTable capsules = capsules_in(bytecode_registry);
    // NOTHING RUNS BEFORE THE WHOLE PROGRAM IS CHECKED.
    code = check_program(bytecode_registry, capsules, functions, state);
    if (stops_the_program(code))
        return code;

    // THROUGH THE SWITCH HIERARCHY (the author, 2026-09-18), and every one of
    // its eight leaves calls the same run_main -- *"we are just running the exact
    // same interpreter a bunch of different ways, we don't have to change the
    // actual interpreter right now"*. The choice is made ONCE, here, outside
    // every loop, which is the property that measured at the floor: a nested
    // switch wrapping the loop costs 0.216 ns against a 0.216 ns floor, where the
    // same switch taken per statement costs 0.708. config/feature_switch.hpp
    // carries the numbers and the reason it switches on TIERS and not on
    // features -- fourteen features one to a switch would be 16,384 leaves.
    if (state.debug_mode)
        state.set(std::string("features.plan = ") + plan_name(plan_for(features)), success);
    code = run_through_the_hierarchy(features, bytecode_registry, capsules, functions, state);
    if (stops_the_program(code))
        return code;

    // THE `word_counts` BIT'S ANSWER, printed when the run is over rather than as
    // it goes: a profile is a thing you read after, and the hot path must not pay
    // for the order a person wants it in.
    if (features.on(Feature::word_counts)) {
        std::cerr << "\nsatl: per-word call counts (features." << feature_facts()[
            static_cast<unsigned>(Feature::word_counts)].name << ")\n";
        std::cerr << word_counts_table(word_counts());
        std::cerr.flush();
    }

    // THE `statements` BIT'S ANSWER. Printed after the run whether it stopped or
    // finished, because "what was it doing" is the same question either way --
    // and on a failure it is the whole point of having kept them.
    if (features.on(Feature::statements)) {
        std::cerr << "\nsatl: the statement ring (features.statements)\n";
        std::cerr << statement_ring_table(statement_ring(), bytecode_registry, bytecode_filenames, 12);
        std::cerr.flush();
    }

    // WHAT WAS HELD BACK, SAID ONCE. Anything reported more than once printed
    // the first time and was counted after that; this is the count.
    std::cerr << report_tally().repeats();

    // A REFUSED WRITE IS ONLY REFUSED AT THE FLUSH. std::cout buffers, so
    // writing to a full disk succeeds line by line and fails once, here --
    // which is why run_calls ended the same way (satl_file.cpp:211). Without
    // this the program exits 0 having printed nothing, silently.
    std::cout.flush();
    if (!std::cout)
        return report_error("satl.run(error): the output refused the last lines", display_error);
    return code;
}

namespace {

// THE PARACHUTE, AND THE REASON S999 NEEDS ONE.
//
// S999 is "the machine would not give satl memory", and reporting it MEANS
// ALLOCATING: the report builds strings, the renderer wraps them, iostreams want
// a buffer. So the one failure at the top of the scale is the one where the
// reporter is likeliest to fail too, and a report that throws while reporting an
// out-of-memory is a core dump where a sentence should be.
//
// So a block is taken at start-up and handed back the moment `new` first fails.
// 64 KiB is far more than the report needs and small enough that nobody notices
// it; what it buys is that the whole way down -- the S-code, the sentence, the
// flush -- runs in memory that was already ours.
//
// TOUCHED, NOT JUST ASKED FOR. Linux hands out address space and no pages until
// something writes to them (SATELLITE_ARGUMENTS measured exactly this with
// threads), so a block that is never written is a block that is not really there
// when it is wanted. One pass writing a byte a page makes it real.
constexpr std::size_t kParachuteBytes = 64 * 1024;
char *parachute = nullptr;

void take_the_parachute()
{
    parachute = static_cast<char *>(std::malloc(kParachuteBytes));
    if (parachute == nullptr)
        return;                      // no memory even now; the handler copes
    for (std::size_t at = 0; at < kParachuteBytes; at += 4096)
        parachute[at] = 1;           // make the pages real, not promised
}

// `new` failed. Give the block back and let the throw happen, so the catch in
// main() reports through the ordinary path with room to do it in.
void out_of_memory_handler()
{
    if (parachute != nullptr) {
        std::free(parachute);
        parachute = nullptr;
        return;                      // one more try, now that there is room
    }
    std::set_new_handler(nullptr);   // nothing left to give: let it throw
}

} // namespace

int main(int argc, char **argv)
{
    std::ios::sync_with_stdio(false);

    // A closed pipe is a refused write, reported with its machine code, never a
    // silent death by SIGPIPE (ERROR #3; the start-up block made it happen before
    // any program ran, found by review 2026-09-15).
    std::signal(SIGPIPE, SIG_IGN);

    take_the_parachute();
    std::set_new_handler(&out_of_memory_handler);

    try {
        const signed long long int code = run_satl(argc, argv);
        // THE RUN DOES NOT END WHILE A WINDOW IS OPEN (SATELLITE_WINDOW.md WIN-3).
        // A program that opens a window and returns would otherwise take it down
        // with it before anybody saw it -- and the author's own example is four
        // lines long. HERE AND NOT IN run_satl: that function returns from two
        // dozen places, and a wait written at each of them is a wait that will be
        // missed from the next one added.
        //
        // IT COSTS NOTHING WHEN THERE IS NO WINDOW. The desk is not started until
        // a window word runs, and this answers at once when it was never started
        // -- so `satl batch.satl` on a headless server is untouched.
        // A REFUSED RUN TAKES ITS WINDOWS DOWN rather than waiting on them: the
        // report is already printed, and a person told their program stopped
        // must not then be left at a prompt that never comes back.
        satellite004::windows_hold_the_run_open(!satellite004::stops_the_program(code));
        return satellite004::exit_status_of(code);
    } catch (const std::bad_alloc &) {
        // S999, THE TOP OF THE SCALE. Before this, an allocation that failed was
        // std::terminate and a core dump -- the one failure a person could learn
        // nothing at all from.
        satellite004::CriticalReport report;
        const satellite004::SCode named = satellite004::s_code_for(satellite004::out_of_memory);
        report.code = named.code;
        report.name = named.name;
        report.description = named.means;
        report.notes.push_back("machine code 48 out_of_memory -- satl exits with this.");
        satellite004::print_critical(report);
        return satellite004::exit_status_of(satellite004::out_of_memory);
    }
}
