// satellite-numbers/words/1.6.5.12.cpp -- satellite.variable.binary.clear, built into satl
//
// satellite.variable.binary.clear   1 6 5 12
// 003's row for setting a binary's bits to 0 -- NOT BUILT in 004.
//
// NUMBERED AND NOT BUILT (2026-10-11). `b.satellite.clear` runs this number, and run_scalar_number
// (bytecode/scalar_calls.cpp) finds nothing here to run and says so: "b.satellite.clear is not built for a
// binary yet".

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_5_12(LibraryRow *row)
{
    row->name = "satellite.variable.binary.clear";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 5;
    row->numbers[3] = 12;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
