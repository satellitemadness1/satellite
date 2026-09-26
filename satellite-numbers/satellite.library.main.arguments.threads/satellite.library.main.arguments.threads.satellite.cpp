// satellite.library.main.arguments.threads  `1 14 1 1 14` -- how many threads the interpreter
// may create on this machine.
//
// A WORD OF ITS OWN SINCE 2026-09-25. The author: *"I want arguments.threads or
// arguments.thread = how many the interpreter can create, and arguments.machine.thread =
// how many physical threads exist on the machine"*. It was an alias of
// arguments.machine.threads (his 2026-09-18 ruling for the pairs), and the two now answer
// different questions; `arguments.thread`, singular, is this word's second spelling
// (words/aliases.tsv).

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
