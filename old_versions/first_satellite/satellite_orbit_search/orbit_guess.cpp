// Phase 2 — guess. The dial, run upward on the query's behalf.
//
// The ladder ALREADY CONTAINS everything "guess without brute force" can
// honestly mean. Levels 5 through 10 are prefix, substring, decoded, one typo,
// two typos and subsequence, and search.cpp implements all six. Nothing here
// invents a new way for two values to be alike, and a phase that did would put
// the dial's meaning in two places.
//
// What was missing is that the USER had to pick the level. A needle that would
// have matched at 8 returns nothing at 1 and says nothing about how close it
// came -- so the program is told "no" by a question it did not know it was
// asking. This phase asks the rest of the question.
//
// IT RUNS EVEN WHEN PHASE 1 FOUND SOMETHING, and orbit.txt is explicit that it
// should: guess builds "up on the direct search, ADDING TO the end result". A
// tight hit and a loose one are two candidates for phase 5 to weigh, and
// skipping the second because the first existed is the short-circuit DECISION 2
// refuses.

#include "satellite_orbit_search/orbit.hpp"
#include "evaluator/eval_internal.hpp"

#include <set>

namespace satellite {

void orbit_guess(const OrbitQuery &query, std::vector<Finding> &out)
{
    // Already at the loosest the ladder goes: there is no wider question to
    // ask, and re-walking would produce the findings phase 1 already has.
    if (query.threshold >= SEARCH_LOOSEST)
        return;

    // What phase 1 concluded, so this one adds rather than repeats. Built once
    // from what is already in `out` -- which is the only channel between the
    // phases, and is what "building up on the direct search" is in code.
    std::set<std::string> known;
    for (const Finding &finding : out)
        known.insert(finding_key(finding));

    OrbitQuery wide = query;
    wide.threshold = SEARCH_LOOSEST;

    std::vector<Finding> found;
    orbit_direct(wide, found);

    for (Finding &finding : found) {
        if (known.count(finding_key(finding)))
            continue;

        // THE LEVEL IT TOOK IS THE WHOLE ANSWER, not a detail of it. "Nothing
        // at 1, but this matched at 8" tells the user their needle was two
        // typos from something real, which is a different fact from "no" and
        // is the fact they were actually after.
        finding.phase = ORBIT_GUESS;
        finding.why = "nothing at level " + std::to_string(query.threshold) +
                      "; this matched at level " + std::to_string(finding.score) +
                      " (" + orbit_level_name(finding.score) + ")";
        out.push_back(std::move(finding));
    }
}

} // namespace satellite
