// satellite-numbers/words/1.18.6.cpp -- satellite.directory.system(), built into satl
//
// satellite.directory.system()   1 18 6
// Where the machine's drives are mounted, one place a drive (the author, 2026-09-25).
// Typed alone at the prompt it draws their space instead (satellite/satl/drives.hpp).
//
// BUILT INTO satl SINCE 2026-10-07 (the author: "built the satellite-numbers directly into the
// interpreter, so they are not external"): the function at the bottom, describe_1_18_6, is the one
// word_table.cpp calls for 1 18 6 at start-up. Until then this was a .so, built by
// build_libraries.py and dlopened. What it does is directory_words.hpp, shared with the other
// directory words.

#include "../directory_words.hpp"

namespace satellite004::built_in {

signed long long int describe_1_18_6(LibraryRow *row)
{
    row->name = "satellite.directory.system()";
    row->numbers[0] = 1;
    row->numbers[1] = 18;
    row->numbers[2] = 6;
    row->depth = 3;
    row->scenarios.directory = satellite004::directory_words::drives;
    return satellite004::success;
}

} // namespace satellite004::built_in
