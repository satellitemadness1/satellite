// Layer five — the answer becoming the question, and where that stops.
//
// Five things this file pins, each of which would otherwise be true only by
// luck:
//
//   1. ROUND N+1 ASKS WHAT ROUND N CONCLUDED. Not "something related to" it --
//      literally that value, which the result's own `pattern` field records, so
//      a trajectory reads as the conversation the engine had with itself.
//   2. A REPEATED QUESTION ENDS IT, whether it is the one just asked (a
//      fixpoint) or an earlier one (a cycle). Two values pointing at each other
//      must close in two rounds and not run to the cap.
//   3. THE WEIGHT RISES when the second question is better than the first. That
//      is the whole reason this layer exists, and it is not a slogan: a loose
//      hit's value is a real string, and asking for it is an exact question.
//   4. A SETTLE RECORDS EXACTLY ONE THING, pairing what the USER asked with
//      what the run settled on. Recording each round would be the engine
//      teaching itself its own guesses.
//   5. THE ROUND COUNT IS REFUSED BY NAME when it is out of range, never
//      clamped -- the dial's DECISION 5b, one layer up.

#include "orbit_test.hpp"

#include <string>

using namespace satellite;

namespace {

// bolt -> fastener -> hardware, and a two-cycle beside it.
ValuePtr glossary()
{
    return map_of({{str("bolt"), str("fastener")},
                   {str("fastener"), str("hardware")},
                   {str("ping"), str("pong")},
                   {str("pong"), str("ping")}});
}

std::vector<OrbitRound> settle_over(const ValuePtr &root, const char *pattern,
                                    int rounds, bool remember = false)
{
    OrbitQuery query;
    query.root = root;
    query.pattern = str(pattern);
    query.remember = remember;

    std::vector<OrbitRound> out;
    orbit_settle(query, rounds, out);
    return out;
}

std::string answer_of(const OrbitRound &round)
{
    if (round.resolution.findings.empty() ||
        !round.resolution.findings.front().value)
        return "<nothing>";
    return to_string(*round.resolution.findings.front().value);
}

} // namespace

void orbit_test_settle_chases()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    // --- the answer becomes the question ------------------------------------
    std::vector<OrbitRound> chain = settle_over(glossary(), "bolt",
                                                ORBIT_SETTLE_ROUNDS);
    check(chain.size() >= 2, "asking for bolt takes more than one round");
    if (chain.size() >= 2) {
        check(answer_of(chain[0]) == "fastener",
              "round 1 concludes what bolt points at");

        // THE POINT OF THE LAYER, checked literally: round 2's question IS
        // round 1's answer. Nothing derived from it, nothing near it.
        check(chain[1].pattern && to_string(*chain[1].pattern) == "fastener",
              "and round 2 asks for that, not for something like it");
    }

    // --- a repeated question ends it ----------------------------------------
    //
    // A fixpoint: this corpus thinks fastener is what bolt meant, so asking for
    // fastener answers fastener and there is nothing further to ask.
    check(chain.size() < ORBIT_SETTLE_ROUNDS,
          "a chain that settles stops well short of the cap");

    // A CYCLE CLOSES AS FAST AS A FIXPOINT, which is what testing against every
    // question asked buys over testing against the last one. ping and pong
    // point at each other, and against the previous question alone this would
    // run to the cap and report a chain that is really a loop.
    std::vector<OrbitRound> loop = settle_over(glossary(), "ping",
                                               ORBIT_SETTLE_ROUNDS);
    check(!loop.empty() && loop.size() <= 3,
          "a two-cycle closes in a couple of rounds, not at the cap");

    orbit_memory_forget();
}

void orbit_test_settle_improves()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    // A question asked BADLY, with the dial open far enough to see past it.
    // "boltzman" is a prefix of "boltzmann" -- rung 5, a weak answer -- but the
    // value it found is a real string sitting in the list.
    const int was = search_threshold();
    set_search_threshold(SEARCH_ONE_TYPO);

    ValuePtr words = list_of({str("boltzmann"), str("planck"), str("maxwell")});
    std::vector<OrbitRound> trip = settle_over(words, "boltzman",
                                               ORBIT_SETTLE_ROUNDS);

    check(trip.size() >= 2, "a loose hit is worth asking about again");
    if (trip.size() >= 2) {
        check(answer_of(trip[0]) == "boltzmann" &&
                  answer_of(trip[1]) == "boltzmann",
              "both rounds reach the same value");

        const Finding &loose = trip[0].resolution.findings.front();
        const Finding &exact = trip[1].resolution.findings.front();

        // THE RISE. Round 1 matched on a rung; round 2 matched on the nose, so
        // the ladder pays it more. Nothing learned anything between them -- the
        // second question was simply better, which is the whole claim this
        // layer makes for itself.
        check(exact.score < loose.score,
              "the second question matches at a tighter rung");
        check(exact.weight > loose.weight,
              "so orbit thinks more of its own second answer");

        // AND ATTENTION DID NOT MOVE. The corpus never changed, so the count of
        // what is really in it cannot have -- which is DECISION 2 holding
        // across a whole trajectory and not only inside one resolution.
        check(exact.attention == loose.attention,
              "while attention, a fact about the corpus, stays put");
    }

    set_search_threshold(was);
    orbit_memory_forget();
}

void orbit_test_settle_remembers_once()
{
    set_orbit_memory_path(TEST_MEMORY);
    orbit_memory_forget();

    // The surface, because `remember` is the surface's business: the engine is
    // handed remember=false by every round and the ONE record is made by the
    // settle itself, at the end, for the question the user actually asked.
    std::string error;
    ValuePtr trip = orbit_settle_collect(glossary(), str("bolt"),
                                         ORBIT_SETTLE_ROUNDS, 64, error);
    check(trip != nullptr, "a settle over a map answers a trajectory");

    std::vector<OrbitRecord> records;
    orbit_memory_read(records);

    // EXACTLY ONE. A settle runs several resolutions and every one of them
    // would have recorded on its own; rounds 2 and up ask questions the ENGINE
    // invented, and a memory fed those would return the engine's own guesses as
    // evidence next time -- weight compounding on nothing.
    check(records.size() == 1, "a whole settle teaches memory exactly one thing");
    if (records.size() == 1) {
        check(records[0].pattern == "bolt",
              "and it is the question the USER asked");
        check(records[0].resolved == "fastener",
              "paired with what the run settled on -- the correction");
    }

    // --- the round count is refused by name ---------------------------------
    error.clear();
    check(orbit_settle_collect(glossary(), str("bolt"), 0, 64, error) == nullptr,
          "zero rounds is refused");
    check(!error.empty(), "and says so by name rather than clamping");

    error.clear();
    check(orbit_settle_collect(glossary(), str("bolt"), ORBIT_SETTLE_MAX + 1, 64,
                               error) == nullptr,
          "and so is more than the language will run");

    orbit_memory_forget();
}
