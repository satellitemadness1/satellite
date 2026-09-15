// The walker: everything inside a value, and where it was. Layer two of the
// search power -- v1's `evaluator/search_walk.cpp`, ported onto its own stack.
//
// It knows LISTS and MAPS and nothing about how two values are compared --
// every comparison here is one call to search_score. What changed from v1 is
// recorded in search.hpp: the walk keeps its own heap stack and reads no depth
// dial, because DESIGN §7.5 says a walk over user-controlled depth may not use
// the C++ stack and a byte ceiling on the control stack is not a count of
// levels. The traversal ORDER is v1's exactly -- an item is recorded, then its
// inside, then its next sibling -- because ties in the final sort break by
// walk order and a program that walks results has to get the same answer
// twice.

#include "satellite_containers/search.hpp"

#include "satellite_containers/containers.hpp"
#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace satellite::containers {
namespace {

// A two-element list is an ENTRY pattern -- key then value -- and this is the
// only place the shape is recognised. It does not STOP the pattern being an
// ordinary value: the walk tries both and keeps whichever scored better, so a
// two-element list nested in a list of lists still finds itself. More, not
// less: that is the whole instruction.
const List *entry_pattern(const Value &pattern)
{
    const List *list = as_list(pattern);
    return (list && list->size() == 2) ? list : nullptr;
}

struct Walk {
    const Value &pattern;
    const List *pair;
    int threshold;
    std::vector<SearchHit> &out;

    // ONE path, shared by the whole walk: a segment is pushed when the walk
    // enters a container's child and popped when it leaves, and only a HIT
    // copies it. v1 copied the path once per child instead, which is the same
    // answer for shallow structures and quadratic for a deep chain.
    List path;

    // A score the caller asked for, or 0. Every level above the dial is
    // dropped HERE, once, so no other line in the walk has to remember the
    // threshold.
    int keep(int score) const
    {
        return (score != SEARCH_NO_MATCH && score <= threshold)
                   ? score
                   : SEARCH_NO_MATCH;
    }

    void record(const Value &value, const Value *key, int score)
    {
        if (score != SEARCH_NO_MATCH)
            out.push_back(SearchHit{value, key ? *key : Value::nothing(),
                                    path, score});
    }
};

// What is left to do. `Enter` records one child and then walks inside it;
// `Leave` pops the child's path segment on the way back out.
struct Step {
    const Value *node = nullptr; // Enter: the child. Leave: null.
    const Value *key = nullptr;  // Enter, map entry: the stored key
    Value segment;               // Enter: the path segment this child adds
};

// Push one container's children, in REVERSE so they pop in order. The child
// itself is recorded when its Enter step runs, which is v1's order: record,
// walk inside, next sibling.
void spread(const Value &node, std::vector<Step> &steps)
{
    if (const List *list = as_list(node)) {
        for (size_t i = list->size(); i-- > 0;)
            steps.push_back(Step{&(*list)[i], nullptr,
                                 Value::number(Number(
                                     static_cast<long long>(i)))});
        return;
    }
    if (const MapBody *map = as_map(node)) {
        for (size_t i = map->entries.size(); i-- > 0;) {
            const MapEntry &entry = map->entries[i];
            steps.push_back(Step{&entry.value, &entry.key, entry.key});
        }
    }
    // A scalar holds nothing to walk. It was already scored by its parent.
}

} // namespace

void search_walk(const Value &root, const Value &pattern, int threshold,
                 std::vector<SearchHit> &out)
{
    const size_t first = out.size();
    Walk walk{pattern, entry_pattern(pattern), threshold, out, List{}};

    // The ROOT ITSELF is scored before anything inside it, so a pattern that
    // describes the whole structure finds the whole structure. Its path is
    // empty, which is what an empty path means: this one, right here.
    walk.record(root, nullptr, walk.keep(search_score(pattern, root)));

    std::vector<Step> steps;
    spread(root, steps);

    while (!steps.empty()) {
        Step step = std::move(steps.back());
        steps.pop_back();

        if (step.node == nullptr) {
            walk.path.pop_back();
            continue;
        }

        walk.path.push_back(std::move(step.segment));
        steps.push_back(Step{}); // the Leave that pops the segment

        if (step.key == nullptr) {
            // A list element, as a value -- which for a nested container is
            // the comparator's container-to-container arm, and for a scalar
            // is the ladder. One call either way.
            walk.record(*step.node, nullptr,
                        walk.keep(search_score(pattern, *step.node)));
        } else {
            // Every way this map entry could have matched, best one kept. One
            // hit per entry and not three: a key match and a value match are
            // two reasons for the same answer, and reporting it twice would
            // make `.size()` a count of reasons rather than of findings.
            int score = score_either(search_score(pattern, *step.key),
                                     search_score(pattern, *step.node));
            if (walk.pair)
                score = score_either(
                    score,
                    score_both(search_score((*walk.pair)[0], *step.key),
                               search_score((*walk.pair)[1], *step.node)));

            // THE VALUE, even when it was the key that matched: "find me
            // bolt" means "give me what bolt points at". The key travels
            // alongside it, so a program that wanted the key has it without a
            // second search.
            walk.record(*step.node, step.key, walk.keep(score));
        }

        spread(*step.node, steps);
    }

    // BEST FIRST, and walk order breaking ties. stable_sort and not sort: two
    // hits at the same level are ordered by where they were found, and an
    // unstable sort would make that order depend on the implementation. A
    // program that walks results has to get the same answer twice.
    std::stable_sort(out.begin() + static_cast<long>(first), out.end(),
                     [](const SearchHit &a, const SearchHit &b) {
                         return a.score < b.score;
                     });
}

namespace {

// One hit, as a map a program can read: value, key, path, score -- the rich
// spelling's shape, v1's DECISION 7a. Built through map_with so the side
// index is maintained by the one function that owns that invariant.
Value hit_as_map(const SearchHit &hit)
{
    MapBody body, next;
    const auto put = [&](const char *name, Value value) {
        if (map_with(body, Value::string(encode_raw(name)), std::move(value),
                     next))
            body = std::move(next);
        next = MapBody{};
    };

    put("value", hit.value);
    put("key", hit.key);
    put("path", Value::list(hit.path));
    put("score", Value::number(Number(static_cast<long long>(hit.score))));
    return Value::map(std::move(body));
}

} // namespace

Value search_collect(const Value &target, const Value &pattern, bool rich,
                     int threshold)
{
    std::vector<SearchHit> hits;
    search_walk(target, pattern, threshold, hits);

    List out;
    out.reserve(hits.size());
    for (SearchHit &hit : hits)
        out.push_back(rich ? hit_as_map(hit) : std::move(hit.value));
    return Value::list(std::move(out));
}

} // namespace satellite::containers
