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
#include "bytecode/bytecode_registry.hpp"
#include "bytecode/function_table.hpp"
#include "bytecode/program_walk.hpp"
#include "machine/exit_status.hpp"
#include "machine/machine_codes.hpp"
#include "machine/machine_state.hpp"
#include "../satellite-numbers/call_number.hpp"
#include "satl/satl_file.hpp"
#include "threads/startup_threads.hpp"
#include "version/version.hpp"

#include <iostream>
#include <string>
#include <vector>

#include <climits>
#include <csignal>
#include <cstring>
#include <unistd.h>

namespace {

// The folder the libraries were built into: satellite-numbers/ beside this executable.
std::string numbers_folder()
{
    char path[PATH_MAX];
    const ssize_t length = readlink("/proc/self/exe", path, sizeof path - 1);
    if (length <= 0)
        return "satellite-numbers";
    std::string executable(path, static_cast<size_t>(length));
    return executable.substr(0, executable.rfind('/')) + "/satellite-numbers";
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

    MachineState state;
    code = arguments.gather(command_line);
    if (stops_the_program(code))
        return code;
    if (arguments.flag("arguments.startup_display"))
        std::cerr << startup_block(arguments);

    // THE PROMPT IS M0.6. Said after the start-up block, so a satl-term prompt
    // tab shows which satl answered, and then holds on the code.
    if (command_line.command == Command::repl)
        return report_error("satl --repl: the prompt is not built yet -- it lands at M0.6", not_built_yet);
    state.debug_mode = arguments.flag("arguments.debug_mode");
    state.set("satellite " + version_line(arguments) + " (starting)", success);
    state.set("arguments(gathered)", success);
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

    code = index.load(numbers_folder(), state);
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

    code = run_main(bytecode_registry, capsules, functions, state);
    if (stops_the_program(code))
        return code;

    // A REFUSED WRITE IS ONLY REFUSED AT THE FLUSH. std::cout buffers, so
    // writing to a full disk succeeds line by line and fails once, here --
    // which is why run_calls ended the same way (satl_file.cpp:211). Without
    // this the program exits 0 having printed nothing, silently.
    std::cout.flush();
    if (!std::cout)
        return report_error("satl.run(error): the output refused the last lines", display_error);
    return code;
}

int main(int argc, char **argv)
{
    std::ios::sync_with_stdio(false);

    // A closed pipe is a refused write, reported with its machine code, never a
    // silent death by SIGPIPE (ERROR #3; the start-up block made it happen before
    // any program ran, found by review 2026-09-15).
    std::signal(SIGPIPE, SIG_IGN);

    return satellite004::exit_status_of(run_satl(argc, argv));
}
