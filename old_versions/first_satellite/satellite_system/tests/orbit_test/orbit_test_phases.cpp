// The six sections. What each one pins is written over it; the four rules the
// whole file exists to hold are written over main(), in orbit_test.cpp.

#include "orbit_test.hpp"

#include <string>
#include <vector>

using namespace satellite;

// --- the run scan ------------------------------------------------------------
//
// The one piece of matching machinery orbit adds. Nothing in the language finds
// a two-element run inside a seven-element list today: score_containers
// compares a list needle to a list hay only at equal lengths.

void orbit_test_runs()
{
    // orbit.txt's list, exactly as written.
    ValuePtr hay = list_of({yes(), yes(), no(), yes(), no(), yes(), yes()});
    const List *hay_list = as_list(*hay);

    List needle;
    needle.push_back(yes());
    needle.push_back(yes());

    std::vector<OrbitRun> runs;
    orbit_scan_runs(*hay_list, needle, SEARCH_EXACT, runs);

    check(runs.size() == 2, "{T,T} occurs twice in {T,T,F,T,F,T,T}");
    if (runs.size() == 2) {
        check(runs[0].at == 0, "the first {T,T} starts at 0");
        check(runs[1].at == 5, "the second {T,T} starts at 5");
        check(runs[0].score == SEARCH_EXACT, "an exact run scores exact");
    }

    // {T,F} — the answer orbit.txt says the engine might come back with.
    List other;
    other.push_back(yes());
    other.push_back(no());
    std::vector<OrbitRun> others;
    orbit_scan_runs(*hay_list, other, SEARCH_EXACT, others);
    check(others.size() == 2, "{T,F} also occurs twice");

    // A run is only as tight as its weakest element: score_both across k.
    List wide;
    wide.push_back(str("bolt"));
    wide.push_back(str("nut"));
    ValuePtr words = list_of({str("BOLT"), str("nut"), str("washer")});
    std::vector<OrbitRun> loose;
    orbit_scan_runs(*as_list(*words), wide, SEARCH_CASE, loose);
    check(loose.size() == 1 && loose[0].score == SEARCH_CASE,
          "a run scores at its loosest element, not its tightest");

    // Nothing at a dial that excludes it. The threshold is honoured inside the
    // scan rather than filtered after, which is what keeps a wide corpus cheap.
    std::vector<OrbitRun> none;
    orbit_scan_runs(*as_list(*words), wide, SEARCH_EXACT, none);
    check(none.empty(), "the dial is honoured by the run scan");
}

// --- accumulation ------------------------------------------------------------

void orbit_test_accumulates()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    // "boltzmann" is in the corpus; "boltzmenn" is not. At the exact dial phase 1
    // finds nothing, and phase 2 has to be what finds it -- one typo away.
    //
    // NOT "boltzman", which was the first needle written here and which the
    // ladder rightly scored at 5: it is a PREFIX of "boltzmann", and prefix is
    // a tighter rung than typo. The test was wrong and search_score was right.
    // One substitution in the middle is a typo and nothing cheaper.
    ValuePtr corpus = list_of({str("boltzmann"), str("washer"), str("nut")});

    OrbitQuery query;
    query.root = corpus;
    query.pattern = str("boltzmenn");
    query.threshold = SEARCH_EXACT;

    Resolution resolution;
    orbit_resolve(query, resolution);

    check(!resolution.findings.empty(),
          "a one-typo needle resolves to something at the exact dial");

    const Finding *hit = find_any(resolution.findings, "boltzmann");
    check(hit != nullptr, "guess reaches what direct could not");
    if (hit) {
        check(hit->phase == ORBIT_GUESS, "and it is attributed to guess");
        check(hit->score == SEARCH_ONE_TYPO, "at the one-typo rung");
        check(hit->why.find("one typo") != std::string::npos,
              "and the sentence says so in words, not only in a number");
    }

    orbit_memory_forget();
}

// --- provenance --------------------------------------------------------------
//
// DECISION 6. Not a debugging aid: a finding that cannot say which phase made
// it is an engine acting on the user without saying so.

