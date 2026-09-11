// A group word, displaying its children. See satellite_system/group_map.hpp.

#include "satellite_system/group_map.hpp"

#include "evaluator/machine.hpp"
#include "satellite_containers/containers.hpp"
#include "satellite_string/satellite_string.hpp"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace satellite::system {

namespace {

// The row that answers a node with NO arguments, or nullptr.
//
// TWO PLACES TO LOOK AND THE SECOND IS WHAT MAKES A GROUP HOLD A GROUP.
// `free` is a word whose own row takes nothing, so its handler is on the node.
// `swap` is a PARENT, and a parent's no-argument answer is its bare `()` child
// -- `swap()` `1 22 4 4 0`. Asking both is what lets `memory()` show `swap`
// beside `free` without either of them being a special case here.
const eval::Handler *answers_bare(words::PathId node)
{
    if (const eval::Handler *found = eval::Handlers::table().find(node);
        found != nullptr && found->arity == 0 && !found->binds_receiver)
        return found;
    for (words::PathId c = words::first_child(static_cast<words::NodeId>(node));
         c != words::kNoPath; c = words::next_sibling(c))
        if (words::text_of(static_cast<words::NodeId>(c)) == "()")
            if (const eval::Handler *bare = eval::Handlers::table().find(c);
                bare != nullptr && bare->arity == 0)
                return bare;
    return nullptr;
}

} // namespace

bool children_of(eval::Machine &machine, words::NodeId group, Value *answer)
{
    // THE CHILDREN COME OUT OF THE REGISTRY AND THE UNIT SHAPES COLLAPSE INTO
    // THEM. `free()` `1 22 4 6` and `free(unit)` `1 22 4 10` are two rows and
    // one WORD, so the map is keyed by the spelling and the arity-0 row is
    // what answers it -- which is also why the entries come out in the default
    // unit with no way to ask for another. A program that wants gigabytes asks
    // `memory.total("gb")`; this is the whole section, at a glance.
    //
    // A CHILD WITH NO NO-ARGUMENT ANSWER IS LEFT OUT rather than filled with
    // `nothing`. Today that is `this` `1 22 4 5`, whose bare shape
    // `1 22 4 5 0` no milestone has built -- and leaving it out says that,
    // where a `nothing` beside seven numbers would read as a fact about this
    // thread's stack.
    MapBody built;
    std::vector<std::string> taken;
    for (words::PathId c = words::first_child(group); c != words::kNoPath;
         c = words::next_sibling(c)) {
        const std::string_view word =
            words::spelling_of(static_cast<words::NodeId>(c));
        if (word.empty())
            continue;               // the bare `()` row names no child
        const std::string key(word);
        if (std::find(taken.begin(), taken.end(), key) != taken.end())
            continue;               // the unit shape of a word already taken

        const eval::Handler *handler = answers_bare(c);
        if (handler == nullptr)
            continue;
        Value one;
        // A HANDLER WITH ARITY 0 READS NO ARGUMENTS, which is what
        // answers_bare() checked before handing it back -- so a null pointer
        // and a count of nothing is the whole call, and this does not need the
        // machine's value stack to make one.
        if (!handler->fn(machine, nullptr, 0, &one))
            return false;           // the handler has already refused
        taken.push_back(key);

        // map_with RATHER THAN A DIRECT push_back, because
        // satellite_containers/bodies.cpp owns the index's invariant and a
        // second builder here would be a second place that has to keep it.
        MapBody next;
        containers::map_with(built, Value::string(encode_raw(key)), one, next);
        built = std::move(next);
    }
    *answer = Value::map(std::move(built));
    return true;
}

} // namespace satellite::system
