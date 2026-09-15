// satellite-004 -- the interpreter built from numbered libraries.
//
//     build/satellite-004 [--debug] program.satl
//     build/satellite-004 --version          satellite 004 revision 02
//
// Start-up loads every compiled library in build/satellite-numbers/ into the
// number index; then the .satl file is loaded, checked, turned into calls with
// their functions already chosen, and run. The exit status is the machine code
// the program stopped on: 0 when it ran to the end.

#include "arguments.hpp"
#include "machine_codes.hpp"
#include "machine_state.hpp"
#include "satellite-numbers/call_number.hpp"
#include "satl_file.hpp"
#include "version.hpp"

#include <iostream>
#include <string>
#include <vector>

#include <climits>
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

    for (int i = 1; i < argc; i++)
        if (std::strcmp(argv[i], "--version") == 0) {
            std::cout << "satellite " << kVersion << " revision " << kRevision << "\n";
            return 0;
        }

    Arguments arguments;
    MachineState state;
    arguments.gather(argc, argv);
    state.debug_mode = arguments.flag("arguments.debug_mode");
    state.set(std::string("satellite ") + kVersion + " revision " + kRevision + " (starting)", success);
    state.set("arguments(gathered)", success);
    if (state.debug_mode)
        display_arguments(arguments, state);

    NumberIndex index;
    signed long long int code = index.load(numbers_folder(), state);
    if (stops_the_program(code))
        return static_cast<int>(code);
    state.set("satellite(loading)", satellite_loading_successful);

    std::string source;
    code = load_satl(arguments.text("arguments.file"), source, state);
    if (stops_the_program(code))
        return static_cast<int>(code);

    code = check_satl(source, state);
    if (stops_the_program(code))
        return static_cast<int>(code);

    std::vector<Call> calls;
    code = compile_satl(source, index, calls, state);
    if (stops_the_program(code))
        return static_cast<int>(code);

    code = run_calls(calls, state);
    return static_cast<int>(code);
}
