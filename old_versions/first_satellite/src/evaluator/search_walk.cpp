// The walker: everything inside a value, and where it was.
//
// Layer two of the search power. It knows LISTS and MAPS and nothing about how
// two values are compared -- every comparison here is one call to
// search_score. plans/search_power.txt, DECISION 1.

#include "evaluator/search.hpp"
#include "evaluator/eval_internal.hpp"

#include <algorithm>

namespace satellite {
namespace {

// A two-element list is an ENTRY pattern -- key then value -- and this is the
// only place the shape is recognised. Two is the shape a map entry has, and it
// is the shape that was asked for (DECISION 6).
//
// It does not STOP the pattern being an ordinary value: the walk tries both and
// keeps whichever scored better, so `{"a", "b"}` still finds a two-element list
// nested inside a list of lists. More, not less.
const List *entry_pattern(const ValuePtr &pattern)
{
    if (!pattern)
        return nullptr;
    const List *list = as_list(*pattern);
    return (list && list->size() == 2) ? list : nullptr;
}

struct Walk {
    const ValuePtr &pattern;
    const List *pair;
    int threshold;
    int max_depth;
    std::vector<SearchHit> &out;
    bool too_deep = false;

    // A score the caller asked for, or 0. Every level above the dial is dropped
    // HERE, once, so no other line in the walk has to remember the threshold.
    int keep(int score) const
    {
        return (score != SEARCH_NO_MATCH && score <= threshold) ? score
                                                                : SEARCH_NO_MATCH;
    }

    void record(const ValuePtr &value, const ValuePtr &key, const List &path,
                int score)
    {
        if (score != SEARCH_NO_MATCH)
            out.push_back(SearchHit{value, key, path, score});
    }

    void visit(const ValuePtr &node, const List &path, int depth);
};

void Walk::visit(const ValuePtr &node, const List &path, int depth)
{
    if (!node)
        return;

    // Bounded on the same argument every other recursion in the evaluator uses.
    // A merely very deep structure fails the way a very deep expression already
    // does, with a sentence naming the knob -- rather than by running out of
    // C++ stack, which reports nothing a program can act on.
    if (depth >= max_depth) {
        too_deep = true;
        return;
    }

    if (const List *list = as_list(*node)) {
        for (size_t i = 0; i < list->size(); i++) {
            const ValuePtr &item = (*list)[i];
            if (!item)
                continue;
            List here = path;
            here.push_back(make_value(Number(static_cast<long long>(i))));
            // The element AS A VALUE -- which for a nested container is the
            // comparator's container-to-container arm, and for a scalar is the
            // ladder. One call either way; the walk does not care which.
            record(item, nullptr, here, keep(search_score(*pattern, *item)));
            visit(item, here, depth + 1);
        }
        return;
    }

    // map_view(), so this arm walks a RESULT as well as a map. A result is a
    // map of string keys underneath and was built that way so that the answer a
    // search produced could be searched again by the power that produced it --
    // `found.search("typo")` over a list of results asks which of them mentions
    // one, and it costs this one word rather than a second walker.
    //
    // Its `alternatives` are NOT walked, and are not here to be walked: they
    // live beside the fields rather than in them, which is what stops one
    // resolution's results from being reachable from each other n times over.
    if (const MapBody *map = map_view(*node)) {
        for (const MapEntry &entry : map->entries) {
            if (!entry.key || !entry.value)
                continue;
            List here = path;
            here.push_back(entry.key);

            // Every way this entry could have matched, best one kept. One hit
            // per entry and not three: a key match and a value match are two
            // reasons for the same answer, and reporting the answer twice would
            // make .length() a count of reasons rather than of findings.
            int score = score_either(search_score(*pattern, *entry.key),
                                     search_score(*pattern, *entry.value));
            if (pair && (*pair)[0] && (*pair)[1]) {
                score = score_either(
                    score, score_both(search_score(*(*pair)[0], *entry.key),
                                      search_score(*(*pair)[1], *entry.value)));
            }

            // THE VALUE, even when it was the key that matched: "find me bolt"
            // means "give me what bolt points at", which is the question the
            // request was actually asking. The key travels alongside it, so a
            // program that wanted the key has it without a second search.
            record(entry.value, entry.key, here, keep(score));
            visit(entry.value, here, depth + 1);
        }
        return;
    }

    // A SPACESUIT IS A LEAF, and DECISION 4 is why. Lists and maps are built
    // then frozen, so a container can only hold values that existed before it
    // did and neither can form a cycle. An object CAN close one -- value.hpp
    // says so plainly and calls the leak the honest cost of reference semantics
    // -- so a walker that followed an ObjectPtr would hang on a structure the
    // language permits. It was already scored as a value by its parent.
}

} // namespace

bool search_walk(const ValuePtr &root, const ValuePtr &pattern, int threshold,
                 int max_depth, std::vector<SearchHit> &out)
{
    if (!root || !pattern)
        return true;

    const size_t first = out.size();
    Walk walk{pattern, entry_pattern(pattern), threshold, max_depth, out};

    // The ROOT ITSELF is scored before anything inside it, so a pattern that
    // describes the whole structure finds the whole structure. Its path is
    // empty, which is what an empty path means: this one, right here.
    walk.record(root, nullptr, List{},
                walk.keep(search_score(*pattern, *root)));
    walk.visit(root, List{}, 0);

    // BEST FIRST, and walk order breaking ties. stable_sort and not sort: two
    // hits at the same level are ordered by where they were found, and an
    // unstable sort would make that order depend on the implementation --
    // which is the same complaint value.hpp makes about an unordered map, one
    // level up. A program that walks results has to get the same answer twice.
    std::stable_sort(out.begin() + static_cast<long>(first), out.end(),
                     [](const SearchHit &a, const SearchHit &b) {
                         return a.score < b.score;
                     });
    return !walk.too_deep;
}

} // namespace satellite
