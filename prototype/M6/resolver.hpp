#pragma once

// The satellite Resolver Interface -- Milestone 6 Prototype.
//
// DESIGN §7 & PLAN §8: 4-pass semantic analysis, frame slot allocation,
// spacesuit inheritance resolution, literal option folding, and M5 diagnostics.

#include "resolve_types.hpp"
#include "diagnostic.hpp"
#include "diagnostic_code.hpp"
#include "suggester.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace satellite {

// Bounded recursion depth limit (DESIGN §7.5)
inline constexpr int MAX_RESOLVE_DEPTH = 2000;

struct DepthGuard {
    explicit DepthGuard(int &depth) : depth_(depth) { depth_++; }
    ~DepthGuard() { depth_--; }

    DepthGuard(const DepthGuard &) = delete;
    DepthGuard &operator=(const DepthGuard &) = delete;

private:
    int &depth_;
};

// Returns the lookup key for a capsule: bare name or "satellite." + name for reserved
std::string capsule_key(const CapsuleDecl &capsule);

// Checks if a name matches any of the 6 allowed spellings of the special arguments variable
bool is_special_arguments_name(std::string_view name);

class Resolver {
public:
    Resolver(const Program &program, AstArena &arena, words::Words &words, ResolveResult &out);

    void run(const ResolveResult *inherited = nullptr);

private:
    using Scope = std::unordered_map<std::string, int32_t>;

    // Pass 1: Collect capsule declarations
    void collect_capsules();

    // Pass 2: Collect spacesuit declarations, link inheritance, flatten layouts, resolve suit bodies
    void collect_suits();
    void link_supers();
    void break_inheritance_cycles();
    void build_layout(SpacesuitInfo &info);
    void resolve_suit_bodies(SpacesuitInfo &info);
    void resolve_suit(SpacesuitInfo &info);

    // Pass 3: Top-level statements
    void resolve_top_level();

    // Pass 4: Capsule bodies
    void resolve_all_capsules();

    // Walkers for AST nodes
    void resolve_capsule(NodeIndex node_idx, const CapsuleDecl &capsule, CapsuleInfo &info);
    void resolve_stmt(NodeIndex stmt_idx);
    void resolve_expr(NodeIndex expr_idx);

    // Semantic resolution
    void resolve_name(NodeIndex expr_idx, const NameExpr &name, Span span);
    void resolve_call(NodeIndex expr_idx, const CallExpr &call, Span span);
    void resolve_member(NodeIndex expr_idx, const MemberExpr &member, Span span);

    // Option folding & Special arguments
    bool resolve_option_fold(NodeIndex expr_idx, NodeIndex target_idx, const std::vector<NodeIndex> &args, Span span);
    bool check_special_arguments_member(NodeIndex expr_idx, const std::string &property, Span span);

    // Type checking & validation
    void check_type(const Type &type, Span span);

    // Scopes & Slots
    int32_t declare(const std::string &name, const Type &type, Span span, NodeIndex decl_node);
    const int32_t *lookup(const std::string &name) const;
    int lookup_field(const std::string &name) const;

    // Diagnostic reporting helpers
    void error(ErrorCode code, Span span, std::string message);
    void error_with_note(ErrorCode code, Span span, std::string message,
                         Span note_span, std::string note_msg);
    void error_with_suggestion(ErrorCode code, Span span, std::string message,
                               std::string suggestion);

    const Program &program_;
    AstArena &arena_;
    words::Words &words_;
    ResolveResult &out_;

    CapsuleInfo *info_ = nullptr;        // Current capsule activation info, or null at top-level
    const SpacesuitInfo *suit_ = nullptr; // Current enclosing spacesuit, or null
    size_t field_limit_ = 0;             // Number of fields currently in scope
    std::vector<Scope> scopes_;
    std::unordered_set<const SpacesuitInfo *> laid_out_;
    std::unordered_map<const SpacesuitInfo *, std::vector<NodeIndex>> own_methods_;

    bool help_topic_ = false;            // True inside satellite.help(...) argument
    int depth_ = 0;                      // Recursion depth counter
};

// Primary entry point for semantic resolution
ResolveResult resolve(const Program &program, AstArena &arena, words::Words &words,
                      const ResolveResult *inherited = nullptr);

} // namespace satellite

