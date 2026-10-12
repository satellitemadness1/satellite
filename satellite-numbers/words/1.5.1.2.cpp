// satellite-numbers/words/1.5.1.2.cpp -- satellite.console.display.centre, built into satl
//
// satellite.console.display.centre   1 5 1 2
// The display's line in the middle of the console -- THE SECOND NAME OF 1 5 1 1,
// satellite.console.display.center.
//
// NOTHING OF ITS OWN RUNS HERE (2026-10-11). `.centre` and `.center` are one code by the time the walker
// sees them -- the lexer folds the second spelling into the first, and that code's first spelling is
// `center` -- so a program's `.satellite.centre()` runs 1 5 1 1, and a function here would be a second
// copy nothing could reach. The word keeps its number, which never moves.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_5_1_2(LibraryRow *row)
{
    row->name = "satellite.console.display.centre";
    row->numbers[0] = 1;
    row->numbers[1] = 5;
    row->numbers[2] = 1;
    row->numbers[3] = 2;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
