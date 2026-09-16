// Name and call resolution, arity checking, literal option folding,
// and special arguments introspection validation.
// Part of Milestone 6 Resolver Prototype in prototype/M6.

#include "resolver.hpp"
#include "suggester.hpp"

namespace satellite {

namespace {

bool is_help_singleton_target(const AstArena &arena, NodeIndex target)
{
    if (target == kNullNode)
        return false;
    const auto &node = arena.get(target);
    if (node.kind != NodeKind::Member)
        return false;
    const auto &member = std::get<MemberExpr>(node.data);
    if (member.name != "help" || member.target == kNullNode)
        return false;
    const auto &base = arena.get(member.target);
    return base.kind == NodeKind::SatelliteLit;
}

} // namespace

void Resolver::resolve_name(NodeIndex expr_idx, const NameExpr &name, Span span)
{
    // 1. Local scope lookup (parameters & locals)
    if (const int32_t *slot = lookup(name.text)) {
        out_.table.set_slot(expr_idx, *slot);
        return;
    }

    // 2. Enclosing spacesuit field lookup
    if (int field_idx = lookup_field(name.text); field_idx >= 0) {
        out_.table.set_field_index(expr_idx, static_cast<uint32_t>(field_idx));
        return;
    }

    // 3. Enclosing spacesuit method lookup
    if (suit_ && suit_->find_method(name.text)) {
        out_.table.set_slot(expr_idx, SLOT_METHOD);
        return;
    }

    // 4. Capsule lookup
    if (out_.capsules.count(name.text)) {
        out_.table.set_slot(expr_idx, SLOT_CAPSULE);
        return;
    }

    // 5. Spacesuit / constructor lookup
    if (out_.suits.count(name.text)) {
        out_.table.set_slot(expr_idx, SLOT_SUIT);
        return;
    }

    // 6. Top-level statements remain SLOT_GLOBAL
    if (!info_ && !suit_) {
        out_.table.set_slot(expr_idx, SLOT_GLOBAL);
        return;
    }

    // 7. Global booleans fallback (DESIGN §8.4)
    if (name.text == "TRUE" || name.text == "FALSE") {
        out_.table.set_slot(expr_idx, SLOT_GLOBAL);
        return;
    }

    // 8. Help topic suppression in satellite.help(topic)
    if (help_topic_) {
        out_.table.set_slot(expr_idx, SLOT_GLOBAL);
        return;
    }

    // 9. Semantic Error: Unknown / undefined variable in capsule or spacesuit
    std::string context = suit_ ? ("spacesuit " + suit_->name) : "capsule";
    std::string msg = "unknown variable in " + context + ": " + name.text;

    // Collect candidate names in scope for suggestion
    std::vector<std::string> candidates;
    for (const auto &sc : scopes_) {
        for (const auto &p : sc)
            candidates.push_back(p.first);
    }
    if (suit_) {
        for (const auto &f : suit_->fields)
            candidates.push_back(f.name);
        for (const auto &m : suit_->methods)
            candidates.push_back(m.first);
    }
    for (const auto &c : out_.capsules)
        candidates.push_back(c.first);

    auto opt_sug = suggest_candidate(name.text, candidates);
    std::string suggestion = opt_sug.has_value() ? opt_sug.value() : "";
    error_with_suggestion(ErrorCode::E0301_UndefinedVariable, span, msg, suggestion);
    out_.table.set_slot(expr_idx, SLOT_GLOBAL);
}

bool Resolver::resolve_option_fold(NodeIndex expr_idx, NodeIndex target_idx,
                                   const std::vector<NodeIndex> &args, Span span)
{
    (void)span;
    if (target_idx == kNullNode)
        return false;
    const auto &target_node = arena_.get(target_idx);
    if (target_node.kind != NodeKind::Member)
        return false;
    const auto &member = std::get<MemberExpr>(target_node.data);

    // WORD_NUMBERS §1.5 & §2.2: List sorting literal option folds
    if (member.name == "sort") {
        if (args.empty()) {
            // sort() default
            out_.table.set_folded_path(expr_idx, static_cast<words::PathId>(words::NodeId::CONTAINER_LIST_SORT_0));
            return true;
        }
        if (args.size() >= 1) {
            const auto &arg0 = arena_.get(args[0]);
            if (arg0.kind == NodeKind::StringLit) {
                const auto &slit = std::get<StringLit>(arg0.data);
                if (slit.text == "down" || slit.text == "\"down\"") {
                    if (args.size() == 1) {
                        out_.table.set_folded_path(expr_idx, static_cast<words::PathId>(words::NodeId::CONTAINER_LIST_SORT_DOWN_0)); // 1 4 2 5
                        return true;
                    } else if (args.size() == 2) {
                        out_.table.set_folded_path(expr_idx, static_cast<words::PathId>(words::NodeId::CONTAINER_LIST_SORT_DOWN_KEY)); // 1 4 2 6
                        return true;
                    }
                } else if (slit.text == "up" || slit.text == "\"up\"") {
                    if (args.size() == 1) {
                        out_.table.set_folded_path(expr_idx, static_cast<words::PathId>(words::NodeId::CONTAINER_LIST_SORT_0)); // 1 4 2 3
                        return true;
                    } else if (args.size() == 2) {
                        out_.table.set_folded_path(expr_idx, static_cast<words::PathId>(words::NodeId::CONTAINER_LIST_SORT_UP)); // 1 4 2 7
                        return true;
                    }
                } else {
                    error_with_suggestion(ErrorCode::E0405_InvalidArgument, arg0.span,
                                          "invalid sort direction: " + slit.text,
                                          "did you mean \"down\" or \"up\"?");
                    return true;
                }
            } else {
                // Non-literal option: generic sort(direction)
                out_.table.set_folded_path(expr_idx, static_cast<words::PathId>(words::NodeId::CONTAINER_LIST_SORT_DIRECTION)); // 1 4 2 4
                return true;
            }
        }
    }

    return false;
}

void Resolver::resolve_call(NodeIndex expr_idx, const CallExpr &call, Span span)
{
    // Try literal option folding first
    resolve_option_fold(expr_idx, call.target, call.args, span);

    if (call.target == kNullNode) {
        for (NodeIndex arg : call.args)
            resolve_expr(arg);
        return;
    }

    const auto &target_node = arena_.get(call.target);

    if (target_node.kind == NodeKind::Name) {
        const auto &callee_name = std::get<NameExpr>(target_node.data);

        // If local shadows the name, it is a dynamic call
        if (!lookup(callee_name.text)) {
            auto check_arity = [&](const std::string &what, size_t expected) {
                if (call.args.size() != expected) {
                    std::string msg = what + " takes " + std::to_string(expected) +
                                      (expected == 1 ? " argument, got " : " arguments, got ") +
                                      std::to_string(call.args.size());
                    error(ErrorCode::E0307_ArityMismatch, span, msg);
                }
                out_.table.get_or_create(expr_idx).callee_param_count = static_cast<uint32_t>(expected);
            };

            // Spacesuit method
            if (suit_) {
                if (const MethodInfo *method = suit_->find_method(callee_name.text)) {
                    out_.table.set_slot(call.target, SLOT_METHOD);
                    check_arity(callee_name.text, method->info->param_count);
                    for (NodeIndex arg : call.args)
                        resolve_expr(arg);
                    return;
                }
            }

            // Top-level capsule
            auto it = out_.capsules.find(callee_name.text);
            if (it != out_.capsules.end()) {
                out_.table.set_slot(call.target, SLOT_CAPSULE);
                check_arity(callee_name.text, it->second.param_count);
                for (NodeIndex arg : call.args)
                    resolve_expr(arg);
                return;
            }

            // Spacesuit constructor call: MySuit(...)
            if (const SpacesuitInfo *suit_info = out_.find_suit(callee_name.text)) {
                out_.table.set_slot(call.target, SLOT_SUIT);
                check_arity(callee_name.text, suit_info->ctor_params());
                for (NodeIndex arg : call.args)
                    resolve_expr(arg);
                return;
            }
        }
    }

    // Special case for satellite.help(topic)
    if (call.args.size() == 1 && is_help_singleton_target(arena_, call.target)) {
        resolve_expr(call.target);
        bool outer_help = help_topic_;
        help_topic_ = true;
        resolve_expr(call.args[0]);
        help_topic_ = outer_help;
        return;
    }

    resolve_expr(call.target);
    for (NodeIndex arg : call.args)
        resolve_expr(arg);
}

bool Resolver::check_special_arguments_member(NodeIndex expr_idx, const std::string &property, Span span)
{
    // DESIGN §7.7: Valid introspection properties of the special `arguments` object
    static const char *kValidProps[] = {
        "username", "memory", "total", "machine", "cpu", "cores", "threads"
    };
    bool valid = false;
    for (const char *cand : kValidProps) {
        if (property == cand) {
            valid = true;
            break;
        }
    }

    if (!valid) {
        std::vector<std::string> valid_vec;
        for (const char *cand : kValidProps)
            valid_vec.push_back(cand);
        auto opt_sug = suggest_candidate(property, valid_vec);
        std::string sug = opt_sug.has_value() ? opt_sug.value() : "";
        error_with_suggestion(ErrorCode::E0304_UndefinedField, span,
                              "unknown property on arguments: " + property, sug);
        return false;
    }

    out_.table.get_or_create(expr_idx).is_arguments_special = true;
    return true;
}

void Resolver::resolve_member(NodeIndex expr_idx, const MemberExpr &member, Span span)
{
    if (member.target != kNullNode) {
        resolve_expr(member.target);

        // Check if accessing properties on special arguments
        const auto &target_info = out_.table.get(member.target);
        if (target_info.is_arguments_special) {
            check_special_arguments_member(expr_idx, member.name, span);
        }
    }
}

} // namespace satellite
