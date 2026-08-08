// §14: collecting suits, linking supers, and flattening the layout.
//
// Part of env/, split from an 854-line env.cpp.

#include "env_internal.hpp"

namespace satellite {

void Resolver::collect_suits(const Program &program)
{
    for (const TopLevel &item : program.items) {
        const Spacesuit *suit = std::get_if<Spacesuit>(&item);
        if (!suit)
            continue;

        if (suit->name.empty())
            continue;   // the parser already reported why

        // One namespace for both, because both are named by a bare word and
        // `foo(1)` would otherwise have two readings.
        if (out_.capsules.count(suit->name)) {
            fail(suit->span, suit->name +
                             " is already a capsule; a spacesuit and a capsule "
                             "cannot share a name");
            continue;
        }

        auto inserted = out_.suits.emplace(suit->name, SpacesuitInfo{});
        if (!inserted.second) {
            fail(suit->span, "spacesuit " + suit->name + " is already defined");
            continue;
        }
        inserted.first->second.suit = suit;
        inserted.first->second.name = suit->name;
    }
}

void Resolver::link_supers()
{
    for (auto &entry : out_.suits) {
        SpacesuitInfo &info = entry.second;
        if (!info.suit || info.suit->super.empty())
            continue;

        auto found = out_.suits.find(info.suit->super);
        if (found == out_.suits.end()) {
            fail(info.suit->super_span,
                 "no such spacesuit to inherit from: " + info.suit->super);
            continue;
        }
        if (&found->second == &info) {
            fail(info.suit->super_span, info.name + " inherits from itself");
            continue;
        }
        info.super = &found->second;
    }
}

// Every later pass walks the superclass chain assuming it ends, so the cycles
// have to be gone before any of them runs — not reported by the first one that
// hangs. Both checks are needed: a suit ON a cycle finds itself, and a suit
// merely POINTING INTO one never does, so it needs the step bound.
void Resolver::break_inheritance_cycles()
{
    const size_t limit = out_.suits.size();
    for (auto &entry : out_.suits) {
        SpacesuitInfo &info = entry.second;
        size_t steps = 0;
        for (const SpacesuitInfo *walk = info.super; walk; walk = walk->super) {
            if (walk == &info) {
                fail(info.suit->super_span,
                     info.name + " inherits from itself through " +
                     info.suit->super);
                info.super = nullptr;
                break;
            }
            if (++steps > limit) {
                fail(info.suit->super_span,
                     "the inheritance chain above " + info.name +
                     " does not terminate");
                info.super = nullptr;
                break;
            }
        }
    }
}

// The layout: the superclass's fields and methods copied in, then this suit's
// own appended (fields) or overriding (methods). Copying rather than chaining
// is what leaves a field access as one index and a method call as one hash
// lookup, with inheritance costing nothing at the point of use.
void Resolver::build_layout(SpacesuitInfo &info)
{
    if (info.super) {
        info.fields = info.super->fields;
        info.methods = info.super->methods;
    }

    for (const SuitItem &item : info.suit->items) {
        const Field *field = std::get_if<Field>(&item);
        if (!field)
            continue;

        check_type(field->type, field->span);

        if (int at = info.find_field(field->name); at >= 0) {
            const SpacesuitInfo *owner = info.fields[static_cast<size_t>(at)].owner;
            fail(field->span,
                 "field " + field->name + " is already declared in " +
                 (owner && owner != &info ? owner->name
                                          : std::string("this spacesuit")));
            continue;
        }

        FieldInfo entry;
        entry.name = field->name;
        entry.type = field->type;
        entry.access = field->access;
        entry.init = field->init.get();
        entry.owner = &info;
        info.fields.push_back(std::move(entry));
    }

    // The constructor chain: the superclass's, then this suit's own.
    if (info.super)
        info.ctors = info.super->ctors;

    // Every method goes in BEFORE any body is walked, so a method may call one
    // declared further down the suit — the same reason capsule names are
    // collected before capsule bodies.
    std::vector<const Method *> own;
    for (const SuitItem &item : info.suit->items) {
        const Method *method = std::get_if<Method>(&item);
        if (!method || method->capsule.name.empty())
            continue;

        if (method->constructor) {
            if (!info.ctors.empty() && info.ctors.back().owner == &info) {
                fail(method->capsule.span,
                     info.name + " already has a constructor");
                continue;
            }
            MethodInfo entry;
            entry.access = method->access;
            entry.owner = &info;
            entry.info = std::make_shared<CapsuleInfo>();
            entry.info->capsule = &method->capsule;
            entry.info->param_count = method->capsule.params.size();
            info.ctors.push_back(std::move(entry));
            own.push_back(method);
            continue;
        }

        const std::string &name = method->capsule.name;
        if (info.find_field(name) >= 0) {
            fail(method->capsule.span,
                 name + " is already a field of " + info.name +
                 "; a field and a method cannot share a name");
            continue;
        }

        // A name defined twice in ONE suit is a mistake; the same name defined
        // in a superclass is an override, which is the point.
        bool duplicate = false;
        for (const Method *seen : own)
            duplicate = duplicate || seen->capsule.name == name;
        if (duplicate) {
            fail(method->capsule.span,
                 "method " + name + " is already defined in " + info.name);
            continue;
        }

        MethodInfo entry;
        entry.access = method->access;
        entry.owner = &info;
        entry.info = std::make_shared<CapsuleInfo>();
        entry.info->capsule = &method->capsule;
        entry.info->param_count = method->capsule.params.size();
        info.methods.insert_or_assign(name, std::move(entry));
        own.push_back(method);
    }

    // Only the last constructor in the chain is handed the site's arguments, so
    // every earlier one has to be callable with none. Reported here rather than
    // at the construction site, because it is a fact about the two spacesuits
    // and holds however many times an instance is built.
    for (size_t i = 0; i + 1 < info.ctors.size(); i++) {
        const MethodInfo &ctor = info.ctors[i];
        if (!ctor.info || ctor.info->param_count == 0)
            continue;
        fail(info.suit->super_span,
             info.name + " inherits from " +
             (ctor.owner ? ctor.owner->name : std::string("a spacesuit")) +
             ", whose constructor takes " +
             std::to_string(ctor.info->param_count) +
             (ctor.info->param_count == 1 ? " argument" : " arguments") +
             ", and there is no syntax yet for passing them");
    }

    own_methods_[&info] = std::move(own);
}

// Bodies, once every spacesuit's layout exists — a method may construct a
// spacesuit declared further down the file, and its constructor's arity has to
// be known before that call can be checked.
void Resolver::resolve_bodies(SpacesuitInfo &info)
{
    for (const Method *method : own_methods_[&info]) {
        const std::shared_ptr<CapsuleInfo> &target =
            method->constructor ? info.ctors.back().info
                                : info.methods.at(method->capsule.name).info;
        suit_ = &info;
        field_limit_ = info.fields.size();
        resolve_capsule(method->capsule, *target);
        suit_ = nullptr;
    }

    // Field initialisers, each with only the fields declared BEFORE it in
    // scope. They resolve with no frame, because construction happens at a
    // declaration rather than inside a call — an initialiser has a receiver but
    // no activation.
    const size_t inherited = info.super ? info.super->fields.size() : 0;
    for (size_t i = inherited; i < info.fields.size(); i++) {
        if (!info.fields[i].init)
            continue;
        info_ = nullptr;
        scopes_.clear();
        suit_ = &info;
        field_limit_ = i;
        resolve_expr(*info.fields[i].init);
        suit_ = nullptr;
    }
}

void Resolver::resolve_suit(SpacesuitInfo &info)
{
    // Exactly once each. A diamond (two suits inheriting one base) reaches the
    // base twice, and building a layout twice would append its own fields on
    // top of themselves and then report every one of them as a duplicate.
    if (!laid_out_.insert(&info).second)
        return;

    // Superclass first: this suit's layout is built on top of a finished one.
    // Cycles are already gone, so the recursion terminates.
    if (info.super)
        resolve_suit(out_.suits.at(info.suit->super));

    build_layout(info);
}

} // namespace satellite
