// satellite-numbers/words/1.6.11.5.cpp -- satellite.variable.hex.to_binary, built into satl
//
// satellite.variable.hex.to_binary   1 6 11 5
// A hex's bits as text -- THE SECOND NAME OF 1 6 11 9, satellite.variable.hex.binary.
//
// NOTHING OF ITS OWN RUNS HERE (2026-10-11). `.binary`, `.bin` and `.as_binary` (003's to_binary is no spelling
// 004's lexer has) are one code by the time the walker sees them -- the lexer folds a second spelling into the
// first -- and that code runs 1 6 11 9, as a string's runs its .string, .number, .binary and .hex rows
// (1 6 1 19-22), so a function here would be a second copy nothing could reach. The word keeps its number,
// which never moves.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_11_5(LibraryRow *row)
{
    row->name = "satellite.variable.hex.to_binary";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 11;
    row->numbers[3] = 5;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
