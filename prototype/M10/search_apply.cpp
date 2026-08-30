// Layer 3: the dial, and what search_collect hands back.
// Milestone 10 Prototype in prototype/M10.

#include "search.hpp"

namespace satellite {
namespace {

thread_local int g_threshold = SEARCH_EXACT;

ValuePtr hit_as_map(const SearchHit &hit)
{
    auto map_body = std::make_shared<MapBody>();
    auto put = [&](const char *name, const ValuePtr &value) {
        std::string canonical;
        Value key_val = Value::sat_string(encode(name));
        map_key_of(key_val, canonical);
        map_body->index[canonical] = map_body->entries.size();
        map_body->entries.push_back(MapEntry{
            std::make_shared<Value>(key_val),
            value ? value : std::make_shared<Value>(Value::nil())
        });
    };

    put("value", hit.value);
    put("key", hit.key);
    put("path", std::make_shared<Value>(Value(std::make_shared<const List>(hit.path))));
    put("score", std::make_shared<Value>(Value::number(Number(static_cast<int64_t>(hit.score)))));
    return std::make_shared<Value>(Value(std::const_pointer_cast<const MapBody>(map_body)));
}

} // namespace

int search_threshold() { return g_threshold; }

void set_search_threshold(int level) { g_threshold = level; }

ValuePtr search_collect(const ValuePtr &target, const ValuePtr &pattern,
                        bool rich, int max_depth, std::string &error)
{
    std::vector<SearchHit> hits;
    if (!search_walk(target, pattern, g_threshold, max_depth, hits)) {
        error = "this structure nests deeper than satellite.library.system."
                "max_depth (" + std::to_string(max_depth) +
                "), so a search cannot finish walking it";
        return nullptr;
    }

    auto out = std::make_shared<List>();
    out->reserve(hits.size());
    for (const SearchHit &hit : hits) {
        out->push_back(rich ? hit_as_map(hit)
                            : (hit.value ? hit.value : std::make_shared<Value>(Value::nil())));
    }
    return std::make_shared<Value>(Value(std::const_pointer_cast<const List>(out)));
}

} // namespace satellite

