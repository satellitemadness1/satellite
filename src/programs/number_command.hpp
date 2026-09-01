#pragma once

// `satl --number <a> <op> <b>` -- PLAN M8's consumer.
//
// WHY M8 NEEDS ONE AT ALL. Every milestone in this tree is a thing that works
// and can be demonstrated (PLAN §8's opening), and M6 and M7 each shipped a
// command for it -- `satl --limits` and `satl --resolve`. Nothing EXECUTES
// until M10, so `satellite.variable.number` would otherwise land as a module
// with a test binary and no way for a person to see it.
//
// THREE OPERANDS AND NOT AN EXPRESSION, WHICH IS THE WHOLE OF THE DESIGN. The
// obvious version of this command reads `satl --number "(1+2)*3"` and needs a
// grammar to do it -- and DESIGN §6 is already the grammar, so a second one
// here would be a second place expression syntax is decided, which is the
// failure this project names in `paths.cpp` and in `format.def`'s rule. Two
// literals and an operator is an OPERAND FORM, the same shape
// `satl --limits <file>` already has: it cannot drift from §6 because it is not
// trying to be §6. When M9 compiles expressions and M10 runs them, the whole
// question moves there and this arm is free to stay what it is.
//
// IT BUILDS A SOURCE LINE OUT OF ITS OWN ARGUMENTS so that M5's reporter works
// unchanged -- codes, carets and spans over text the user typed, even though
// what they typed was a command line and not a file. See the .cpp.

#include <string>
#include <vector>

namespace satellite {

// Compute `args[2] args[3] args[4]` and print the result with what it cost.
//
// EXIT_USAGE when the three operands are not there, EXIT_MALFORMED when one of
// them is not a number or the operator is not one satl has, EXIT_NOT_YET for
// `%` -- which is a real operation whose PATH is M15's -- and EXIT_FINE with
// the answer otherwise. The statuses are opening.hpp's and this arm invents
// none of them.
int number_command(const std::vector<std::string> &args);

} // namespace satellite
