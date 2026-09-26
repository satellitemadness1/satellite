// satellite.library.main.arguments.machine.threads  `1 14 1 1 1 3` -- how many threads this
// machine's processors run at once: 24 on a 12-core processor running two a core.
//
// THE AUTHOR, 2026-09-25: *"arguments.machine.thread = how many physical threads exist on
// the machine"* -- and `arguments.machine.thread`, singular, is this word's second spelling
// (words/aliases.tsv). How many threads the INTERPRETER may create is arguments.threads, a
// word of its own; this one answered that count until 2026-09-25, while the arguments
// variable's row of the same name said 24.
//
// READ EVERY TIME, NEVER CACHED, and the answer is machine_facts.hpp's, so the row the
// arguments variable gathers and this word cannot say different numbers.

#include "../machine_facts.hpp"

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    if (row == nullptr) return satellite004::error;
    row->name = "satellite.library.main.arguments.machine.threads";
    row->numbers[0] = 1;
    row->numbers[1] = 14;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->numbers[4] = 1;
    row->numbers[5] = 3;
    row->depth = 6;
    row->scenarios.fact = &satellite004::machine_facts::answer_hardware_threads;
    return satellite004::success;
}
