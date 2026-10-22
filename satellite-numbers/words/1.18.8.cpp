// satellite-numbers/words/1.18.8.cpp -- satellite.directory.create(d), built into satl
//
// satellite.directory.create(d)   1 18 8
// Makes the folder and every missing folder above it; one already there is fine. Answers true or false
// and is NEVER an error, as change is. The author's of 2026-10-21 (FILE_AND_DRIVE_PLAN.md FD-1); what it
// does is directory_words.hpp's create.

#include "../directory_words.hpp"

namespace satellite004::built_in {

signed long long int describe_1_18_8(LibraryRow *row)
{
    row->name = "satellite.directory.create(d)";
    row->numbers[0] = 1;
    row->numbers[1] = 18;
    row->numbers[2] = 8;
    row->depth = 3;
    row->scenarios.directory = satellite004::directory_words::create;
    return satellite004::success;
}

} // namespace satellite004::built_in
