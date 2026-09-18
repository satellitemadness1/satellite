// satellite.library.main.arguments.session.language  `1 14 1 1 8 3` -- the language this session is set to.
//
// SATELLITE_ARGUMENTS Phase C, one of "the values that are free": a uname field,
// a sysconf call, a getenv or an /etc file -- nothing opened that the machine was
// not already going to tell us. The reader and the answer are in
// satellite-numbers/machine_facts.hpp, shared because each library is compiled
// from exactly one .cpp, and because a word with aliases must not be able to
// answer two different things.
//
// READ EVERY TIME, NEVER CACHED, and a machine that does not state it REFUSES
// with machine_fact_not_read (36) naming what it could not read -- rather than
// answering "" or 0, either of which a program would take for an answer.

#include "../machine_facts.hpp"

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    if (row == nullptr) return satellite004::error;
    row->name = "satellite.library.main.arguments.session.language";
    row->numbers[0] = 1;
    row->numbers[1] = 14;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->numbers[4] = 8;
    row->numbers[5] = 3;
    row->depth = 6;
    row->scenarios.fact = &satellite004::machine_facts::answer_language;
    return satellite004::success;
}
