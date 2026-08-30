#pragma once

// The satellite Resolve Data Model & Types -- Milestone 6 Prototype.
//
// DESIGN §7 & PLAN §8: Static semantic analysis, scope management, and frame slot allocation.
//
// Key principles:
// 1. Pure AST Arena: All resolved data lives in a side table (ResolveTable)
//    indexed by uint32_t NodeIndex, with zero mutations to AstNode.
// 2. Thread isolation: Locals and parameters resolve to integer frame slots.
//    Top-level statements remain SLOT_GLOBAL.
// 3. No slot reuse across scopes (DESIGN §7.4): Rebinding creates a fresh slot
//    to preserve spacesuit reference semantics and diagnostic clarity.
// 4. Literal option fold (WORD_NUMBERS §1.5): Calls with literal string options
//    fold into specific PathIds at resolve time.
// 5. Special `arguments` variable (DESIGN §7.7): Recognises all six spellings.

#include "ast.hpp"
#include "diagnostic.hpp"
#include "satellite_words/words.hpp"
#include "satellite_words/words_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace satellite {

// ---------------------------------------------------------------------------
// Frame Slot Sentinels
// ---------------------------------------------------------------------------

inline constexpr int32_t SLOT_GLOBAL    = -1; // Top-level statement or global library path
inline constexpr int32_t SLOT_CAPSULE   = -2; // Refers to a top-level capsule
inline constexpr int32_t SLOT_METHOD    = -3; // Refers to a method of the current spacesuit
inline constexpr int32_t SLOT_SUIT      = -4; // Refers to a spacesuit type or constructor
inline constexpr int32_t SLOT_FIELD     = -5; // Refers to a field of the current spacesuit
inline constexpr int32_t SLOT_ARGUMENTS = -6; // Refers to the special §7.7 `arguments` object

inline bool is_local_slot(int32_t slot)
{
    return slot >= 0;
}

// ---------------------------------------------------------------------------
// Node Resolution Info (Stored in Side Table)
// ---------------------------------------------------------------------------

struct ResolveNodeInfo {
    int32_t slot = SLOT_GLOBAL;
    uint32_t field_index = 0;                  // When slot == SLOT_FIELD
    Type resolved_type;                        // Declared or resolved type
    words::PathId folded_path_id = words::kNoPath; // Resolved folded option path
    bool is_arguments_special = false;         // True for §7.7 special arguments parameter/variable
    uint32_t callee_param_count = 0;           // For Call nodes: expected parameter count
    Access member_access = Access::Protected;  // For member/field access checking
};

// ---------------------------------------------------------------------------
// Capsule & Function Info
// ---------------------------------------------------------------------------

struct CapsuleInfo {
    const CapsuleDecl *capsule = nullptr;
    NodeIndex node = kNullNode;
    std::string name;
    bool reserved = false;

    // Parameters occupy slots [0, param_count); locals occupy [param_count, slot_count).
    size_t param_count = 0;
    size_t slot_count = 0;

    // Parallel static array of declared slot types and diagnostic names
    std::vector<Type> slot_types;
    std::vector<std::string> slot_names;

    words::PathId path_id = words::kNoPath;
    bool has_special_arguments = false;
};

// ---------------------------------------------------------------------------
// Spacesuits, Fields & Methods Info
// ---------------------------------------------------------------------------

struct SpacesuitInfo;

struct FieldInfo {
    std::string name;
    Type type;
    Access access = Access::Protected;
    NodeIndex init = kNullNode;
    const SpacesuitInfo *owner = nullptr; // Suit that declared this field
    uint32_t index = 0;                  // Flat index in SpacesuitInfo::fields
    Span span;
};

struct MethodInfo {
    Access access = Access::Protected;
    bool constructor = false;
    const SpacesuitInfo *owner = nullptr;
    std::shared_ptr<CapsuleInfo> info;
    Span span;
};

