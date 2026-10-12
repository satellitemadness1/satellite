// satellite-numbers/words/1.6.5.4.cpp -- satellite.variable.binary.as_number, built into satl
//
// satellite.variable.binary.as_number   1 6 5 4
// A binary's worth, a number -- THE SECOND NAME OF 1 6 5 9, satellite.variable.binary.number.
//
// NOTHING OF ITS OWN RUNS HERE (2026-10-11). `.as_number`, `.to_number` and `.number` are one code by the time
// the walker sees them -- the lexer folds a second spelling into the first -- and that code runs 1 6 5 9, as a
// string's runs its .string, .number, .binary and .hex rows (1 6 1 19-22), so a function here would be a second
// copy nothing could reach. The word keeps its number, which never moves.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_5_4(LibraryRow *row)
{
    row->name = "satellite.variable.binary.as_number";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 5;
    row->numbers[3] = 4;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
