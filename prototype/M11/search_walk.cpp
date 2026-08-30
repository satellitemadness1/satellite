// The walker: everything inside a value, and where it was.
// Layer 2 of the search power -- Milestone 11 Prototype in prototype/M11.

#include "search.hpp"
#include <algorithm>

namespace satellite {
namespace {

const List *entry_pattern(const ValuePtr &pattern)
{
    if (!pattern || !pattern->is_list())
        return nullptr;
    auto list_ref = std::get<ListRef>(*pattern);
    return (list_ref && list_ref->size() == 2) ? list_ref.get() : nullptr;
}

struct Walk {
    const ValuePtr &pattern;
    const List *pair;
    int threshold;
    int max_depth;
    std::vector<SearchHit> &out;
    bool too_deep = false;

    int keep(int score) const
    {
        return (score != SEARCH_NO_MATCH && score <= threshold) ? score : SEARCH_NO_MATCH;
    }

    void record(const ValuePtr &value, const ValuePtr &key, const List &path, int score)
    {
        if (score != SEARCH_NO_MATCH)
            out.push_back(SearchHit{value, key, path, score});
    }

    void visit(const ValuePtr &node, const List &path, int depth);
};

void Walk::visit(const ValuePtr &node, const List &path, int depth)
{
    if (!node) return;

    if (depth >= max_depth) {
        too_deep = true;
        return;
    }

    if (node->is_list()) {
        auto list_ref = std::get<ListRef>(*node);
        if (list_ref) {
            for (size_t i = 0; i < list_ref->size(); i++) {
                const ValuePtr &item = (*list_ref)[i];
                if (!item) continue;
                List here = path;
                here.push_back(std::make_shared<Value>(Value::number(Number(static_cast<int64_t>(i)))));
                record(item, nullptr, here, keep(search_score(*pattern, *item)));
                visit(item, here, depth + 1);
            }
        }
        return;
    }

    if (std::holds_alternative<MapRef>(*node)) {
        auto map_ref = std::get<MapRef>(*node);
        if (map_ref) {
            for (const MapEntry &entry : map_ref->entries) {
                if (!entry.key || !entry.value) continue;
                List here = path;
                here.push_back(entry.key);

                int score = score_either(search_score(*pattern, *entry.key),
                                         search_score(*pattern, *entry.value));
                if (pair && (*pair)[0] && (*pair)[1]) {
                    score = score_either(
                        score, score_both(search_score(*(*pair)[0], *entry.key),
                                          search_score(*(*pair)[1], *entry.value)));
                }

                record(entry.value, entry.key, here, keep(score));
                visit(entry.value, here, depth + 1);
            }
        }
        return;
    }
}

} // namespace

bool search_walk(const ValuePtr &root, const ValuePtr &pattern, int threshold,
                 int max_depth, std::vector<SearchHit> &out)
{
    if (!root || !pattern)
        return true;

    const size_t first = out.size();
    Walk walk{pattern, entry_pattern(pattern), threshold, max_depth, out};

    walk.record(root, nullptr, List{}, walk.keep(search_score(*pattern, *root)));
    walk.visit(root, List{}, 0);

    std::stable_sort(out.begin() + static_cast<long>(first), out.end(),
                     [](const SearchHit &a, const SearchHit &b) {
                         return a.score < b.score;
                     });
    return !walk.too_deep;
}

} // namespace satellite

