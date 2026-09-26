// Phase 3 — predict. "by logic and force, to attempt to render a correct
// result" (orbit.txt), which names two things because they are two things.
//
// LOGIC is the one-hop link. A value found at one path whose text is also a KEY
// at another path resolves to whatever that key points at. That is transitivity,
// and it is the one inference a flat search structurally cannot make: the
// walker scores the needle against everything, but nothing in it ever asks what
// a HIT points at. It is bounded to a single hop for the reason DECISION 4 of
// the search plan bounds the walker at all -- an unbounded chase is a hang on a
// structure the language permits.
//
// FORCE is neighbour construction, and it is the half that makes orbit.txt's
// own answer expressible. Searching {T,T} and being told the answer may be
// {T,F} is a CONSTRUCTED candidate: no amount of searching produces it, because
// it is not what was asked for. So the query is perturbed by one element and
// the perturbed query is asked instead.
//
// ONE ELEMENT, and that bound is the whole reason this is not the brute force
// the request specifically excluded. k elements changed over an alphabet of n
// is n^k; one element changed is n*k, so the phase stays linear in the thing it
// perturbs. A second element would be the combinatorial search this design says
// it is not doing.

#include "satellite_orbit_search/orbit.hpp"
#include "evaluator/eval_internal.hpp"

#include <algorithm>
#include <set>

namespace satellite {
namespace {

// The alphabet cap, and it is stated rather than silent. A corpus of ten
// thousand distinct strings would otherwise mean ten thousand perturbations per
// position, which is the brute force this phase exists to avoid being. The most
// frequent terms are kept, because a neighbour built from a term that occurs
// once is a neighbour that occurs at most once.
constexpr size_t ALPHABET_CAP = 32;

// Every map key in the structure, and what it points at. The index the link
// rule reads: "is this text a key somewhere else, and if so, what is under it".
struct KeyIndex {
    std::vector<std::string> keys;
    std::vector<ValuePtr> values;
    std::vector<List> paths;
};

void index_keys(const ValuePtr &node, const List &path, int depth,
                int max_depth, KeyIndex &out)
{
    if (!node || depth >= max_depth)
        return;

    if (const List *list = as_list(*node)) {
        for (size_t i = 0; i < list->size(); i++) {
            List here = path;
            here.push_back(make_value(Number(static_cast<long long>(i))));
            index_keys((*list)[i], here, depth + 1, max_depth, out);
        }
        return;
    }
    if (const MapBody *map = as_map(*node)) {
        for (const MapEntry &entry : map->entries) {
            if (!entry.key || !entry.value)
                continue;
            List here = path;
            here.push_back(entry.key);
            out.keys.push_back(to_string(*entry.key));
            out.values.push_back(entry.value);
            out.paths.push_back(here);
            index_keys(entry.value, here, depth + 1, max_depth, out);
        }
    }
}

// The scalars of the corpus, most frequent first, capped. Containers are
// dropped: a neighbour of {T, T} built by substituting a whole sublist for one
// element is not a neighbour, it is a different shape of question.
void perturbation_terms(const OrbitQuery &query, std::vector<OrbitTerm> &out)
{
    std::vector<OrbitTerm> all;
    orbit_alphabet(query.root, query.max_depth, all);
    for (const OrbitTerm &term : all)
        if (term.value && !as_list(*term.value) && !as_map(*term.value))
            out.push_back(term);

    std::stable_sort(out.begin(), out.end(),
                     [](const OrbitTerm &a, const OrbitTerm &b) {
                         return a.count > b.count;
                     });
    if (out.size() > ALPHABET_CAP)
        out.resize(ALPHABET_CAP);
}

} // namespace

void orbit_predict(const OrbitQuery &query, std::vector<Finding> &out)
{
    std::set<std::string> known;
    for (const Finding &finding : out)
        known.insert(finding_key(finding));

    // --- logic: the one-hop link -------------------------------------------
    KeyIndex index;
    index_keys(query.root, List{}, 0, query.max_depth, index);

    // Snapshotted before appending, because this phase adds to the vector it is
    // reading. Without the snapshot a linked finding could be linked from
    // again, one hop at a time, which is the unbounded chase the bound exists
    // to prevent -- and it would be unbounded by accident rather than by
    // decision, which is worse.
    const size_t before = out.size();
    for (size_t i = 0; i < before; i++) {
        const Finding &from = out[i];
        if (!from.value)
            continue;
        const std::string text = to_string(*from.value);

        for (size_t k = 0; k < index.keys.size(); k++) {
            if (index.keys[k] != text)
                continue;

            Finding linked(index.values[k], make_value(encode_raw(text)),
                           index.paths[k], from.score, ORBIT_PREDICT,
                           "\"" + text + "\" was found as a value, and is also "
                           "a key elsewhere, which points here");
            if (known.count(finding_key(linked)))
                continue;
            known.insert(finding_key(linked));
            out.push_back(std::move(linked));
        }
    }

    // --- force: one element changed ----------------------------------------
    //
    // Only a LIST pattern has elements to perturb. A scalar needle perturbed
    // toward the corpus is just the ladder run loose, which is phase 2's job
    // and is already done by the time this runs.
    const List *needle = as_list(*query.pattern);
    if (!needle || needle->empty())
        return;

    std::vector<OrbitTerm> terms;
    perturbation_terms(query, terms);
    const std::string asked = to_string(*query.pattern);

    for (size_t at = 0; at < needle->size(); at++) {
        for (const OrbitTerm &term : terms) {
            List candidate = *needle;
            candidate[at] = term.value;

            ValuePtr proposal = make_value(candidate);
            const std::string text = to_string(*proposal);
            if (text == asked)
                continue;  // not a neighbour of itself

            // A NEIGHBOUR IS ONLY PROPOSED IF IT OCCURS. Force perturbs the
            // QUESTION, never the answer: the engine may ask something the user
            // did not ask, but what it hands back is still a run that is really
            // in the corpus. A proposal that occurs nowhere is not a prediction,
            // it is noise with a sentence attached.
            std::vector<OrbitRun> runs;
            int occurrences = 0;
            int best = SEARCH_NO_MATCH;
            if (const List *hay = as_list(*query.root)) {
                orbit_scan_runs(*hay, candidate, query.threshold, runs);
                occurrences = static_cast<int>(runs.size());
                for (const OrbitRun &run : runs)
                    best = score_either(best, run.score);
            }
            if (occurrences == 0)
                continue;

            Finding forced(
                proposal, nullptr, List{}, best, ORBIT_PREDICT,
                "you asked for " + asked + "; changing element " +
                    std::to_string(at) + " to " + to_string(*term.value) +
                    " gives " + text + ", which occurs " +
                    std::to_string(occurrences) +
                    (occurrences == 1 ? " time" : " times"));
            forced.constructed = true;

            // The count goes into the SENTENCE and nowhere else. Phase 5 counts
            // attention itself, from the corpus, for every finding alike -- and
            // a phase that wrote its own count into the number would be one
            // phase's opinion arriving dressed as a fact. It was built the other
            // way first, and the asymmetry that produced is recorded over
            // orbit_sightings: a run this phase INVENTED scored 2 while an
            // identical run phase 1 FOUND scored 0.

            const std::string id = finding_key(forced);
            if (known.count(id))
                continue;
            known.insert(id);
            out.push_back(std::move(forced));
        }
    }
}

} // namespace satellite
