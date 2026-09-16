// Scopes, frame slot allocation, type checking, and diagnostic emission.
// Part of Milestone 6 Resolver Prototype in prototype/M6.

#include "resolver.hpp"
#include "unparse.hpp"

#include <algorithm>

namespace satellite {

std::string capsule_key(const CapsuleDecl &capsule)
{
    return capsule.reserved ? "satellite." + capsule.name : capsule.name;
}

bool is_special_arguments_name(std::string_view name)
{
    // DESIGN §7.7 & WORD_NUMBERS §2.3: The six spellings of arguments
    return name == "arguments" || name == "argument"  ||
           name == "args"      || name == "arg"       ||
           name == "argumentz" || name == "argz";
}

const int32_t *Resolver::lookup(const std::string &name) const
{
    // Innermost scope first so an inner block shadows outer blocks
    for (auto scope = scopes_.rbegin(); scope != scopes_.rend(); ++scope) {
        auto found = scope->find(name);
        if (found != scope->end())
            return &found->second;
    }
    return nullptr;
}

int32_t Resolver::declare(const std::string &name, const Type &type, Span span, NodeIndex decl_node)
{
    // DESIGN §1: "satellite" is the reserved language root and cannot be declared
    if (name == "satellite") {
        error(ErrorCode::E0007_ReservedWordAsIdentifier, span,
              "satellite is reserved and cannot be declared");
        return SLOT_GLOBAL;
    }

    // Top-level declarations are globals (DESIGN §7.2, §10)
    if (!info_) {
        if (decl_node != kNullNode) {
            out_.table.set_slot(decl_node, SLOT_GLOBAL);
            out_.table.set_type(decl_node, type);
        }
        return SLOT_GLOBAL;
    }

    Scope &scope = scopes_.back();

    // DESIGN §7.4: Slots are never reused across scopes.
    // Redeclaring a variable in the same scope rebinds the name to a FRESH slot.
    int32_t slot = static_cast<int32_t>(info_->slot_count++);
    info_->slot_types.push_back(type);
    info_->slot_names.push_back(name);

    if (decl_node != kNullNode) {
        out_.table.set_slot(decl_node, slot);
        out_.table.set_type(decl_node, type);

        if (is_special_arguments_name(name)) {
            info_->has_special_arguments = true;
            out_.table.get_or_create(decl_node).is_arguments_special = true;
        }
    }

    auto found = scope.find(name);
    if (found != scope.end())
        found->second = slot;
    else
        scope.emplace(name, slot);

    return slot;
}

int Resolver::lookup_field(const std::string &name) const
{
    if (!suit_)
        return -1;
    const size_t limit = std::min(field_limit_, suit_->fields.size());
    for (size_t i = 0; i < limit; i++) {
        if (suit_->fields[i].name == name)
            return static_cast<int>(i);
    }
    return -1;
}

void Resolver::check_type(const Type &type, Span span)
{
    const Span at = (type.span.end > type.span.start) ? type.span : span;

    if (type.is_spacesuit() && !out_.suits.count(type.name)) {
        error(ErrorCode::E0303_UndefinedSpacesuit, at,
              "no such spacesuit: " + type.name);
    }

    if (type.space == "variable") {
        static const char *kKnownVariableTypes[] = {
            "bool", "number", "string", "time", "file",
            "binary", "hex", "hexadecimal", "thread", "variant",
            "capsule", "float", "network", "window", "date",
            "duration", "expression"
        };
        bool known = false;
        for (const char *cand : kKnownVariableTypes) {
            if (type.name == cand) {
                known = true;
                break;
            }
        }
        if (!known) {
            error(ErrorCode::E0106_ExpectedType, at,
                  "no such type: satellite.variable." + type.name);
        }

        if (!type.args.empty()) {
            error(ErrorCode::E0308_TypeMismatch, at,
                  "satellite.variable." + type.name +
                  " is not generic, so it takes no type arguments");
        }
    }

    if (type.space == "container") {
        if (type.name == "list") {
            if (type.args.size() > 1) {
                error(ErrorCode::E0308_TypeMismatch, at,
                      "satellite.container.list takes at most one type argument, got " +
                      std::to_string(type.args.size()));
            }
        } else if (type.name == "map") {
            if (!type.args.empty() && type.args.size() != 2) {
                error(ErrorCode::E0308_TypeMismatch, at,
                      "satellite.container.map takes two type arguments (key, value), got " +
                      std::to_string(type.args.size()));
            }
            // DESIGN §8.6: Map key must be string or number
            if (type.args.size() == 2) {
                const Type &key = type.args[0];
                if (!(key.space == "variable" &&
                      (key.name == "string" || key.name == "number"))) {
                    Span key_at = (key.span.end > key.span.start) ? key.span : at;
                    error(ErrorCode::E0308_TypeMismatch, key_at,
                          "a map key must be satellite.variable.string or satellite.variable.number");
                }
            }
        } else if (type.name == "result") {
            if (!type.args.empty()) {
                error(ErrorCode::E0308_TypeMismatch, at,
                      "satellite.container.result is not generic, so it takes no type arguments");
            }
        } else {
            error(ErrorCode::E0106_ExpectedType, at,
                  "no such container type: " + type.name);
        }
    }

    for (const Type &arg : type.args)
        check_type(arg, span);
}

void Resolver::error(ErrorCode code, Span span, std::string message)
{
    out_.diagnostics.push_back(Diagnostic::error(code, std::move(message), span));
}

void Resolver::error_with_note(ErrorCode code, Span span, std::string message,
                              Span note_span, std::string note_msg)
{
    Diagnostic diag = Diagnostic::error(code, std::move(message), span);
    diag.add_note(std::move(note_msg), note_span);
    out_.diagnostics.push_back(std::move(diag));
}

void Resolver::error_with_suggestion(ErrorCode code, Span span, std::string message,
                                     std::string suggestion)
{
    Diagnostic diag = Diagnostic::error(code, std::move(message), span);
    if (!suggestion.empty()) {
        diag.with_suggestion("did you mean '" + suggestion + "'?", span, suggestion);
    }
    out_.diagnostics.push_back(std::move(diag));
}

} // namespace satellite
