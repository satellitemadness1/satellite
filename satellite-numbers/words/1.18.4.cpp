// satellite-numbers/words/1.18.4.cpp -- satellite.directory.list(), built into satl
//
// satellite.directory.list()   1 18 4
// The names in the working directory, sorted, `.` and `..` dropped and real dotfiles kept.
// Typed alone at the prompt it draws the table instead (satellite/satl/listing.hpp).
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_18_4, is the one
// word_table.cpp calls for 1 18 4 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. What it does is directory_words.hpp, shared with the other
// two directory words; written 2026-09-17 for PLAN M0.6.

#include "../directory_words.hpp"

namespace satellite004::built_in {

signed long long int describe_1_18_4(LibraryRow *row)
{
    row->name = "satellite.directory.list()";
    row->numbers[0] = 1;
    row->numbers[1] = 18;
    row->numbers[2] = 4;
    row->depth = 3;
    row->scenarios.directory = satellite004::directory_words::list;
    return satellite004::success;
}

} // namespace satellite004::built_in
