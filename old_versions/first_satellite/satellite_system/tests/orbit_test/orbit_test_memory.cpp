// Phase 4 — memory, and the rule that keeps it honest.
//
// Split out of orbit_test_phases.cpp when the result type's sections took that
// file past the 325-line ceiling. The seam was already there: everything else in
// that file is five phases over values held in memory, and this is the one that
// touches a FILE ON DISK -- it points the path at a test file of its own,
// forgets it before and after, and would otherwise leave a trail in the user's
// real $HOME/.satl_orbit.
//
// MEMORY PROPOSES, IT NEVER INVENTS is the rule under test. A record whose value
// is not in the corpus in front of us must produce nothing, or a result's
// `value` would be something the searched structure does not contain -- which is
// a lie whatever number is printed beside it.

#include "orbit_test.hpp"

#include <string>

using namespace satellite;

// --- phase 4: memory ---------------------------------------------------------

void orbit_test_memory()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    std::vector<OrbitRecord> empty;
    orbit_memory_read(empty);
    check(empty.empty(), "a missing memory is a memory with nothing in it");

    orbit_memory_record("\"bolt\"", "M8 x 40");
    orbit_memory_record("\"bolt\"", "M8 x 40");

    std::vector<OrbitRecord> records;
    orbit_memory_read(records);
    check(records.size() == 1, "the same pairing twice is one record");
    if (records.size() == 1)
        check(records[0].count == 2, "with a count of two");

    // A field containing a tab and a newline survives the round trip, because a
    // satellite string may hold any byte and the file is one record per line.
    orbit_memory_record("with\ta tab", "and\na newline");
    std::vector<OrbitRecord> awkward;
    orbit_memory_read(awkward);
    check(awkward.size() == 2, "an awkward record is still one line");

    bool round_tripped = false;
    for (const OrbitRecord &record : awkward)
        if (record.pattern == "with\ta tab" && record.resolved == "and\na newline")
            round_tripped = true;
    check(round_tripped, "and both fields survive the round trip byte for byte");

    // A REMEMBERED RUN IS CONFIRMABLE. A run is a window over a list and not a
    // node in it, so it never appears in the corpus alphabet -- and the first
    // build of phase 4 looked only there, which meant the winner of orbit.txt's
    // own example was recorded on every single run and re-found on none of
    // them. The count in the file reached 22 with `prior` still 0 in every
    // result. This is that bug, pinned.
    orbit_memory_forget();
    {
        ValuePtr corpus = list_of({yes(), yes(), no(), yes(), no(), yes(), yes()});
        OrbitQuery runs;
        runs.root = corpus;
        runs.pattern = list_of({yes(), yes()});

        Resolution first;
        orbit_resolve(runs, first);   // records the winner
        check(!first.findings.empty() && first.findings.front().prior == 0,
              "the first resolve of a run has no prior to draw on");

        Resolution second;
        orbit_resolve(runs, second);  // and now memory has it
        check(!second.findings.empty() && second.findings.front().prior > 0,
              "the second resolve finds the run in memory");
        check(!second.findings.empty() &&
                  second.findings.front().weight >
                      first.findings.front().weight,
              "and being remembered raises weight");

        // AND IT DOES NOT MOVE ATTENTION. Memory is an opinion about an answer
        // and not a sighting of one, so DECISION 2 -- attention may never read
        // a phase's opinion -- is pinned here as behaviour and not as a rule
        // in a comment.
        check(!first.findings.empty() && !second.findings.empty() &&
                  first.findings.front().attention ==
                      second.findings.front().attention,
              "and being remembered does not move attention");
    }

    // MEMORY NEVER INVENTS. This record's value is not in the corpus, so it
    // must contribute nothing at all.
    orbit_memory_forget();
    orbit_memory_record("\"ghost\"", "not in this corpus");

    OrbitQuery query;
    query.root = list_of({str("bolt"), str("nut")});
    query.pattern = str("ghost");
    query.threshold = SEARCH_EXACT;

    Resolution resolution;
    orbit_resolve(query, resolution);
    check(find_any(resolution.findings, "not in this corpus") == nullptr,
          "memory proposes only what is in the corpus in front of it");

    orbit_memory_forget();
}

