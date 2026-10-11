// satellite-numbers/words/1.18.5.cpp -- satellite.directory.list(d), built into satl
//
// satellite.directory.list(d)   1 18 5
// list's own function given a directory to read. The same answer, about somewhere else.
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_18_5, is the one
// word_table.cpp calls for 1 18 5 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. What it does is directory_words.hpp, shared with the other
// two directory words; written 2026-09-17 for PLAN M0.6.

#include "../directory_words.hpp"

namespace satellite004::built_in {

signed long long int describe_1_18_5(LibraryRow *row)
{
    row->name = "satellite.directory.list(d)";
    row->numbers[0] = 1;
    row->numbers[1] = 18;
    row->numbers[2] = 5;
    row->depth = 3;
    row->scenarios.directory = satellite004::directory_words::list;
    return satellite004::success;
}

} // namespace satellite004::built_in
