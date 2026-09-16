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

// WHAT AN ARGUMENT IS WORTH. The author, 2026-09-16:
// "satellite.console.display(satellite.console.display(\"Hello, World!\")) is
// correct satellite code, and it should work" -- so a call is an argument, its
// answer becomes the outer call's argument, and something has to carry that
// answer between them. This is that something.
//
// ONE KIND PER SCENARIO, on purpose. number_row.hpp has a library export one
// function per KIND of value (text / count / flag), so a Value's Kind is what
// chooses which one runs. It is deliberately not a variant of every satellite
// type yet: satellite_number and satellite_string go in here when the libraries
// take them, and Kind is the seam they arrive at.
//
// A CALL ANSWERS ITS MACHINE CODE, as a count. display("x") prints and answers
// 0, so display(display("x")) prints x and then prints 0. That is the honest
// reading of what a word returns today -- satellite.returns(TYPE) exists as a
// word and nothing declares one yet.
struct Value {
    enum class Kind { nothing, text, count, flag };

    Kind kind = Kind::nothing;
    std::string text;
    unsigned long long int count = 0;
    bool flag = false;
};

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

// NOTHING RUNS BEFORE THE WHOLE PROGRAM IS CHECKED. Every capsule body is
// walked and every statement in it judged BEFORE main is entered, so a program
// that cannot finish does not half-print first -- which check.sh asserts in as
// many words ("nothing ran before the refusal").
//
// Written fresh against the bytecode rather than borrowed (the author,
// 2026-09-16: "forget the prototype, just use 003 or preferably make everything
// new"). The prototype's compile_satl could not do this job: it knows neither a
// user's own capsule nor any include spelling past the first, and refused a
// working program outright.
//
// Answers success, or satl_line_not_understood (13) for a statement there is no
// scenario for, string_error (4) for an argument that is an expression, or
// int_error (3) for a number too large to hold.
signed long long int check_program(const BytecodeRegistry &registry,
                                   const CapsuleTable &capsules,
                                   const FunctionTable &functions,
                                   MachineState &state);

// Runs satellite.main's body, and whatever it calls. Answers success, or the
// machine code the program stopped on.
signed long long int run_main(const BytecodeRegistry &registry,
                              const CapsuleTable &capsules,
                              const FunctionTable &functions,
                              MachineState &state);

} // namespace satellite004
