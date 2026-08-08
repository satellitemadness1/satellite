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

// A spacesuit type names a spacesuit, and this is where that is checked —
// before anything runs, rather than at the declaration that would have
// constructed the instance. Recursive because the type may be a container's
// element type: satellite.container.list<my_class> is a list of instances.
void Resolver::check_type(const Type &type, Span span)
{
    if (type.is_spacesuit() && !out_.suits.count(type.name))
        fail(type.span.end > type.span.start ? type.span : span,
             "no such spacesuit: " + type.name);
    for (const Type &arg : type.args)
        check_type(arg, span);
}

} // namespace satellite
