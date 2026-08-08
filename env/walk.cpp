// The recursive walk over expressions, statements and capsule bodies.
//
// Part of env/, split from an 854-line env.cpp.

#include "env_internal.hpp"

namespace satellite {

void Resolver::resolve_expr(const Expr &expr)
{
    if (depth_ >= MAX_RESOLVE_DEPTH) {
        fail(expr.span, "expression nests deeper than the resolver can walk (" +
                        std::to_string(MAX_RESOLVE_DEPTH) + ")");
        return;
    }
    DepthGuard guard(depth_);

    if (const Name *name = std::get_if<Name>(&expr)) {
        resolve_name(*name, expr.span);
        return;
    }
    if (const Call *call = std::get_if<Call>(&expr)) {
        resolve_call(*call, expr.span);
        return;
    }
    if (const Member *member = std::get_if<Member>(&expr)) {
        // Only the receiver can contain a name; the member itself is a
        // selector, resolved against the receiver's value at run time.
        if (member->target)
            resolve_expr(*member->target);
        return;
    }
    if (const Index *index = std::get_if<Index>(&expr)) {
        if (index->target)
            resolve_expr(*index->target);
        if (index->subscript)
            resolve_expr(*index->subscript);
        return;
    }
    if (const Slice *slice = std::get_if<Slice>(&expr)) {
        if (slice->target)
            resolve_expr(*slice->target);
        if (slice->lo)
            resolve_expr(*slice->lo);
        if (slice->hi)
            resolve_expr(*slice->hi);
        return;
    }
    if (const Unary *unary = std::get_if<Unary>(&expr)) {
        if (unary->operand)
            resolve_expr(*unary->operand);
        return;
    }
    if (const Binary *binary = std::get_if<Binary>(&expr)) {
        if (binary->left)
            resolve_expr(*binary->left);
        if (binary->right)
            resolve_expr(*binary->right);
        return;
    }

    // NumberLit, StringLit and SatelliteLit hold no names.
}

void Resolver::resolve_stmt(const Stmt &stmt)
{
    if (depth_ >= MAX_RESOLVE_DEPTH) {
        fail(stmt.span, "statements nest deeper than the resolver can walk (" +
                        std::to_string(MAX_RESOLVE_DEPTH) + ")");
        return;
    }
    DepthGuard guard(depth_);

    if (const VarDecl *decl = std::get_if<VarDecl>(&stmt)) {
        // The initialiser is resolved BEFORE the name is declared, so
        // `satellite.variable.number x = x` is an unknown-variable error
        // rather than a read of the uninitialised slot it is writing.
        check_type(decl->type, stmt.span);

        // `my_suit x` with no arguments still constructs, so it still has to
        // satisfy the constructor. Checked here rather than at the construction
        // site, because the bare declaration has no Call node for resolve_call
        // to have checked already.
        if (!decl->init && decl->type.is_spacesuit()) {
            const SpacesuitInfo *suit = out_.find_suit(decl->type.name);
            if (suit && suit->ctor_params() != 0)
                fail(stmt.span, decl->type.name + "'s constructor takes " +
                                std::to_string(suit->ctor_params()) +
                                (suit->ctor_params() == 1 ? " argument"
                                                          : " arguments") +
                                ", so " + decl->name +
                                " has to be declared with them: " +
                                decl->type.name + " " + decl->name + "(...)");
        }

        if (decl->init)
            resolve_expr(*decl->init);
        decl->slot = declare(decl->name, decl->type, stmt.span);
        return;
    }

    if (const Assign *assign = std::get_if<Assign>(&stmt)) {
        if (assign->value)
            resolve_expr(*assign->value);
        if (assign->target)
            resolve_expr(*assign->target);
        return;
    }

    if (const ExprStmt *expr = std::get_if<ExprStmt>(&stmt)) {
        if (expr->expr)
            resolve_expr(*expr->expr);
        return;
    }

    if (const Return *ret = std::get_if<Return>(&stmt)) {
        if (ret->value)
            resolve_expr(*ret->value);
        return;
    }

    if (const Block *block = std::get_if<Block>(&stmt)) {
        scopes_.emplace_back();
        for (const StmtPtr &inner : block->statements)
            if (inner)
                resolve_stmt(*inner);
        scopes_.pop_back();
        return;
    }

    if (const If *node = std::get_if<If>(&stmt)) {
        if (node->condition)
            resolve_expr(*node->condition);
        if (node->then_branch)
            resolve_stmt(*node->then_branch);
        if (node->else_branch)
            resolve_stmt(*node->else_branch);
        return;
    }

    if (const While *node = std::get_if<While>(&stmt)) {
        if (node->condition)
            resolve_expr(*node->condition);
        if (node->body)
            resolve_stmt(*node->body);
        return;
    }

    if (const For *node = std::get_if<For>(&stmt)) {
        // The loop gets its own scope so that a declaration in the init clause
        // belongs to the loop and does not leak past it.
        scopes_.emplace_back();
        if (node->init)
            resolve_stmt(*node->init);
        if (node->condition)
            resolve_expr(*node->condition);
        if (node->step)
            resolve_stmt(*node->step);
        if (node->body)
            resolve_stmt(*node->body);
        scopes_.pop_back();
        return;
    }
}

void Resolver::resolve_capsule(const Capsule &capsule, CapsuleInfo &info)
{
    info_ = &info;
    scopes_.clear();
    scopes_.emplace_back();

    for (const Param &param : capsule.params) {
        check_type(param.type, param.span);
        // A slot is allocated for EVERY parameter, including one that fails to
        // bind, so slot positions keep matching argument positions. Dropping a
        // slot here would silently shift every later parameter by one.
        int slot = static_cast<int>(info.slot_count++);
        info.slot_types.push_back(param.type);
        info.slot_names.push_back(param.name);

        if (param.name == "satellite") {
            fail(param.span, "satellite is reserved and cannot name a parameter");
            continue;
        }
        if (!scopes_.back().emplace(param.name, slot).second)
            fail(param.span, "duplicate parameter " + param.name);
    }

    if (capsule.body)
        resolve_stmt(*capsule.body);

    info_ = nullptr;
    scopes_.clear();
}

} // namespace satellite
