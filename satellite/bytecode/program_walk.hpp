#pragma once
// satellite/bytecode/program_walk.hpp -- the whole program, loaded and run out
// of bytecode_registry.
//
// THREE PIECES, AND EACH IS WHY A PART OF THE REGISTRY IS SHAPED AS IT IS:
//
//   load_program()  fills the registry, ONE ROW A FILE -- the main .satl first,
//                   then every file its includes name, and their includes after
//                   them. This is what the second vector was always for (the
//                   author, 2026-09-16: "we have to take in other satellite
//                   files, like other includes").
//
//   CapsuleTable    name -> where its body starts. The language's own words are
//                   found by CODE in the function table; a user's capsules are
//                   found by NAME here, because a user's name has no number.
//                   Two tables, two kinds of name, no search in either.
//
//   run_main()      walks main's body. A call is a POSITION, never an object:
//                   nothing is allocated to run a line, which is the whole
//                   difference between this and 003's tree.
//
// THERE ARE NO GLOBALS (the author, 2026-09-16): "we begin exe inside of main,
// and end exe inside of main... the only globals are the includes, other
// files". So load_program reads a file's includes and its capsule declarations
// and NOTHING else -- a statement outside a capsule has nowhere to put its
// result and no moment to run in, and file_can_run() refuses such a file.

#include "bytecode_registry.hpp"
#include "function_table.hpp"
#include "include_shape.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace satellite004 {

// Where a capsule's body begins: which row, and the code just past its `{`.
struct CapsuleSite {
    std::size_t row = 0;
    std::size_t body = 0;   // the first code INSIDE the braces
};

using CapsuleTable = std::unordered_map<std::string, CapsuleSite>;

// Reads `main_file` and every file its includes name, breadth first, one row a
// file. A file already loaded is not loaded twice, so a cycle of includes ends
// instead of running forever. Answers success, or the code load_satl gave.
signed long long int load_program(const std::string &main_file,
                                  StartupThreads &threads,
                                  unsigned long long int batches,
                                  BytecodeRegistry &registry,
                                  BytecodeFilenames &filenames,
                                  MachineState &state);

// Every `satellite.capsule <name>()` in every row, by name. satellite.main is
// in here too, under "satellite.main".
CapsuleTable capsules_in(const BytecodeRegistry &registry);

// Runs satellite.main's body, and whatever it calls. Answers success, or the
// machine code the program stopped on.
signed long long int run_main(const BytecodeRegistry &registry,
                              const CapsuleTable &capsules,
                              const FunctionTable &functions,
                              MachineState &state);

} // namespace satellite004
