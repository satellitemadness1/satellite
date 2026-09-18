// satellite.library.main.arguments.directory  `1 14 1 1 12` -- the directory this interpreter is in now.
//
// SATELLITE_ARGUMENTS Phase C. The reader and the answer are in
// satellite-numbers/machine_facts.hpp, shared because each library is compiled
// from exactly one .cpp -- and because this word has aliases that must not be
// able to answer anything different.
//
// READ EVERY TIME, NEVER CACHED. A machine fact a minute old is a wrong answer
// wearing a right answer's face.

#include "../machine_facts.hpp"

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    if (row == nullptr) return satellite004::error;
    row->name = "satellite.library.main.arguments.directory";
    row->numbers[0] = 1;
    row->numbers[1] = 14;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->numbers[4] = 12;
    row->depth = 5;
    row->scenarios.fact = &satellite004::machine_facts::answer_directory;
    return satellite004::success;
}
