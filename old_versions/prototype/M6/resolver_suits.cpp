// Spacesuit resolution: collection, superclass linking, cycle breaking,
// layout flattening, method table override, constructor chaining, and body resolution.
// Part of Milestone 6 Resolver Prototype in prototype/M6.

#include "resolver.hpp"

namespace satellite {

void Resolver::collect_suits()
{
    for (NodeIndex idx : program_.items) {
        const auto &node = arena_.get(idx);
        if (node.kind != NodeKind::Spacesuit)
            continue;

        const auto &suit = std::get<SpacesuitDecl>(node.data);
        if (suit.name.empty())
            continue;

        // Check collision with capsules
        auto clash = out_.capsules.find(suit.name);
        if (clash != out_.capsules.end()) {
            std::string msg = suit.name + " is already a capsule; a spacesuit and a capsule cannot share a name";
            if (clash->second.capsule) {
                error_with_note(ErrorCode::E0205_DuplicateDeclaration, node.span, msg,
                                clash->second.capsule->span, "the capsule was declared here");
            } else {
                error(ErrorCode::E0205_DuplicateDeclaration, node.span, msg);
            }
            continue;
        }

        auto inserted = out_.suits.emplace(suit.name, SpacesuitInfo{});
        if (!inserted.second) {
            std::string msg = "spacesuit " + suit.name + " is already defined";
            const SpacesuitInfo &first = inserted.first->second;
            error_with_note(ErrorCode::E0205_DuplicateDeclaration, node.span, msg,
                            first.span, "first defined here");
            continue;
        }

        SpacesuitInfo &info = inserted.first->second;
        info.suit = &suit;
        info.node = idx;
        info.name = suit.name;
        info.path_id = suit.path_id;
        info.span = node.span;

        out_.table.set_slot(idx, SLOT_SUIT);
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
            error(ErrorCode::E0303_UndefinedSpacesuit, info.suit->super_span,
                  "no such spacesuit to inherit from: " + info.suit->super);
            continue;
        }
        if (&found->second == &info) {
            error(ErrorCode::E0303_UndefinedSpacesuit, info.suit->super_span,
                  info.name + " inherits from itself");
            continue;
        }
        info.super = &found->second;
    }
}

void Resolver::break_inheritance_cycles()
{
    const size_t limit = out_.suits.size();
    for (auto &entry : out_.suits) {
        SpacesuitInfo &info = entry.second;
        size_t steps = 0;
        for (const SpacesuitInfo *walk = info.super; walk; walk = walk->super) {
            if (walk == &info) {
                error(ErrorCode::E0303_UndefinedSpacesuit, info.suit->super_span,
                      info.name + " inherits from itself through " + info.suit->super);
                info.super = nullptr;
                break;
            }
            if (++steps > limit) {
                error(ErrorCode::E0303_UndefinedSpacesuit, info.suit->super_span,
                      "the inheritance chain above " + info.name + " does not terminate (cycle detected)");
                info.super = nullptr;
                break;
            }
        }
    }
}

