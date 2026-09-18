// satellite.library.main.arguments.threads  `1 14 1 1 14` -- how many threads this machine allows.
//
// AN ALIAS OF `arguments.machine.threads`, on the author's word (2026-09-18):
// *"let's use the longer choice for each one, can we have an alias for them
// though?"* The LONG name is the word; this is a second way to write it.
//
// AN ALIAS IS A SECOND ROW AND A SECOND .so, because a code is one number and
// one library -- but it is NEVER a second copy of the answer. Both point at
// machine_facts.hpp's answer_threads(), so the two cannot answer differently,
// which is the only way an alias really goes wrong.

#include "../machine_facts.hpp"
#include "../../satellite/config/machine_probe.hpp"

inline satellite004::FactReply answer_threads()
{
    // C6 -- THE MEASURED COUNT WHEN `satl --config` HAS RUN HERE, and the lowest
    // ceiling /proc states when it has not. NEVER PROBES: reading a word must not
    // cost nine seconds and three gigabytes, which is the whole reason --config
    // is a separate, once-per-machine command.
    const unsigned long long int said = satellite004::threads_this_machine_allows();
    if (said == 0)
        return satellite004::machine_facts::could_not_read("any thread ceiling",
                                                           satellite004::machine_fact_not_read);
    return satellite004::machine_facts::a_count(said);
}

extern "C" signed long long int satellite_number_describe(satellite004::LibraryRow *row)
{
    if (row == nullptr) return satellite004::error;
    row->name = "satellite.library.main.arguments.threads";
    row->numbers[0] = 1;
    row->numbers[1] = 14;
    row->numbers[2] = 1;
    row->numbers[3] = 1;
    row->numbers[4] = 14;
    row->depth = 5;
    row->scenarios.fact = &answer_threads;
    return satellite004::success;
}