void orbit_test_provenance()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    ValuePtr corpus = list_of({yes(), yes(), no(), yes(), no(), yes(), yes()});

    OrbitQuery query;
    query.root = corpus;
    query.pattern = list_of({yes(), yes()});

    Resolution resolution;
    orbit_resolve(query, resolution);

    check(!resolution.findings.empty(), "orbit.txt's own example resolves");

    for (const Finding &finding : resolution.findings) {
        check(!finding.why.empty(), "every finding carries a reason");
        check(finding.phase >= ORBIT_DIRECT && finding.phase <= ORBIT_FREQUENCY,
              "every finding carries a phase in range");
        check(finding.weight >= 0 && finding.weight <= 100,
              "weight is a percentage");

        // A COUNT AND NOT A PERCENTAGE, so there is no ceiling to check; that
        // it agrees with the corpus is checked directly below.
        check(finding.attention >= 0, "attention is a count, never negative");

        // said[] is what makes a result the state of the search rather than its
        // conclusion, so a phase that concluded something and left no sentence
        // would be a silent one in a result that claims to carry all five.
        check(!finding.said[finding.phase].empty(),
              "a finding records its own phase's sentence");
        check(!finding.said[ORBIT_FREQUENCY].empty(),
              "and phase 5's, which is where it placed");

        // A constructed finding has no path, and that empty has to be
        // distinguishable from the root's empty by the flag rather than by
        // guessing from the emptiness.
        if (finding.constructed)
            check(finding.path.empty(), "a constructed finding has no path");
    }

    orbit_memory_forget();
}

// --- orbit.txt's example, end to end -----------------------------------------

void orbit_test_the_example()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    ValuePtr corpus = list_of({yes(), yes(), no(), yes(), no(), yes(), yes()});

    OrbitQuery query;
    query.root = corpus;
    query.pattern = list_of({yes(), yes()});

    Resolution resolution;
    orbit_resolve(query, resolution);

    // What was asked for is found, twice, and it is a DIRECT finding -- a run
    // that is really in the list is found, not guessed.
    const Finding *asked = find_from(resolution.findings, "[true, true]",
                                     ORBIT_DIRECT);
    check(asked != nullptr, "{T,T} is found directly in {T,T,F,T,F,T,T}");
    if (asked)
        check(!asked->constructed, "and it is not a construction");

    // And {T,F} is reachable as a constructed alternative, which is the answer
    // orbit.txt says the engine might come back with. It is PROPOSED, not
    // found: nothing in the corpus said "the user meant {T,F}".
    const Finding *other = find_from(resolution.findings, "[true, false]",
                                     ORBIT_PREDICT);
    check(other != nullptr, "{T,F} is proposed by predict as a neighbour");
    if (other) {
        check(other->constructed, "and it is marked as constructed");
        // ATTENTION COUNTS THE WINDOWS, and it used to be wrong in the
        // interesting direction: phase 3 wrote its own count, which made a run
        // the engine INVENTED look more present than an identical run phase 1
        // had FOUND. Phase 5 counts both from the corpus now.
        check(other->attention == 2,
              "a constructed neighbour's attention is its real window count");
        check(other->why.find("changing element") != std::string::npos,
              "and it says which element it changed");

        // THE HALVING. A constructed answer to a question the user did not ask
        // cannot outrank the same evidence for the question they did.
        check(asked == nullptr || other->weight < asked->weight,
              "a construction ranks below what was actually asked for");

        // AND THE TWO NUMBERS DISAGREE, which is why there are two of them: the
        // proposal is in the corpus exactly as often as what was asked for, and
        // orbit thinks far less of it. One blended `confidence` could not say
        // both of those at once, and this is the case it could not say.
        check(asked == nullptr || other->attention == asked->attention,
              "with the same attention as the answer that outranks it");
    }

    orbit_memory_forget();
}

// --- predict's logic half: the one-hop link ----------------------------------

void orbit_test_link()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    // "bolt" is a VALUE under `part`, and also a KEY of the second map. Nothing
    // in a flat search ever asks what a hit points at; the link rule does.
    ValuePtr corpus = list_of({
        map_of({{str("part"), str("bolt")}}),
        map_of({{str("bolt"), str("M8 x 40")}}),
    });

    // THE NEEDLE IS "part", NOT "bolt", and that is the whole point of the
    // test. Searching for "bolt" proves nothing: search_walk answers a key
    // match with the entry's VALUE, so direct would hand back "M8 x 40" on its
    // own and the link rule would never be exercised. Written that way first,
    // and it passed for the wrong reason.
    //
    // Searching for "part" reaches "bolt" directly, and reaching "M8 x 40" from
    // there is a second hop that only phase 3 can make.
    OrbitQuery query;
    query.root = corpus;
    query.pattern = str("part");
    query.threshold = SEARCH_EXACT;

    Resolution resolution;
    orbit_resolve(query, resolution);

    check(find_from(resolution.findings, "bolt", ORBIT_DIRECT) != nullptr,
          "direct reaches the value under the key that matched");

    const Finding *linked = find_from(resolution.findings, "M8 x 40",
                                      ORBIT_PREDICT);
    check(linked != nullptr, "the one-hop link resolves a value that is a key");
    if (linked)
        check(linked->why.find("key elsewhere") != std::string::npos,
              "and the sentence explains the hop");

    orbit_memory_forget();
}
