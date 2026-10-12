// satellite-numbers/words/1.6.5.6.cpp -- satellite.variable.binary.to_hex, built into satl
//
// satellite.variable.binary.to_hex   1 6 5 6
// A binary's worth in base 16, as text -- THE SECOND NAME OF 1 6 5 11, satellite.variable.binary.hex.
//
// NOTHING OF ITS OWN RUNS HERE (2026-10-11). `.to_hex`, `.as_hex` and `.hex` are one code by the time the
// walker sees them -- the lexer folds a second spelling into the first -- and that code runs 1 6 5 11, as a
// string's runs its .string, .number, .binary and .hex rows (1 6 1 19-22), so a function here would be a second
// copy nothing could reach. The word keeps its number, which never moves.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_5_6(LibraryRow *row)
{
    row->name = "satellite.variable.binary.to_hex";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 5;
    row->numbers[3] = 6;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
