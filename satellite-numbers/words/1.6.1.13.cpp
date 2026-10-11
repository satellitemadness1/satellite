// satellite-numbers/words/1.6.1.13.cpp -- satellite.variable.string.to_number, built into satl
//
// satellite.variable.string.to_number   1 6 1 13
// The text read as a number -- THE SECOND NAME OF 1 6 1 20, satellite.variable.string.number.
//
// NOTHING OF ITS OWN RUNS HERE (2026-10-10). `.to_number`, `.number` and `.as_number` are one code
// by the time the walker sees them -- the lexer folds a second spelling into the first, and that
// code's first spelling is `number` -- so a program's `.to_number` runs 1 6 1 20, and a function
// here would be a second copy nothing could reach. The 32-bit copy of 003's behaviour this file
// held from 2026-09-14 (a .so until 2026-10-07) is gone: where a method had two copies, the
// author kept the one programs ran. The word keeps its number, which never moves.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_1_13(LibraryRow *row)
{
    row->name = "satellite.variable.string.to_number";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 1;
    row->numbers[3] = 13;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
