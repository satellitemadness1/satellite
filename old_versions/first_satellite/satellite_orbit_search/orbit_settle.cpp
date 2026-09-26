// Layer five — the answer becomes the question.
//
// orbit_resolve settles a query into a state. This settles the STATE: it takes
// what the resolution concluded, asks that instead, and keeps going until the
// question stops changing.
//
// WHY IT IS WORTH A LAYER. Everything below this improves an answer by looking
// harder at the corpus. This improves it by asking a BETTER QUESTION, and those
// are different things. A one-typo hit is a weak answer -- weight 15, the ladder
// saying "this is two rungs from anything" -- but the value it found is a real
// string that is really in there, and asking for THAT is an exact question with
// an exact answer. Nothing here learned anything and no rule was added; the
// second question was simply better than the first, and the trajectory shows
// the weight rise from 15 to 50 as it happens.
//
// It is also how orbit reaches past its own one-hop bound. Phase 3's link rule
// stops at a single hop because an unbounded chase inside the walker is a hang
// on a structure the language permits (search_power.txt, DECISION 4). A hop per
// ROUND is that same chase made safe: bounded by a count, stopped by a repeated
// question, and with every step standing in the answer as a result of its own.
//
// WHAT IT IS NOT. It does not learn. Between two settles over the same corpus
// nothing in this file changes, and the only thing in the whole engine that
// carries anything from one run to the next is phase 4's file. That is worth
// saying plainly here, at the top of the piece most likely to be mistaken for
// it: this is a fixpoint iteration, not a training loop.

#include "satellite_orbit_search/orbit.hpp"
#include "evaluator/eval_internal.hpp"

#include <set>

namespace satellite {

bool orbit_settle(const OrbitQuery &query, int rounds,
                  std::vector<OrbitRound> &out)
{
    if (!query.root || !query.pattern || rounds < 1)
        return true;

    // EVERY QUESTION ASKED SO FAR, and testing against all of them rather than
    // against the last one is what makes a CYCLE stop as fast as a fixpoint.
    // a pointing at b and b pointing back at a is an ordinary shape in a map of
    // cross-references; against the previous question alone it would run to the
    // round cap and report a chain, which is a false picture of a structure
    // that answered in two.
    std::set<std::string> asked;

    OrbitQuery round = query;

    // NO ROUND RECORDS. See OrbitQuery::remember: rounds 2 and up ask questions
    // the engine invented, and a memory fed its own guesses would return them as
    // evidence. The one record this makes is at the bottom of this function.
    round.remember = false;

    for (int i = 0; i < rounds; i++) {
        asked.insert(to_string(*round.pattern));

        OrbitRound step;
        step.pattern = round.pattern;
        if (!orbit_resolve(round, step.resolution))
            return false;

        // A ROUND THAT FOUND NOTHING ENDS IT, and is not recorded. There is no
        // next question to ask -- the answer that would have become one does not
        // exist -- and a trajectory whose last element concluded nothing would
        // read as though the run had settled on nothing, which is a different
        // claim from "it stopped here".
        if (step.resolution.findings.empty())
            break;

        out.push_back(std::move(step));

        const Finding &winner = out.back().resolution.findings.front();
        if (!winner.value)
            break;

        // A REPEATED QUESTION IS THE END, whichever kind it is. Equal to the
        // question just asked, it is a fixpoint: this corpus thinks that is what
        // you meant. Equal to an earlier one, the chain has closed a loop. Both
        // are answers, and both are read off the trajectory rather than
        // announced -- the last result's `pattern` and `value` say which.
        if (asked.count(to_string(*winner.value)))
            break;

        round.pattern = winner.value;
    }

    // AND THEN IT REMEMBERS -- once, and this is the useful thing to remember.
    //
    // The pairing is the question the USER asked with what the whole run settled
    // on, so what phase 4 learns from a settle is the CORRECTION: ask for
    // "boltzman" again tomorrow and memory says this has meant "boltzmann"
    // before. Recording each round instead would teach it seven things nobody
    // asked, and recording nothing would waste the one honest lesson here.
    //
    // Constructed answers are not recorded, on orbit_resolve's own rule: an
    // engine that remembered its own proposals would compound weight on nothing.
    if (query.remember && !out.empty()) {
        const Finding &settled = out.back().resolution.findings.front();
        if (!settled.constructed && settled.value)
            orbit_memory_record(to_string(*query.pattern),
                                to_string(*settled.value));
    }
    return true;
}

ValuePtr orbit_settle_collect(const ValuePtr &target, const ValuePtr &pattern,
                              int rounds, int max_depth, std::string &error)
{
    if (rounds < 1 || rounds > ORBIT_SETTLE_MAX) {
        error = "a settle runs between 1 and " +
                std::to_string(ORBIT_SETTLE_MAX) + " rounds, not " +
                std::to_string(rounds);
        return nullptr;
    }

    OrbitQuery query;
    query.root = target;
    query.pattern = pattern;
    query.threshold = search_threshold();
    query.max_depth = max_depth;

    std::vector<OrbitRound> rounds_out;
    if (!orbit_settle(query, rounds, rounds_out)) {
        error = "this structure nests deeper than satellite.library.system."
                "max_depth (" + std::to_string(max_depth) +
                "), so a search cannot finish walking it";
        return nullptr;
    }

    // ONE RESULT PER ROUND: that round's winner, built by the same builder
    // `.orbit()` uses and with nothing added to it. The round's other candidates
    // are not thrown away -- they are the winner's own .alternatives(), which is
    // what makes taking element zero lossless rather than a summary.
    List out;
    out.reserve(rounds_out.size());
    for (const OrbitRound &step : rounds_out) {
        OrbitQuery asked = query;
        asked.pattern = step.pattern;

        ValuePtr results = orbit_results_of(step.resolution, asked);
        const List *list = results ? as_list(*results) : nullptr;
        if (list && !list->empty())
            out.push_back(list->front());
    }
    return make_value(std::move(out));
}

} // namespace satellite
