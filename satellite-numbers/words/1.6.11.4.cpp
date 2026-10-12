// satellite-numbers/words/1.6.11.4.cpp -- satellite.variable.hex.digits, built into satl
//
// satellite.variable.hex.digits   1 6 11 4
// How many digits a hex is written with (003 DESIGN 8.5: "width() COUNTS BITS ON BOTH AND digits() COUNTS
// DIGITS") -- 003's row.
//
// REACHED, AND NOT BUILT (2026-10-11): a number's words gave `digits` its method code, so scalar_calls.cpp's
// hexadecimal_number_of names this row for it, and run_scalar_number says it is not built for a hex yet
// until its code is written here -- as 1 6 5 2, a binary's .width, is.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_11_4(LibraryRow *row)
{
    row->name = "satellite.variable.hex.digits";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 11;
    row->numbers[3] = 4;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
