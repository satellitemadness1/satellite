// satellite-numbers/words/1.18.1.cpp -- satellite.directory.change(d), built into satl
//
// satellite.directory.change(d)   1 18 1
// Moves the program's working directory. Answers true or false and is NEVER an error, so a
// program may ask about a directory without being stopped (003's rule).
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_18_1, is the one
// word_table.cpp calls for 1 18 1 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. What it does is directory_words.hpp, shared with the other
// two directory words; written 2026-09-17 for PLAN M0.6.

#include "../directory_words.hpp"

namespace satellite004::built_in {

signed long long int describe_1_18_1(LibraryRow *row)
{
    row->name = "satellite.directory.change(d)";
    row->numbers[0] = 1;
    row->numbers[1] = 18;
    row->numbers[2] = 1;
    row->depth = 3;
    row->scenarios.directory = satellite004::directory_words::change;
    return satellite004::success;
}

} // namespace satellite004::built_in
