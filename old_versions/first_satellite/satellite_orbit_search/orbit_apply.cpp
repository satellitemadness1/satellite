// The domain: what `.orbit()` hands back to a satellite program.
//
// The only file in this module that knows both that a resolution exists and
// that the language has a result type. The five phases know neither, which is
// what lets a phase change without the surface moving and the surface change
// without a phase moving -- the same seam search_apply.cpp draws one layer down,
// for the same reason.
//
// A RESULT IS satellite.container.result, AND THAT IS DECISION 4 LANDED. It was
// a plain map until 2026-08-24, which cost exactly what the plan said it would:
// `r.why()` was `r["why"]`, and a result could not be BOUND TO A TYPED VARIABLE
// AT ALL, because the fields are heterogeneous and no `map<K, V>` is true about
// them. Now `satellite.container.list<satellite.container.result> found` is a
// declaration that type-checks, and .weight(), .attention(), .phases() and
// .alternatives() are methods a user's own map key cannot collide with.
//
// What did NOT change is that a result is a map underneath: it prints as one, it
// type-checks against a bare `satellite.container.map`, and the search power
// walks into it -- so the result of a search can be searched by the power that
// produced it, with no new code.
//
// The building of one is next door in orbit_result.cpp. This file knows a
// resolution exists; that one knows what a result is; and neither has to know
// both.

#include "satellite_orbit_search/orbit.hpp"
#include "evaluator/eval_internal.hpp"

namespace satellite {

ValuePtr orbit_collect(const ValuePtr &target, const ValuePtr &pattern,
                       int max_depth, std::string &error)
{
    OrbitQuery query;
    query.root = target;
    query.pattern = pattern;

    // THE SAME DIAL .search() READS. Orbit does not get a second threshold: a
    // user who set satellite.system.threshold(3) has said how loose a match may
    // be, and phase 1 has to mean by that what the subscript means by it. What
    // orbit adds is phase 2, which goes PAST the dial deliberately and says so
    // in every sentence it writes.
    query.threshold = search_threshold();
    query.max_depth = max_depth;

    Resolution resolution;
    if (!orbit_resolve(query, resolution)) {
        error = "this structure nests deeper than satellite.library.system."
                "max_depth (" + std::to_string(max_depth) +
                "), so a search cannot finish walking it";
        return nullptr;
    }

    // ALWAYS A LIST, EMPTY WHEN NOTHING RESOLVED, which is DECISION 7's rule one
    // layer up and is right for the same reason: asking whether a structure
    // contains something is the whole point, and a question that errors when the
    // answer is "no" cannot be asked.
    //
    // A list of RESULTS, so `found.length()` is how many came back and
    // `found[0].length()` is how many fields one of them has. The surprise is
    // real and is worth naming here where it is imposed: a result is a map, and
    // every map method on it counts fields. `found[0].alternatives().length()`
    // is how a program says the other number.
    return orbit_results_of(resolution, query);
}

} // namespace satellite
