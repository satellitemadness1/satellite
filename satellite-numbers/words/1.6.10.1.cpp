// satellite-numbers/words/1.6.10.1.cpp -- satellite.variable.float.to_string, built into satl
//
// satellite.variable.float.to_string   1 6 10 1
// The float as text -- THE SECOND NAME OF 1 6 10 2, satellite.variable.float.string.
//
// NOTHING OF ITS OWN RUNS HERE (2026-10-11). `.to_string`, `.str` and `.string` are one code by the time the
// walker sees them -- the lexer folds a second spelling into the first -- and that code runs 1 6 10 2, as a
// string's runs its .string, .number, .binary and .hex rows (1 6 1 19-22), so a function here would be a second
// copy nothing could reach. The word keeps its number, which never moves.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_10_1(LibraryRow *row)
{
    row->name = "satellite.variable.float.to_string";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 10;
    row->numbers[3] = 1;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
