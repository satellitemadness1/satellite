// satellite-numbers/words/1.6.5.5.cpp -- satellite.variable.binary.digits, built into satl
//
// satellite.variable.binary.digits   1 6 5 5
// How many digits a binary is written with (003 DESIGN 8.5: "width() COUNTS BITS ON BOTH AND digits() COUNTS
// DIGITS") -- 003's row.
//
// REACHED, AND NOT BUILT (2026-10-11): a number's words gave `digits` its method code, so scalar_calls.cpp's
// binary_number_of names this row for it, and run_scalar_number says it is not built for a binary yet
// until its code is written here -- as 1 6 5 2, a binary's .width, is.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_5_5(LibraryRow *row)
{
    row->name = "satellite.variable.binary.digits";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 5;
    row->numbers[3] = 5;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
