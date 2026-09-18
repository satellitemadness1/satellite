// satellite.library.main.arguments.machine.threads  `1 14 1 1 1 3` -- how many threads this machine allows.
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

#include "../../satellite/config/machine_probe.hpp"

#include "../machine_facts.hpp"

namespace {

using satellite004::FactReply;

FactReply answer_arguments_machine_threads()
{
    // C6 -- THE MEASURED COUNT WHEN `satl --config` HAS RUN HERE, and the lowest
    // ceiling /proc states when it has not. Never probes: reading this word must
    // not cost nine seconds and three gigabytes, which is the whole reason
    // --config is a separate once-per-machine command.
    const unsigned long long int said = satellite004::threads_this_machine_allows();
    if (said == 0)
        return satellite004::machine_facts::could_not_read("any thread ceiling",
                                                           satellite004::machine_fact_not_read);
    return satellite004::machine_facts::a_count(said);
}

} // namespace

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
    row->scenarios.fact = &answer_arguments_machine_threads;
    return satellite004::success;
}
