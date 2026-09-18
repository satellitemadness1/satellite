// satellite.library.main.arguments.ram  `1 14 1 1 16` -- the machine's whole memory.
//
// AN ALIAS OF `arguments.memory.total`, on the author's word (2026-09-18):
// *"let's use the longer choice for each one, can we have an alias for them
// though?"* The LONG name is the word; this is a second way to write it.
//
// AN ALIAS IS A SECOND ROW AND A SECOND .so, because a code is one number and
// one library -- but it is NEVER a second copy of the answer. Both point at
// machine_facts.hpp's answer_memory_total(), so the two cannot answer differently,
// which is the only way an alias really goes wrong.

#include "../machine_facts.hpp"

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    if (row == nullptr) return satellite004::error;
    row->name = "satellite.library.main.arguments.ram";
    row->numbers[0] = 1;
    row->numbers[1] = 14;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->numbers[4] = 16;
    row->depth = 5;
    row->scenarios.fact = &satellite004::machine_facts::answer_memory_total;
    return satellite004::success;
}
