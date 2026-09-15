#pragma once

// `satl --compile` and `satl --call` -- M9's two arms.
//
// TWO FLAGS IN ONE FILE, which is the shape programs/dump_commands.cpp and
// file_commands.cpp already have and is the seam main.cpp is split on: an arm
// whose work belongs to a module lives beside the other arms of the same
// module. `--compile` prints what was built and `--call` runs it, and they
// share every line up to that point -- read the file, parse it, resolve it,
// compile it -- so splitting them would be splitting one function.
//
// NEITHER OF THEM RUNS A PROGRAM, AND THAT IS PLAN §8's BOUNDARY RATHER THAN A
// SHORTFALL. M10 is "the milestone at which satellite executes anything at all"
// -- it brings the console, `satellite.main` and `satellite.return`'s three
// shapes -- and `satl --run` still answers that it is not built. What `--call`
// does is name one capsule and print the value it answered, with no console
// behind it and no `main` in front of it, which is exactly `satl --number`'s
// shape one milestone on.

#include <string>
#include <vector>

namespace satellite {

// `satl --compile <file>` -- the closure tree, printed.
int compile_command(const std::string &path);

// `satl --call <file> <capsule> [number...]` -- run one capsule and print what
// it answered. The arguments are numbers because DESIGN §8's other three arms
// cannot be written on a command line without inventing a syntax for them, and
// DESIGN §6 is already the grammar. number_command.hpp makes the same argument
// about `--number` taking three operands rather than an expression.
int call_command(const std::vector<std::string> &args);

} // namespace satellite
