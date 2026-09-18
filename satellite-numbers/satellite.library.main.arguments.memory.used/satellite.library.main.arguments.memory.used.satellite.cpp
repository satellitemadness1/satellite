// satellite.library.main.arguments.memory.used  `1 14 1 1 2 3` -- what this machine is using, in bytes.
//
// SATELLITE_ARGUMENTS B7-B11: one of "the values that are free" -- a /proc or
// sysconf read needing a library and a row rather than a new reader. The reader
// is in satellite-numbers/machine_facts.hpp, shared because each library is
// compiled from exactly one .cpp.
//
// READ EVERY TIME, NEVER CACHED -- which matters most for THIS word: memory that
// was free a minute ago is a wrong answer wearing a right answer's face.
//
// A FAILURE IS SAID, NOT ANSWERED AS 0: machine_fact_not_read (36), naming what
// could not be read, because 0 is a number a program would divide by.

#include "../machine_facts.hpp"

namespace {

using satellite004::FactReply;

FactReply answer_arguments_memory_used()
{
    unsigned long long int whole = 0, spare = 0;
    if (satellite004::machine_facts::meminfo_bytes("MemTotal:", whole) == false ||
        satellite004::machine_facts::meminfo_bytes("MemAvailable:", spare) == false)
        return satellite004::machine_facts::could_not_read("MemTotal and MemAvailable in /proc/meminfo",
                                                           satellite004::machine_fact_not_read);
    // USED IS TOTAL LESS AVAILABLE, not total less MemFree. MemFree leaves out
    // the page cache, which the kernel hands back the moment anything wants it --
    // so `free` off MemFree reads as almost nothing on a machine that is fine,
    // and `used` off it reads as almost everything. MemAvailable is the kernel's
    // own estimate of what a program could actually get.
    return satellite004::machine_facts::a_count(spare > whole ? 0 : whole - spare);
}

} // namespace

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    if (row == nullptr) return satellite004::error;
    row->name = "satellite.library.main.arguments.memory.used";
    row->numbers[0] = 1;
    row->numbers[1] = 14;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->numbers[4] = 2;
    row->numbers[5] = 3;
    row->depth = 6;
    row->scenarios.fact = &answer_arguments_memory_used;
    return satellite004::success;
}
