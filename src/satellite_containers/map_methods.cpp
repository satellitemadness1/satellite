// The map's nine rows -- `1 4 1 1` through `1 4 1 9` -- and the minted
// `search(pattern)` `1 4 1 10`. See satellite_containers/handlers.hpp.
//
// THE KEY CONTRACT IS THE WHOLE OF WHAT MAKES THIS A SEPARATE FILE from the
// list's. A key is a string or a number and can never be anything else
// (DESIGN §8.4, and §6.5 is the reason one level down: a key's hash and
// equality run inside a mutation, so they must be native and can never be
// satellite code). map_key_of in satellite_value/ is the canonicaliser, and
// S0727 is the one sentence every "that cannot be a key" failure quotes.
//
// A MISS IS AN ERROR AND NOT NOTHING (S0726), which is v1's rule kept for a
// reason M12 made sharper than it was: `nothing` is a value an entry can
// legitimately hold, so "absent" and "present but holding nothing" must not
// be one answer. `has(k)` is how a program asks without risking it.

#include "error_reporter/report.hpp"
#include "satellite_containers/containers.hpp"
#include "satellite_containers/methods_internal.hpp"
#include "satellite_containers/search.hpp"
#include "satellite_value/render.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <utility>

namespace satellite::containers {
namespace {

bool map_set(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const MapBody *self = nullptr;
    std::string canonical;
    if (!map_at(m, a, 0, &self) || !key_at(m, a, 1, &canonical))
        return false;
    MapBody next;
    // key_at already proved the key canonicalises, so this cannot fail --
    // map_with re-derives it rather than taking the string, because the body
    // transform is the one place that owns the index's invariant.
    map_with(*self, a[1], a[2], next);
    *answer = Value::map(std::move(next));
    return true;
}

bool map_get(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const MapBody *self = nullptr;
    std::string canonical;
    if (!map_at(m, a, 0, &self) || !key_at(m, a, 1, &canonical))
        return false;
    const auto found = self->index.find(canonical);
    if (found == self->index.end()) {
        m.refuse(errors::make<errors::Code::EVAL_NO_SUCH_KEY>(
            m.span_of(m.here()), text_of(a[1])));
        return false;
    }
    *answer = self->entries[found->second].value;
    return true;
}

// `has` IS THE QUESTION `get` REFUSES, so an unusable key is still a
// question with an answer -- `m.has(a_list)` is false rather than S0727.
// That is the one place this file parts from key_at: asking whether a map
// holds something it could never hold is a fair question, and answering it
// with a refusal would make `has` unusable as the guard `get`'s own sentence
// tells people to reach for.
bool map_has(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const MapBody *self = nullptr;
    if (!map_at(m, a, 0, &self))
        return false;
    std::string canonical;
    *answer = Value::boolean(map_key_of(a[1], canonical) &&
                             self->index.count(canonical) != 0);
    return true;
}

bool map_size(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const MapBody *self = nullptr;
    if (!map_at(m, a, 0, &self))
        return false;
    *answer = Value::number(Number(static_cast<long long>(self->entries.size())));
    return true;
}

bool map_empty(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const MapBody *self = nullptr;
    if (!map_at(m, a, 0, &self))
        return false;
    *answer = Value::boolean(self->entries.empty());
    return true;
}

bool map_clear(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const MapBody *self = nullptr;
    if (!map_at(m, a, 0, &self))
        return false;
    *answer = Value::map(MapBody{});
    return true;
}

// REMOVING WHAT WAS NEVER THERE IS AN ERROR, symmetric with `get`: it is a
// bug in the program, and `has(k)` is how to ask first.
bool map_remove(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const MapBody *self = nullptr;
    std::string canonical;
    if (!map_at(m, a, 0, &self) || !key_at(m, a, 1, &canonical))
        return false;
    MapBody next;
    if (!map_without(*self, a[1], next)) {
        m.refuse(errors::make<errors::Code::EVAL_NO_SUCH_KEY>(
            m.span_of(m.here()), text_of(a[1])));
        return false;
    }
    *answer = Value::map(std::move(next));
    return true;
}

// INSERTION ORDER, which is the whole reason MapBody keeps a vector beside
// its index: `.keys` is how a map is walked, since DESIGN §6's only loop is
// the C-shaped `for` and the language has no foreach.
bool map_keys(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const MapBody *self = nullptr;
    if (!map_at(m, a, 0, &self))
        return false;
    List out;
    out.reserve(self->entries.size());
    for (const MapEntry &entry : self->entries)
        out.push_back(entry.key);
    *answer = Value::list(std::move(out));
    return true;
}

bool map_values(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const MapBody *self = nullptr;
    if (!map_at(m, a, 0, &self))
        return false;
    List out;
    out.reserve(self->entries.size());
    for (const MapEntry &entry : self->entries)
        out.push_back(entry.value);
    *answer = Value::list(std::move(out));
    return true;
}

bool map_search(eval::Machine &m, const Value *a, uint32_t, Value *answer)
{
    const MapBody *self = nullptr;
    if (!map_at(m, a, 0, &self))
        return false;
    *answer = search_collect(a[0], a[1], true, m.search_threshold());
    return true;
}

} // namespace

void install_map_methods()
{
    eval::Handlers &table = eval::Handlers::table();
    const auto path = [](words::NodeId id) {
        return static_cast<words::PathId>(id);
    };

    table.install(path(words::NodeId::CONTAINER_MAP_SET),
                  {map_set, true, 3, "M16", true});
    table.install(path(words::NodeId::CONTAINER_MAP_GET),
                  {map_get, true, 2, "M16"});
    table.install(path(words::NodeId::CONTAINER_MAP_HAS),
                  {map_has, true, 2, "M16"});
    table.install(path(words::NodeId::CONTAINER_MAP_SIZE),
                  {map_size, true, 1, "M16"});
    table.install(path(words::NodeId::CONTAINER_MAP_EMPTY),
                  {map_empty, true, 1, "M16"});
    table.install(path(words::NodeId::CONTAINER_MAP_CLEAR),
                  {map_clear, true, 1, "M16", true});
    table.install(path(words::NodeId::CONTAINER_MAP_REMOVE),
                  {map_remove, true, 2, "M16", true});
    table.install(path(words::NodeId::CONTAINER_MAP_KEYS),
                  {map_keys, true, 1, "M16"});
    table.install(path(words::NodeId::CONTAINER_MAP_VALUES),
                  {map_values, true, 1, "M16"});
    table.install(path(words::NodeId::CONTAINER_MAP_SEARCH),
                  {map_search, true, 2, "M16"});
}

} // namespace satellite::containers
