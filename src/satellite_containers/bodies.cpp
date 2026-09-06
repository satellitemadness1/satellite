// The map's two write transforms. See satellite_containers/containers.hpp.
//
// COPY-ON-WRITE, SO A SET IS O(n) -- parity with the language's only other
// insertion primitive rather than a new cost: `.append` copies the whole
// backing vector too, and DESIGN §12 records the in-place fast path as
// deliberately deferred. v1 measured the shape at 0.04 / 0.18 / 0.77 s to
// build maps of 2000 / 4000 / 8000, and the index is what keeps LOOKUP off
// that curve.

#include "satellite_containers/containers.hpp"

#include <string>
#include <utility>

namespace satellite::containers {

bool map_with(const MapBody &current, const Value &key, const Value &value,
              MapBody &next)
{
    std::string canonical;
    if (!map_key_of(key, canonical))
        return false;

    next = current;
    const auto found = next.index.find(canonical);
    if (found != next.index.end()) {
        next.entries[found->second].value = value;
        return true;
    }
    next.index.emplace(std::move(canonical), next.entries.size());
    next.entries.push_back(MapEntry{key, value});
    return true;
}

bool map_without(const MapBody &current, const Value &key, MapBody &next)
{
    std::string canonical;
    if (!map_key_of(key, canonical))
        return false;

    const auto found = current.index.find(canonical);
    if (found == current.index.end())
        return false;

    const size_t gone = found->second;
    next.entries.clear();
    next.entries.reserve(current.entries.size() - 1);
    for (size_t i = 0; i < current.entries.size(); i++)
        if (i != gone)
            next.entries.push_back(current.entries[i]);
    next.index.clear();
    next.index.reserve(next.entries.size());
    for (size_t i = 0; i < next.entries.size(); i++) {
        std::string rebuilt;
        map_key_of(next.entries[i].key, rebuilt); // validated on insertion
        next.index.emplace(std::move(rebuilt), i);
    }
    return true;
}

} // namespace satellite::containers