void Resolver::build_layout(SpacesuitInfo &info)
{
    if (info.super) {
        info.fields = info.super->fields;
        info.methods = info.super->methods;
        info.ctors = info.super->ctors;
    }

    for (const SuitItem &item : info.suit->items) {
        if (const FieldDecl *field = std::get_if<FieldDecl>(&item)) {
            check_type(field->type, field->span);

            if (int at = info.find_field(field->name); at >= 0) {
                const SpacesuitInfo *owner = info.fields[static_cast<size_t>(at)].owner;
                std::string owner_str = (owner && owner != &info) ? owner->name : "this spacesuit";
                error(ErrorCode::E0205_DuplicateDeclaration, field->span,
                      "field " + field->name + " is already declared in " + owner_str);
                continue;
            }

            FieldInfo entry;
            entry.name = field->name;
            entry.type = field->type;
            entry.access = field->access;
            entry.init = field->init;
            entry.owner = &info;
            entry.index = static_cast<uint32_t>(info.fields.size());
            entry.span = field->span;
            info.fields.push_back(std::move(entry));
        }
    }

    std::vector<NodeIndex> own_list;
    for (const SuitItem &item : info.suit->items) {
        const MethodDecl *method = std::get_if<MethodDecl>(&item);
        if (!method || method->capsule.name.empty())
            continue;

        if (method->constructor) {
            if (!info.ctors.empty() && info.ctors.back().owner == &info) {
                error(ErrorCode::E0205_DuplicateDeclaration, method->capsule.span,
                      info.name + " already has a constructor");
                continue;
            }
            MethodInfo entry;
            entry.access = method->access;
            entry.constructor = true;
            entry.owner = &info;
            entry.span = method->span;
            entry.info = std::make_shared<CapsuleInfo>();
            entry.info->capsule = &method->capsule;
            entry.info->name = method->capsule.name;
            entry.info->param_count = method->capsule.params.size();
            info.ctors.push_back(std::move(entry));
            continue;
        }

        const std::string &name = method->capsule.name;
        if (info.find_field(name) >= 0) {
            error(ErrorCode::E0205_DuplicateDeclaration, method->capsule.span,
                  name + " is already a field of " + info.name +
                  "; a field and a method cannot share a name");
            continue;
        }

        auto it = info.methods.find(name);
        if (it != info.methods.end() && it->second.owner == &info) {
            error(ErrorCode::E0205_DuplicateDeclaration, method->capsule.span,
                  "method " + name + " is already defined in " + info.name);
            continue;
        }

        MethodInfo entry;
        entry.access = method->access;
        entry.constructor = false;
        entry.owner = &info;
        entry.span = method->span;
        entry.info = std::make_shared<CapsuleInfo>();
        entry.info->capsule = &method->capsule;
        entry.info->name = method->capsule.name;
        entry.info->param_count = method->capsule.params.size();
        info.methods.insert_or_assign(name, std::move(entry));
    }

    // Check constructor parameter arity constraints across inheritance chain
    for (size_t i = 0; i + 1 < info.ctors.size(); i++) {
        const MethodInfo &ctor = info.ctors[i];
        if (ctor.info && ctor.info->param_count > 0) {
            std::string parent_name = ctor.owner ? ctor.owner->name : "a spacesuit";
            error(ErrorCode::E0307_ArityMismatch, info.suit->super_span,
                  info.name + " inherits from " + parent_name +
                  ", whose constructor takes " + std::to_string(ctor.info->param_count) +
                  " argument(s), but only the most derived constructor can accept arguments");
        }
    }
}

void Resolver::resolve_suit_bodies(SpacesuitInfo &info)
{
    // Resolve constructor bodies
    for (MethodInfo &ctor : info.ctors) {
        if (ctor.owner == &info && ctor.info && ctor.info->capsule) {
            suit_ = &info;
            field_limit_ = info.fields.size();
            resolve_capsule(kNullNode, *ctor.info->capsule, *ctor.info);
            suit_ = nullptr;
        }
    }

    // Resolve regular method bodies
    for (auto &pair : info.methods) {
        MethodInfo &method = pair.second;
        if (method.owner == &info && method.info && method.info->capsule) {
            suit_ = &info;
            field_limit_ = info.fields.size();
            resolve_capsule(kNullNode, *method.info->capsule, *method.info);
            suit_ = nullptr;
        }
    }

    // Resolve field initializers with progressive visibility
    const size_t inherited_count = info.super ? info.super->fields.size() : 0;
    for (size_t i = inherited_count; i < info.fields.size(); i++) {
        if (info.fields[i].init == kNullNode)
            continue;
        info_ = nullptr;
        scopes_.clear();
        suit_ = &info;
        field_limit_ = i; // Only earlier fields are visible in initializer
        resolve_expr(info.fields[i].init);
        suit_ = nullptr;
        field_limit_ = 0;
    }
}

void Resolver::resolve_suit(SpacesuitInfo &info)
{
    if (!laid_out_.insert(&info).second)
        return;

    if (info.super) {
        auto it = out_.suits.find(info.suit->super);
        if (it != out_.suits.end())
            resolve_suit(it->second);
    }

    build_layout(info);
}

} // namespace satellite

