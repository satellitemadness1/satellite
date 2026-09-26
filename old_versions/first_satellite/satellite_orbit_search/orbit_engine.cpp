// The pipeline, and the machinery more than one phase needs.
//
// orbit_resolve is the whole engine: five phases, in order, on one accumulating
// vector. It is deliberately this short. Every decision about WHAT a phase
// concludes lives in that phase's own file, so this file can be read to find
// out what orbit does without learning how any of it works.

#include "satellite_orbit_search/orbit.hpp"
#include "evaluator/eval_internal.hpp"

namespace satellite {

const char *orbit_phase_name(OrbitPhase phase)
{
    switch (phase) {
    case ORBIT_DIRECT:    return "direct";
    case ORBIT_GUESS:     return "guess";
    case ORBIT_PREDICT:   return "predict";
    case ORBIT_INDEX:     return "index";
    case ORBIT_FREQUENCY: return "frequency";
    }
    return "unknown";
}

const char *orbit_level_name(int score)
{
    switch (score) {
    case SEARCH_EXACT:       return "exact";
    case SEARCH_CASE:        return "case only";
    case SEARCH_CROSS_TYPE:  return "same text, different type";
    case SEARCH_TRIMMED:     return "whitespace only";
    case SEARCH_PREFIX:      return "a prefix";
    case SEARCH_SUBSTRING:   return "a substring";
    case SEARCH_DECODED:     return "the same once decoded";
    case SEARCH_ONE_TYPO:    return "one typo away";
    case SEARCH_TWO_TYPOS:   return "two typos away";
    case SEARCH_SUBSEQUENCE: return "the same letters in order";
    }
    return "no match";
}

// WHERE it was and WHAT it is, and both halves are load-bearing. Two phases
// reaching the same value at the same path are two votes for one answer, which
// is the number agreement counts; the same value at two different paths is two
// answers that happen to look alike, which it is not.
//
// A constructed finding has no path, so its identity is its value alone — which
// is right: force proposes a VALUE, and there is nowhere it came from.
std::string finding_key(const Finding &finding)
{
    std::string key = finding.constructed ? "~" : "@";
    if (!finding.constructed)
        for (const ValuePtr &step : finding.path) {
            key += step ? to_string(*step) : "?";
            key += '\x1f';
        }
    key += '\x1e';
    key += finding.value ? to_string(*finding.value) : "nil";
    return key;
}

// A run is only as tight as its weakest element. score_both is the two-value
// form of exactly that rule -- 0 if either failed, the looser of the two
// otherwise -- so folding it across k elements is the same rule at k, and not a
// second definition of what a combined score means.
void orbit_scan_runs(const List &hay, const List &needle, int threshold,
                     std::vector<OrbitRun> &out)
{
    const size_t width = needle.size();
    if (width == 0 || width > hay.size())
        return;

    for (size_t at = 0; at + width <= hay.size(); at++) {
        int score = SEARCH_TIGHTEST;
        for (size_t i = 0; i < width; i++) {
            const ValuePtr &want = needle[i];
            const ValuePtr &got  = hay[at + i];
            if (!want || !got) {
                score = SEARCH_NO_MATCH;
                break;
            }
            score = score_both(score, search_score(*want, *got));
            if (score == SEARCH_NO_MATCH)
                break;
        }
        if (score != SEARCH_NO_MATCH && score <= threshold)
            out.push_back(OrbitRun{at, score});
    }
}

namespace {

// Every value inside `root`, counted by its rendering.
//
// A SPACESUIT IS A LEAF here for the same reason it is one in search_walk, and
// the reason is not style: lists and maps are built then frozen and cannot form
// a cycle, but value.hpp says plainly that an object can, so a walk that
// followed an ObjectPtr would hang on a structure the language permits.
void gather(const ValuePtr &node, int depth, int max_depth,
            std::vector<OrbitTerm> &out, std::vector<std::string> &seen)
{
    if (!node || depth >= max_depth)
        return;

    // The node itself, before descending, so a nested container is counted as a
    // term as well as walked. A pattern that IS a container has to be findable.
    const std::string rendered = to_string(*node);
    bool found = false;
    for (size_t i = 0; i < seen.size(); i++)
        if (seen[i] == rendered) {
            out[i].count++;
            found = true;
            break;
        }
    if (!found) {
        seen.push_back(rendered);
        out.push_back(OrbitTerm{node, 1});
    }

    if (const List *list = as_list(*node)) {
        for (const ValuePtr &item : *list)
            gather(item, depth + 1, max_depth, out, seen);
        return;
    }
    if (const MapBody *map = as_map(*node)) {
        for (const MapEntry &entry : map->entries) {
            gather(entry.key, depth + 1, max_depth, out, seen);
            gather(entry.value, depth + 1, max_depth, out, seen);
        }
    }
}

// One value's literal sightings: itself as a node, plus every PROPER window of
// a longer list whose elements are exactly it. Attention, and nothing else feeds
// it.
void sight(const ValuePtr &node, const std::string &wanted, const List *needle,
           int depth, int max_depth, int &count)
{
    if (!node || depth >= max_depth)
        return;

    // The node itself, THE ROOT INCLUDED. orbit_alphabet drops the root from its
    // own terms because a corpus does not contain itself -- but that rule is
    // about what phase 3 may perturb toward, and this is a different question. A
    // pattern that IS the whole structure is in there once, and answering 0
    // would make the most present answer of all look like the least.
    if (to_string(*node) == wanted)
        count++;

    if (const List *list = as_list(*node)) {
        // A window spanning the WHOLE list is skipped: that list is the node
        // counted immediately above, and counting both would report one
        // sighting as two. It is the same rule phase 1 applies when it declines
        // to report a full-width run that search_walk already found.
        if (needle && !needle->empty() && needle->size() < list->size()) {
            std::vector<OrbitRun> runs;
            orbit_scan_runs(*list, *needle, SEARCH_EXACT, runs);
            count += static_cast<int>(runs.size());
        }
        for (const ValuePtr &item : *list)
            sight(item, wanted, needle, depth + 1, max_depth, count);
        return;
    }
    if (const MapBody *map = as_map(*node)) {
        for (const MapEntry &entry : map->entries) {
            sight(entry.key, wanted, needle, depth + 1, max_depth, count);
            sight(entry.value, wanted, needle, depth + 1, max_depth, count);
        }
    }

    // A spacesuit is a leaf here for the reason it is one everywhere else in
    // this power: an object can close a reference cycle and a walk that
    // followed one would hang.
}

} // namespace

int orbit_sightings(const ValuePtr &root, const ValuePtr &value, int max_depth)
{
    if (!root || !value)
        return 0;
    int count = 0;
    sight(root, to_string(*value), as_list(*value), 0, max_depth, count);
    return count;
}

void orbit_alphabet(const ValuePtr &root, int max_depth,
                    std::vector<OrbitTerm> &out)
{
    std::vector<std::string> seen;
    // The ROOT ITSELF is not a term of its own alphabet. It is counted by
    // gather so that a pattern describing the whole structure is findable, and
    // then dropped here: a corpus does not contain itself, and leaving it in
    // would make phase 3 propose the entire list as a neighbour of {T, T}.
    gather(root, 0, max_depth, out, seen);
    if (!out.empty())
        out.erase(out.begin());
}

bool orbit_resolve(const OrbitQuery &query, Resolution &out)
{
    if (!query.root || !query.pattern)
        return true;

    // FIVE PHASES, ONE VECTOR, AND NO EARLY EXIT. DECISION 2: orbit.txt says
    // guess builds "up on the direct search, adding to the end result", and the
    // deeper reason is that stopping early destroys phase 5 -- frequency
    // weighs candidates against each other, and one unopposed candidate can
    // only ever be rated 100%.
    //
    // Each phase reads what the ones before it appended. That shared vector IS
    // "building up on the direct search"; there is no other channel between
    // them, and none of them can see a phase that has not run yet.
    orbit_direct(query, out.findings);
    orbit_guess(query, out.findings);
    orbit_predict(query, out.findings);
    orbit_index(query, out.findings);

    // Produces nothing. Merges the duplicates the four above are expected to
    // produce -- that duplication is the agreement signal, not a defect -- and
    // ranks what is left.
    orbit_frequency(query, out.findings);

    // AND THEN IT REMEMBERS. Phase 4 can only ever answer if something wrote
    // the record, and this is the only place that knows both what was asked and
    // what it settled on.
    //
    // ONLY THE WINNER, and only when it was really in there. A constructed
    // answer is the engine's own proposal, and a memory that recorded its own
    // guesses would feed them back as evidence on the next run -- confidence
    // compounding on nothing, which is the failure mode this whole file is
    // arranged to avoid.
    //
    // The cost is one read and one rewrite of the memory file per resolution,
    // and it is stated here rather than discovered: `.orbit()` touches the disk
    // and `.search()` does not, which is part of why they are two spellings.
    // `remember` is false for every round of a settle, and the reason is the
    // paragraph above taken one step further: a settle's rounds 2 and up ask
    // questions the ENGINE invented, so recording them would be the engine
    // teaching itself its own guesses. A settle records once, at the end,
    // pairing what the USER asked with what the whole run settled on.
    if (query.remember && !out.findings.empty() &&
        !out.findings.front().constructed && out.findings.front().value)
        orbit_memory_record(to_string(*query.pattern),
                            to_string(*out.findings.front().value));

    return !out.too_deep;
}

} // namespace satellite
