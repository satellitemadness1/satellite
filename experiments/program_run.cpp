// The whole program, out of the bytecode: includes loaded, capsules found, main run.
#include "bytecode/program_walk.hpp"
#include "bytecode/word_codes.hpp"
#include <cstdio>
using namespace satellite004;

int main(int argc, char **argv)
{
    const std::string path = argc > 1 ? argv[1] : "test_programs/hello_world.satl";
    MachineState state;
    state.debug_mode = false;

    StartupThreads threads;
    threads.start(64, state);
    NumberIndex index;
    index.load("build/satellite-numbers", state);
    FunctionTable functions;
    functions.build(index, state);

    BytecodeRegistry registry;
    BytecodeFilenames filenames;
    if (stops_the_program(load_program(path, threads, 64, registry, filenames, state))) return 1;

    printf("loaded %zu files, one row each:\n", registry.size());
    for (std::size_t i = 0; i < registry.size(); ++i)
        printf("   row %zu  %-44s %zu codes\n", i, filenames[i].c_str(), registry[i].size());

    if (stops_the_program(file_can_run(registry[0], filenames[0], state))) return 1;

    const CapsuleTable capsules = capsules_in(registry);
    printf("\n%zu capsules, found by NAME (a user's name has no number):\n", capsules.size());
    for (const auto &c : capsules)
        printf("   %-24s row %zu, body at code %zu\n", c.first.c_str(), c.second.row, c.second.body);

    printf("\nrunning satellite.main:\n");
    const signed long long int code = run_main(registry, capsules, functions, state);
    printf("\nmachine code %lld\n", code);
    return static_cast<int>(code);
}
