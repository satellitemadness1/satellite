// Phase 1 — direct. What you think of when you think of searching.
//
// Two halves. The first is search_walk exactly as it already is, at the dial
// the user set, converted to findings. The second is the run scan, and it is
// the one piece of matching machinery orbit adds rather than reuses.
//
// WHY THE RUN SCAN HAD TO BE BUILT. orbit.txt's own example is a list
// {T,T,F,T,F,T,T} searched for {T,T}, and nothing in the language finds that.
// **Verified** in src/evaluator/search.cpp: score_containers compares a list
// needle to a list hay only when `nl->size() != hl->size()` is false -- equal
// lengths -- so a two-element pattern can match a two-element list and nothing
// else. A window of two inside a list of seven is not a case anything handles.
//
// It is in DIRECT and not in GUESS because a run that is literally sitting in
// the list is found, not guessed. Phase 2's job is the dial; this is not that.

#include "satellite_orbit_search/orbit.hpp"
#include "evaluator/eval_internal.hpp"

namespace satellite {
namespace {

std::string at_level(int score)
{
    return "at level " + std::to_string(score);
}

// The window as the value that was found: you asked for a run of k and a run of
// k is what is handed back, not the k elements loose. `at` goes into the path
// because a run's location IS an index into the list it was found in.
ValuePtr window_value(const List &hay, size_t at, size_t width)
{
    List window;
    window.reserve(width);
    for (size_t i = 0; i < width; i++)
        window.push_back(hay[at + i]);
    return make_value(std::move(window));
}

// Every list inside `node`, scanned for runs of the pattern. Recursive, because
// a list of lists is a structure the language permits and a run in the third
// one is as real as a run in the first.
void scan(const ValuePtr &node, const List &needle, const OrbitQuery &query,
          const List &path, int depth, std::vector<Finding> &out)
{
    if (!node || depth >= query.max_depth)
        return;

    if (const List *hay = as_list(*node)) {
        std::vector<OrbitRun> runs;
        orbit_scan_runs(*hay, needle, query.threshold, runs);
        for (const OrbitRun &run : runs) {
            // A WINDOW SPANNING THE WHOLE LIST IS NOT REPORTED HERE. That is a
            // list matching a list of the same length, which score_containers
            // already answers and search_walk already found -- so reporting it
            // again would be one finding counted as two, and phase 5 counts
            // agreement across phases. A duplicate inside one phase is not
            // agreement, it is double-entry.
            if (needle.size() == hay->size())
                continue;

            List here = path;
            here.push_back(make_value(Number(static_cast<long long>(run.at))));
            out.push_back(Finding(
                window_value(*hay, run.at, needle.size()), nullptr, here,
                run.score, ORBIT_DIRECT,
                "a run of " + std::to_string(needle.size()) + " starting at " +
                    std::to_string(run.at) + ", " + at_level(run.score)));
        }

        for (size_t i = 0; i < hay->size(); i++) {
            List here = path;
            here.push_back(make_value(Number(static_cast<long long>(i))));
            scan((*hay)[i], needle, query, here, depth + 1, out);
        }
        return;
    }

    if (const MapBody *map = as_map(*node)) {
        for (const MapEntry &entry : map->entries) {
            if (!entry.key || !entry.value)
                continue;
            List here = path;
            here.push_back(entry.key);
            scan(entry.value, needle, query, here, depth + 1, out);
        }
    }

    // A spacesuit is a leaf, on search_walk's argument: an object can close a
    // reference cycle and a walker that followed one would hang.
}

} // namespace

void orbit_direct(const OrbitQuery &query, std::vector<Finding> &out)
{
    // --- the walk, unchanged ------------------------------------------------
    //
    // Not reimplemented and not adjusted. Phase 1 IS the existing search power,
    // which is what makes `.orbit()` a superset of `.search()` by construction
    // rather than by two implementations agreeing.
    std::vector<SearchHit> hits;
    search_walk(query.root, query.pattern, query.threshold, query.max_depth,
                hits);

    for (const SearchHit &hit : hits) {
        std::string why = hit.path.empty()
                              ? "the whole structure matched, " + at_level(hit.score)
                              : "found, " + at_level(hit.score);
        out.push_back(Finding(hit.value, hit.key, hit.path, hit.score,
                              ORBIT_DIRECT, std::move(why)));
    }

    // --- the run scan -------------------------------------------------------
    //
    // Only a LIST pattern describes a run. A scalar needle has no width, and a
    // map needle is a subset question that score_containers already answers
    // over a whole map rather than over a window of one.
    const List *needle = as_list(*query.pattern);
    if (needle && !needle->empty())
        scan(query.root, *needle, query, List{}, 0, out);
}

} // namespace satellite
