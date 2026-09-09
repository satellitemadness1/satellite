// Three implementations of the same loop, timed side by side.
//
// THE PROGRAM IS 100,000 ITERATIONS OF { compare, add, assign } and not a
// "hello world", whatever the filenames say. That matters for reading the
// numbers: satl's are exact arbitrary-precision decimals (DESIGN §8.1) and
// CPython's are bignums too, so those two are comparable, while the C++ is
// `long long` at -O0 and is a floor rather than a rival.
//
// EVERY COMMAND'S EXIT STATUS IS REPORTED, because a run that failed still
// takes time and would otherwise print as if it had worked. That is exactly
// what this harness used to do: hello_world.satl declares a satellite.spacesuit
// and PLAN §8 does not build those until M26, so the satl row is a REFUSAL
// being timed. It says so now rather than showing a number that means nothing.

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Program {
    const char *name;
    std::string command;
};

// Wall clock around one run, plus what the child exited with.
//
// std::system returns a wait status and not an exit code -- 256 for a program
// that returned 1 -- so it is decoded here. A harness that printed the raw
// number would report "exit 256" for the ordinary failure case.
struct Result {
    double seconds;
    int status;
};

Result run(const std::string &command)
{
    const auto start = std::chrono::steady_clock::now();
    const int raw = std::system(command.c_str());
    const auto end = std::chrono::steady_clock::now();

    int status = -1;
    if (raw != -1 && WIFEXITED(raw))
        status = WEXITSTATUS(raw);

    return Result{std::chrono::duration<double>(end - start).count(), status};
}

} // namespace

int main()
{
    // THREE ENTRIES AND THREE USES. This vector held two while the code below
    // read commands[2], which is out of bounds on a std::vector -- no check, no
    // crash, just whatever bytes followed it handed to std::system(). Kept in
    // the comment because the fix is invisible otherwise: the bug was a missing
    // ROW, not a wrong index.
    const std::vector<Program> programs = {
        {"satl", "../satl ./hello_world.satl"},
        {"cxx", "./hello_world.cxx"},
        {"py", "python3 ./hello_world.py"},
    };

    for (const Program &program : programs) {
        const Result result = run(program.command);
        std::cout << program.name << ": " << result.seconds << " seconds";
        if (result.status != 0)
            std::cout << "   <-- EXIT " << result.status
                      << ", so this time measures a FAILURE and not the loop";
        std::cout << "\n";
    }

    std::cout << "\n";
    return 0;
}
