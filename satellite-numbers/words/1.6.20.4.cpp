// satellite-numbers/words/1.6.20.4.cpp -- satellite.variable.fraction.reverse, built into satl
//
// satellite.variable.fraction.reverse   1 6 20 4
// A fraction turned round -- NOT BUILT, and what it answers is not decided (whether 1/3 gives 3/1).
//
// NUMBERED AND NOT BUILT (2026-10-11; the author's I that evening: "yes" -- a fraction's .reverse() numbered
// now, said "not built yet" until it is built). `q.satellite.reverse()` runs this number, and
// run_scalar_number (bytecode/scalar_calls.cpp) finds nothing here to run and says so: "q.satellite.reverse is
// not built for a fraction yet"; the bare q.reverse() is refused before anything runs, naming it. A fraction had
// 1 6 20 1 to 3, and took the next free number, 4.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_20_4(LibraryRow *row)
{
    row->name = "satellite.variable.fraction.reverse";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 20;
    row->numbers[3] = 4;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
