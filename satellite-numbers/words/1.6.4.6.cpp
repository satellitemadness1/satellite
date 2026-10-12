// satellite-numbers/words/1.6.4.6.cpp -- satellite.variable.number.to_string, built into satl
//
// satellite.variable.number.to_string   1 6 4 6
// The number's base 10 text -- THE SECOND NAME OF 1 6 4 17, satellite.variable.number.string.
//
// NOTHING OF ITS OWN RUNS HERE (2026-10-11). `.to_string`, `.str` and `.string` are one code by the time the
// walker sees them -- the lexer folds a second spelling into the first -- and a number's runs 1 6 4 17,
// satellite.variable.number.string, as a string's runs 1 6 1 19 and its 1 6 1 13 is a second name: so a
// function here would be a second copy nothing could reach. The word keeps its number, which never moves.

#include "../number_row.hpp"
#include "../../satellite/machine/machine_codes.hpp"

namespace satellite004::built_in {

signed long long int describe_1_6_4_6(LibraryRow *row)
{
    row->name = "satellite.variable.number.to_string";
    row->numbers[0] = 1;
    row->numbers[1] = 6;
    row->numbers[2] = 4;
    row->numbers[3] = 6;
    row->depth = 4;
    return satellite004::success;
}

} // namespace satellite004::built_in
