// Scopes, slot allocation, and type checking at a declaration.
//
// Part of env/, split from an 854-line env.cpp.

#include "env_internal.hpp"

namespace satellite {

std::string capsule_key(const Capsule &capsule)
{
    return capsule.reserved ? "satellite." + capsule.name : capsule.name;
}

const int *Resolver::lookup(const std::string &name) const
{
    // Innermost scope first, so an inner declaration shadows an outer one.
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        auto found = scope->find(name);
        if (found != scope->end())
            return &found->second;
    }
    return nullptr;
}

int Resolver::declare(const std::string &name, const Type &type, Span span)
{
    // §1's one reserved word, enforced at the binding site.
    if (name == "satellite") {
        fail(span, "satellite is reserved and cannot be declared");
        return SLOT_GLOBAL;
    }

    // Top-level declarations are globals. §6 demotes capsule locals only, and
    // §10's main.x ruling means `satellite.variable.number x = 1` at the top
    // level has to stay observable as satellite.library.main.x.
    if (!info_)
        return SLOT_GLOBAL;

    Scope &scope = scopes_.back();
    auto found = scope.find(name);
    if (found != scope.end()) {
        fail(span, name + " is already declared in this scope");
        return found->second;
    }

    // Slots are never reused across scopes. Reuse would save a few pointers
    // per frame and cost the ability to name a slot in a diagnostic.
    int slot = static_cast<int>(info_->slot_count++);
    info_->slot_types.push_back(type);
    info_->slot_names.push_back(name);
    scope.emplace(name, slot);
    return slot;
}

// The fields of suit_ that are in scope right now, searched linearly because
// this runs once per name at resolve time and never during the walk.
int Resolver::lookup_field(const std::string &name) const
{
    if (!suit_)
        return -1;
    const size_t limit = std::min(field_limit_, suit_->fields.size());
    for (size_t i = 0; i < limit; i++)
        if (suit_->fields[i].name == name)
            return static_cast<int>(i);
    return -1;
}

// A spacesuit type names a spacesuit, and a container names a container with
// the right number of arguments. Both are checked here — before anything runs,
// rather than at the declaration that would have constructed the instance.
//
// Recursive because a type may be a container's element type:
// satellite.container.list<my_class> is a list of instances, and
// satellite.container.map<satellite.variable.string, list<my_class>> nests two
// deep.
//
// The container half is what makes `type.args[1]` safe to read at every runtime
// site that touches a map — matches(), the insertion check in call_mutator. The
// parser deliberately accepts any word as a type name (it validates only the
// SPACE), so without this a `map<K>` reaches eval and reads past the end.
//
// The rule is "0 or N", never "exactly N": matches() treats an empty argument
// list as "matches anything", and a bare satellite.container.list is legal
// today. This newly rejects satellite.container.list<A, B> and
// satellite.container.zzz, which run clean at present; no shipped example
// writes either.
void Resolver::check_type(const Type &type, Span span)
{
    const Span at = type.span.end > type.span.start ? type.span : span;

    if (type.is_spacesuit() && !out_.suits.count(type.name))
        fail(at, "no such spacesuit: " + type.name);

    // Only a container is generic. The parser deliberately accepts a `<...>`
    // list after ANY satellite-rooted type, because it validates the space and
    // not the name, so satellite.variable.number<A, B> parsed and resolved
    // clean before this — pre-existing laxness rather than anything the map
    // introduced, and closed here because the container half is checked below
    // and a half-checked shape reads as an oversight.
    if (type.space == "variable" && !type.args.empty())
        fail(at, "satellite.variable." + type.name +
                 " is not generic, so it takes no type arguments");

    if (type.space == "container") {
        if (type.name == "list") {
            if (type.args.size() > 1)
                fail(at, "satellite.container.list takes one type argument, "
                         "got " + std::to_string(type.args.size()));
        } else if (type.name == "map") {
            if (!type.args.empty() && type.args.size() != 2)
                fail(at, "satellite.container.map takes two type arguments "
                         "(a key and a value), got " +
                         std::to_string(type.args.size()));
            // The key restriction is §8.6, and it is checked here as well as at
            // insertion because a bare `map` with no arguments skips this and
            // must still be caught when a key arrives.
            if (type.args.size() == 2) {
                const Type &key = type.args[0];
                if (!(key.space == "variable" &&
                      (key.name == "string" || key.name == "number")))
                    fail(key.span.end > key.span.start ? key.span : at,
                         "a map key must be satellite.variable.string or "
                         "satellite.variable.number, not " + unparse(key));
            }
        } else {
            fail(at, "no such container type: " + type.name);
        }
    }

    for (const Type &arg : type.args)
        check_type(arg, span);
}

} // namespace satellite
