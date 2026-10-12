// satellite-numbers/words/1.6.5.2.cpp -- satellite.variable.binary.width, built into satl
//
// satellite.variable.binary.width   1 6 5 2
// How many bits wide a binary is -- 003's row, NOT BUILT in 004.
//
// NUMBERED AND NOT BUILT (2026-10-11). `b.satellite.width` runs this number, and run_scalar_number
// (bytecode/scalar_calls.cpp) finds nothing here to run and says so: "b.satellite.width is not built for a
// binary yet". A hex's .width is built (1 6 11 2) and counts bits, four to the digit, so that a width means the
// same thing on both (003 DESIGN 8.5); a binary's would be its digits.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_5_2(LibraryRow *row)
{
    row->name = "satellite.variable.binary.width";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 5;
    row->numbers[3] = 2;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
