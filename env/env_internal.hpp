#pragma once

// Private to env/. The public surface is ../env.hpp and it has not changed.
//
// env.cpp was 854 lines. Split along the passes the resolver already ran in
// order: scopes and slot allocation, name and call resolution, the tree walk,
// the spacesuit layout pass, and the driver plus the public API.
//
// The Resolver was in an anonymous namespace when this was one translation
// unit and needs external linkage now. §6's rule that resolve() must complete
// single-threaded before any evaluation of the same Program is unchanged by
// that -- it was never a statement about linkage.

#include "../env.hpp"

#include <algorithm>
#include <unordered_set>

namespace satellite {

// The resolver walks the tree recursively, so it needs the same kind of bound
// the evaluator does — and for the same reason: without one, a deeply nested
// expression segfaults the C++ stack instead of producing an error. Kept well
// under what an 8 MB stack holds for this frame size.
constexpr int MAX_RESOLVE_DEPTH = 2000;

struct DepthGuard {
    explicit DepthGuard(int &depth) : depth_(depth) { depth_++; }
    ~DepthGuard() { depth_--; }

    DepthGuard(const DepthGuard &) = delete;
    DepthGuard &operator=(const DepthGuard &) = delete;

private:
    int &depth_;
};

// The name a capsule is called by: bare for a user capsule, prefixed for a
// reserved one, exactly as §1 has it.
// Defined in scopes.cpp; run.cpp calls it too, so it cannot stay a definition
// in a header included by five translation units.
std::string capsule_key(const Capsule &capsule);

class Resolver {
public:
    explicit Resolver(ResolveResult &out) : out_(out) {}

    void run(const Program &program);

private:
    using Scope = std::unordered_map<std::string, int>;

    void collect(const Program &program);
    void collect_suits(const Program &program);
    void link_supers();
    void break_inheritance_cycles();
    void build_layout(SpacesuitInfo &info);
    void resolve_bodies(SpacesuitInfo &info);
    void resolve_suit(SpacesuitInfo &info);
    void resolve_capsule(const Capsule &capsule, CapsuleInfo &info);
    void resolve_stmt(const Stmt &stmt);
    void resolve_expr(const Expr &expr);
    void resolve_name(const Name &name, Span span);
    void resolve_call(const Call &call, Span span);
    void check_type(const Type &type, Span span);

    int declare(const std::string &name, const Type &type, Span span);
    const int *lookup(const std::string &name) const;
    int lookup_field(const std::string &name) const;

    void fail(Span span, std::string message)
    {
        out_.errors.push_back(ResolveError{std::move(message), span});
    }

    ResolveResult &out_;

    // Null at the top level, which is what switches the whole pass between
    // "allocate a frame slot" and "leave it to satellite.library".
    CapsuleInfo *info_ = nullptr;

    // The spacesuit whose member is being resolved, or null outside one. It is
    // a SECOND switch, independent of info_: a method body has both (a frame
    // and a suit), a field initialiser has only a suit, an ordinary capsule has
    // only a frame, and top-level code has neither.
    const SpacesuitInfo *suit_ = nullptr;

    // How many of suit_'s fields are in scope. Always every field inside a
    // method body; inside a field's own initialiser it is that field's index,
    // so `satellite.variable.number n = n` is an unknown-name error rather than
    // a read of the slot it is about to write — the same rule resolve_stmt
    // already applies to a local declaration.
    size_t field_limit_ = 0;

    std::vector<Scope> scopes_;
    std::unordered_set<const SpacesuitInfo *> laid_out_;

    // The members each suit declared ITSELF, kept between the layout pass and
    // the body pass. Inherited entries are copies and were already resolved in
    // the suit that declared them; resolving them again would stamp the same
    // AST twice, and once is the contract Name::slot documents.
    std::unordered_map<const SpacesuitInfo *, std::vector<const Method *>>
        own_methods_;

    int depth_ = 0;
};

} // namespace satellite
