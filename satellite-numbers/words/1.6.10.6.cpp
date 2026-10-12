// satellite-numbers/words/1.6.10.6.cpp -- satellite.variable.float.reverse, built into satl
//
// satellite.variable.float.reverse   1 6 10 6
// A float's digits turned round -- SATELLITE_INFINITY.md FLT-4, decided (12.34 gives 34.12) and NOT BUILT.
//
// NUMBERED AND NOT BUILT (2026-10-11; the author's I that evening: "yes" -- a float's .reverse() numbered now,
// said "not built yet" until it is built). `f.satellite.reverse()` runs this number, and run_scalar_number
// (bytecode/scalar_calls.cpp) finds nothing here to run and says so: "f.satellite.reverse is not built for a
// float yet", as a binary's .satellite.width is; the bare f.reverse() is refused before anything runs, naming
// it. A float had 1 6 10 1 to 5, and took the next free number, 6.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_10_6(LibraryRow *row)
{
    row->name = "satellite.variable.float.reverse";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 10;
    row->numbers[3] = 6;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
