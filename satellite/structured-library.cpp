// satellite-004 -- the interpreter built from numbered libraries.
//
//     build/satellite-004 [--debug] program.satl
//     build/satellite-004 --version | -V     the title lines: version, revision, build, compiler
//     build/satellite-004 --help | -h        the start-up block and how to run a program
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
// the program stopped on: 0 when it ran to the end.
//
// (the author, 2026-09-16) "First start 256 threads, then load the tiny C++
// libraries, then convert the .satl to 16-bit." That order is the order below,
// and the tokens are the first thing built out of a program.

#include "arguments/arguments.hpp"
#include "bytecode/bytecode_registry.hpp"
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

int main(int argc, char **argv)
{
    using namespace satellite004;

    std::ios::sync_with_stdio(false);

    // A closed pipe is a refused write, reported with its machine code, never a
    // silent death by SIGPIPE (ERROR #3; the start-up block made it happen before
    // any program ran, found by review 2026-09-15).
    std::signal(SIGPIPE, SIG_IGN);

    // The author's satellite_config.hpp first: the title lines need its numbers.
    Arguments arguments;
    signed long long int code = arguments.gather_config();
    if (stops_the_program(code))
        return static_cast<int>(code);

    // Only as the first word, so a program's own words are never taken (PLAN M0.5).
    const std::string first = argc > 1 ? argv[1] : "";
    if (first == "--version" || first == "-V" || first == "--help" || first == "-h") {
        if (argc > 2)
            return static_cast<int>(report_error("satl(command line): " + first + " takes no other words, and \"" +
                                                     argv[2] + "\" was given", command_line_not_understood));
        if (first == "--version" || first == "-V")
            std::cout << title_lines(arguments);
        else
            std::cout << startup_block(arguments) <<
                         "    satl <program.satl>            run a program\n"
                         "    satl --debug <program.satl>    run it, showing every state and argument\n"
                         "    satl --version                 the version, revision and build\n"
                         "    satl --help                    this\n";
        std::cout.flush();
        if (!std::cout)
            return static_cast<int>(report_error("satl(" + first + "): the output refused the lines", display_error));
        return success;
    }

    MachineState state;
    code = arguments.gather(argc, argv);
    if (stops_the_program(code))
        return static_cast<int>(code);
    if (arguments.flag("arguments.startup_display"))
        std::cerr << startup_block(arguments);
    state.debug_mode = arguments.flag("arguments.debug_mode");
    state.set("satellite " + version_line(arguments) + " (starting)", success);
    state.set("arguments(gathered)", success);
    if (state.debug_mode)
        display_arguments(arguments, state);

    // The start-up threads, warm before anything else loads (the author, 2026-09-15).
    // Never more than arguments.threads_max; a refused thread is reported, and the
    // program still runs on the threads that started.
    unsigned long long int startup = static_cast<unsigned long long int>(arguments.number("arguments.threads_startup"));
    if (arguments.find("arguments.threads_max") != nullptr &&
        startup > static_cast<unsigned long long int>(arguments.number("arguments.threads_max"))) {
        const unsigned long long int threads_max = static_cast<unsigned long long int>(arguments.number("arguments.threads_max"));
        // Shown every time, not only in debug mode: the author asked for more threads than start.
        report_error("threads.startup(capped): arguments.threads_startup " + std::to_string(startup) +
                         " is more than arguments.threads_max " + std::to_string(threads_max) + ", so " +
                         std::to_string(threads_max) + " threads start",
                     success);
        startup = threads_max;
    }

    // Declared BEFORE the threads, so they are destroyed after the threads have
    // run every queued job and stopped: a job may point into them (PLAN M7).
    NumberIndex index;
    std::string source;
    std::vector<Call> calls;
    BytecodeRegistry bytecode_registry;
    BytecodeFilenames bytecode_filenames;

    StartupThreads threads;
    threads.start(startup, state);

    code = index.load(numbers_folder(), state);
    if (stops_the_program(code))
        return static_cast<int>(code);
    state.set("satellite(loading)", satellite_loading_successful);

    code = load_satl(arguments.text("arguments.file"), source, state);
    if (stops_the_program(code))
        return static_cast<int>(code);

    code = check_satl(source, state);
    if (stops_the_program(code))
        return static_cast<int>(code);

    // The program becomes 16-bit tokens here, on the threads that are already
    // warm, in batches of lines. Nothing has looked at what the program SAYS
    // yet -- this is the first thing built out of it. A character the registry
    // has no code for is marked with error_token and does not stop the run
    // (the lexer never throws), so its code is reported and the run goes on.
    code = build_bytecode_registry(arguments.text("arguments.file"), source, threads, startup,
                                   bytecode_registry, bytecode_filenames, state);
    if (stops_the_program(code))
        return static_cast<int>(code);

    code = compile_satl(source, index, calls, state);
    if (stops_the_program(code))
        return static_cast<int>(code);

    code = run_calls(calls, state);
    return static_cast<int>(code);
}
