// AST walker over statements, expressions, and capsule bodies.
// Part of Milestone 6 Resolver Prototype in prototype/M6.

#include "resolver.hpp"

namespace satellite {

void Resolver::resolve_expr(NodeIndex expr_idx)
{
    if (expr_idx == kNullNode)
        return;

    if (depth_ >= MAX_RESOLVE_DEPTH) {
        error(ErrorCode::E0401_RecursionLimitExceeded, arena_.get(expr_idx).span,
              "expression nests deeper than the resolver limit (" +
              std::to_string(MAX_RESOLVE_DEPTH) + ")");
        return;
    }
    DepthGuard guard(depth_);

    const auto &node = arena_.get(expr_idx);

    switch (node.kind) {
    case NodeKind::Name:
        resolve_name(expr_idx, std::get<NameExpr>(node.data), node.span);
        break;

    case NodeKind::Call:
        resolve_call(expr_idx, std::get<CallExpr>(node.data), node.span);
        break;

    case NodeKind::Member:
        resolve_member(expr_idx, std::get<MemberExpr>(node.data), node.span);
        break;

    case NodeKind::Index: {
        const auto &idx_expr = std::get<IndexExpr>(node.data);
        if (idx_expr.target != kNullNode)
            resolve_expr(idx_expr.target);
        if (idx_expr.subscript != kNullNode)
            resolve_expr(idx_expr.subscript);
        break;
    }

    case NodeKind::Slice: {
        const auto &slice = std::get<SliceExpr>(node.data);
        if (slice.target != kNullNode)
            resolve_expr(slice.target);
        if (slice.lo != kNullNode)
            resolve_expr(slice.lo);
        if (slice.hi != kNullNode)
            resolve_expr(slice.hi);
        break;
    }

    case NodeKind::Unary: {
        const auto &unary = std::get<UnaryExpr>(node.data);
        if (unary.operand != kNullNode)
            resolve_expr(unary.operand);
        break;
    }

    case NodeKind::Binary: {
        const auto &bin = std::get<BinaryExpr>(node.data);
        if (bin.left != kNullNode)
            resolve_expr(bin.left);
        if (bin.right != kNullNode)
            resolve_expr(bin.right);
        break;
    }

    case NodeKind::ListLit: {
        const auto &list = std::get<ListLit>(node.data);
        for (NodeIndex elem : list.elements)
            resolve_expr(elem);
        break;
    }

    case NodeKind::NamedArg: {
        const auto &named = std::get<NamedArg>(node.data);
        if (named.value != kNullNode)
            resolve_expr(named.value);
        break;
    }

    case NodeKind::NumberLit:
    case NodeKind::DurationLit:
    case NodeKind::BitsLit:
    case NodeKind::StringLit:
    case NodeKind::SatelliteLit:
    default:
        break;
    }
}

void Resolver::resolve_stmt(NodeIndex stmt_idx)
{
    if (stmt_idx == kNullNode)
        return;

    if (depth_ >= MAX_RESOLVE_DEPTH) {
        error(ErrorCode::E0401_RecursionLimitExceeded, arena_.get(stmt_idx).span,
              "statements nest deeper than the resolver limit (" +
              std::to_string(MAX_RESOLVE_DEPTH) + ")");
        return;
    }
    DepthGuard guard(depth_);

    const auto &node = arena_.get(stmt_idx);

    switch (node.kind) {
    case NodeKind::VarDecl: {
        const auto &decl = std::get<VarDeclStmt>(node.data);
        check_type(decl.type, node.span);

        // If declaring an instance of a spacesuit without explicit args, check if constructor requires params
        if (decl.init == kNullNode && decl.type.is_spacesuit()) {
            if (const SpacesuitInfo *suit = out_.find_suit(decl.type.name)) {
                if (suit->ctor_params() != 0) {
                    error(ErrorCode::E0307_ArityMismatch, node.span,
                          decl.type.name + "'s constructor takes " +
                          std::to_string(suit->ctor_params()) +
                          (suit->ctor_params() == 1 ? " argument" : " arguments") +
                          ", so " + decl.name + " must be initialized with them: " +
                          decl.type.name + " " + decl.name + "(...)");
                }
            }
        }

        // Initializer is resolved BEFORE the variable enters scope
        // (so `satellite.variable.number x = x` errors on the RHS x)
        if (decl.init != kNullNode)
            resolve_expr(decl.init);

        declare(decl.name, decl.type, node.span, stmt_idx);
        break;
    }

    case NodeKind::Assign: {
        const auto &assign = std::get<AssignStmt>(node.data);
        if (assign.value != kNullNode)
            resolve_expr(assign.value);
        if (assign.target != kNullNode)
            resolve_expr(assign.target);
        break;
    }

    case NodeKind::ExprStmt: {
        const auto &expr_stmt = std::get<ExprStmt>(node.data);
        if (expr_stmt.expr != kNullNode)
            resolve_expr(expr_stmt.expr);
        break;
    }

    case NodeKind::Return: {
        const auto &ret = std::get<ReturnStmt>(node.data);
        if (ret.value != kNullNode)
            resolve_expr(ret.value);
        if (!info_) {
            error(ErrorCode::E0309_ReturnOutsideCapsule, node.span,
                  "satellite.return() cannot be used outside of a capsule");
        }
        break;
    }

    case NodeKind::Block: {
        const auto &blk = std::get<BlockStmt>(node.data);
        scopes_.emplace_back();
        for (NodeIndex inner : blk.statements)
            resolve_stmt(inner);
        scopes_.pop_back();
        break;
    }

    case NodeKind::If: {
        const auto &if_stmt = std::get<IfStmt>(node.data);
        if (if_stmt.condition != kNullNode)
            resolve_expr(if_stmt.condition);
        if (if_stmt.then_branch != kNullNode)
            resolve_stmt(if_stmt.then_branch);
        if (if_stmt.else_branch != kNullNode)
            resolve_stmt(if_stmt.else_branch);
        break;
    }

    case NodeKind::While: {
        const auto &while_stmt = std::get<WhileStmt>(node.data);
        if (while_stmt.condition != kNullNode)
            resolve_expr(while_stmt.condition);
        if (while_stmt.body != kNullNode)
            resolve_stmt(while_stmt.body);
        break;
    }

    case NodeKind::For: {
        const auto &for_stmt = std::get<ForStmt>(node.data);
        scopes_.emplace_back();
        if (for_stmt.init != kNullNode)
            resolve_stmt(for_stmt.init);
        if (for_stmt.condition != kNullNode)
            resolve_expr(for_stmt.condition);
        if (for_stmt.step != kNullNode)
            resolve_stmt(for_stmt.step);
        if (for_stmt.body != kNullNode)
            resolve_stmt(for_stmt.body);
        scopes_.pop_back();
        break;
    }

    default:
        break;
    }
}

void Resolver::resolve_capsule(NodeIndex node_idx, const CapsuleDecl &capsule, CapsuleInfo &info)
{
    info_ = &info;
    scopes_.clear();
    scopes_.emplace_back(); // Parameter scope

    if (node_idx != kNullNode)
        out_.table.set_slot(node_idx, SLOT_CAPSULE);

    if (capsule.returns)
        check_type(*capsule.returns, capsule.span);

    for (const ParamDecl &param : capsule.params) {
        check_type(param.type, param.span);

        int32_t slot = static_cast<int32_t>(info.slot_count++);
        info.slot_types.push_back(param.type);
        info.slot_names.push_back(param.name);

        if (param.name == "satellite") {
            error(ErrorCode::E0007_ReservedWordAsIdentifier, param.span,
                  "satellite is reserved and cannot name a parameter");
            continue;
        }

        if (is_special_arguments_name(param.name)) {
            info.has_special_arguments = true;
        }

        if (!scopes_.back().emplace(param.name, slot).second) {
            error(ErrorCode::E0205_DuplicateDeclaration, param.span,
                  "duplicate parameter: " + param.name);
        }
    }

    if (capsule.body != kNullNode)
        resolve_stmt(capsule.body);

    info_ = nullptr;
    scopes_.clear();
}

} // namespace satellite