struct SpacesuitInfo {
    const SpacesuitDecl *suit = nullptr;
    NodeIndex node = kNullNode;
    std::string name;
    const SpacesuitInfo *super = nullptr; // Null when there is no superclass
    Span span;

    // Superclass fields FIRST, then this suit's own
    std::vector<FieldInfo> fields;

    // Keyed by method name, with derived definitions overriding inherited ones
    std::unordered_map<std::string, MethodInfo> methods;

    // Ordered constructors: superclass first, derived last
    std::vector<MethodInfo> ctors;

    words::PathId path_id = words::kNoPath;

    bool is_a(const std::string &other) const
    {
        for (const SpacesuitInfo *walk = this; walk; walk = walk->super) {
            if (walk->name == other)
                return true;
        }
        return false;
    }

    const MethodInfo *find_method(const std::string &method_name) const
    {
        auto it = methods.find(method_name);
        return (it != methods.end()) ? &it->second : nullptr;
    }

    int find_field(const std::string &field_name) const
    {
        for (size_t i = 0; i < fields.size(); i++) {
            if (fields[i].name == field_name)
                return static_cast<int>(i);
        }
        return -1;
    }

    size_t ctor_params() const
    {
        return ctors.empty() || !ctors.back().info
                   ? 0
                   : ctors.back().info->param_count;
    }
};

// ---------------------------------------------------------------------------
// ResolveTable: Side Table Indexed by NodeIndex
// ---------------------------------------------------------------------------

class ResolveTable {
public:
    ResolveTable() = default;

    const ResolveNodeInfo &get(NodeIndex idx) const
    {
        static const ResolveNodeInfo kDefault;
        if (idx < nodes_.size())
            return nodes_[idx];
        return kDefault;
    }

    ResolveNodeInfo &get_or_create(NodeIndex idx)
    {
        if (idx >= nodes_.size())
            nodes_.resize(idx + 1);
        return nodes_[idx];
    }

    void set_slot(NodeIndex idx, int32_t slot)
    {
        get_or_create(idx).slot = slot;
    }

    int32_t get_slot(NodeIndex idx) const
    {
        return get(idx).slot;
    }

    void set_field_index(NodeIndex idx, uint32_t field_idx)
    {
        auto &info = get_or_create(idx);
        info.slot = SLOT_FIELD;
        info.field_index = field_idx;
    }

    uint32_t get_field_index(NodeIndex idx) const
    {
        return get(idx).field_index;
    }

    void set_folded_path(NodeIndex idx, words::PathId path_id)
    {
        get_or_create(idx).folded_path_id = path_id;
    }

    words::PathId get_folded_path(NodeIndex idx) const
    {
        return get(idx).folded_path_id;
    }

    void set_type(NodeIndex idx, Type t)
    {
        get_or_create(idx).resolved_type = std::move(t);
    }

    const Type *get_type(NodeIndex idx) const
    {
        if (idx < nodes_.size())
            return &nodes_[idx].resolved_type;
        return nullptr;
    }

    size_t size() const { return nodes_.size(); }
    void clear() { nodes_.clear(); }

private:
    std::vector<ResolveNodeInfo> nodes_;
};

// ---------------------------------------------------------------------------
// ResolveResult: Complete Output of the Resolution Phase
// ---------------------------------------------------------------------------

struct ResolveResult {
    std::unordered_map<std::string, CapsuleInfo> capsules;
    std::unordered_map<std::string, SpacesuitInfo> suits;
    ResolveTable table;
    std::vector<Diagnostic> diagnostics;

    bool ok() const { return diagnostics.empty(); }

    const CapsuleInfo *find_capsule(const std::string &name) const
    {
        auto it = capsules.find(name);
        return (it != capsules.end()) ? &it->second : nullptr;
    }

    const SpacesuitInfo *find_suit(const std::string &name) const
    {
        auto it = suits.find(name);
        return (it != suits.end()) ? &it->second : nullptr;
    }
};

} // namespace satellite

