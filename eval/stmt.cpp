// Statements and control flow.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

namespace satellite {

Flow Evaluator::exec_block(const Block &block)
{
    for (const StmtPtr &stmt : block.statements) {
        if (failed())
            return Flow::Normal;
        if (!stmt)
            continue;
        if (exec(*stmt) == Flow::Return)
            return Flow::Return;
    }
    return Flow::Normal;
}

bool Evaluator::condition(const Expr &expr, bool &out)
{
    ValuePtr v = eval(expr);
    if (failed() || !v)
        return false;
    const bool *b = std::get_if<bool>(v.get());
    if (!b) {
        // No implicit truthiness. Every value in satellite has a declared
        // type, so "is 0 false?" is a question the language never has to
        // answer — and every language that did answer it regrets the answer.
        fail(expr.span, "condition must be a satellite.variable.bool, got " +
                        std::string(module_of(*v) ? module_of(*v) : "nil"));
        return false;
    }
    out = *b;
    return true;
}

Flow Evaluator::exec(const Stmt &stmt)
{
    if (depth_ >= max_depth_) {
        fail(stmt.span, "satellite stack overflow (satellite.library.system."
                        "max_depth = " + std::to_string(max_depth_) + ")");
        return Flow::Normal;
    }
    DepthGuard guard(depth_);

    if (const VarDecl *decl = std::get_if<VarDecl>(&stmt)) {
        // `satellite` is the one reserved word, and the binding site is where
        // that is enforced (§1).
        if (decl->name == "satellite") {
            fail(stmt.span, "satellite is reserved and cannot be declared");
            return Flow::Normal;
        }

        Value value = default_of(decl->type);
        if (decl->init) {
            ValuePtr init = eval(*decl->init);
            if (failed() || !init)
                return Flow::Normal;
            value = *init;
            if (!matches(decl->type, value)) {
                fail(decl->init->span,
                     "cannot initialise " + unparse(decl->type) + " " +
                     decl->name + " with " + to_string(value));
                return Flow::Normal;
            }
        } else if (decl->type.is_spacesuit()) {
            // `my_class_name my_object` builds one. This is the construction
            // site the language has instead of a constructor call, which is
            // what lets the declaration read exactly like every other one.
            const SpacesuitInfo *suit = resolved_.find_suit(decl->type.name);
            if (!suit) {
                // resolve() rejects an unknown spacesuit before anything runs.
                fail(stmt.span, "internal: no such spacesuit: " +
                                decl->type.name);
                return Flow::Normal;
            }
            ValuePtr made = construct(*suit, {}, stmt.span);
            if (failed() || !made)
                return Flow::Normal;
            value = *made;
        }

        // A capsule local declares into its frame; a top-level variable
        // declares into satellite.library, where the declared type has to be
        // remembered alongside it. A frame slot needs no such record: its type
        // is in the capsule, one copy for every activation.
        if (decl->slot >= 0) {
            Slot slot{decl->slot, std::string(), decl->name, true};
            write_slot(slot, make_value(std::move(value)), stmt.span);
            return Flow::Normal;
        }

        Slot slot{SLOT_GLOBAL, ns_, decl->name, true};
        declared_[slot.key()] = decl->type;
        Library::instance().set(slot.ns, slot.name, std::move(value));
        return Flow::Normal;
    }

    if (const Assign *assign = std::get_if<Assign>(&stmt)) {
        if (!assign->target || !assign->value)
            return Flow::Normal;

        Slot slot = slot_of(*assign->target);
        if (!slot.valid) {
            fail(assign->target->span, "cannot assign to this expression");
            return Flow::Normal;
        }

        ValuePtr value = eval(*assign->value);
        if (failed() || !value)
            return Flow::Normal;

        const Type *declared = declared_type(slot);
        if (declared && !matches(*declared, *value)) {
            fail(assign->value->span,
                 "cannot assign " + to_string(*value) + " to " +
                 unparse(*declared) + " " + slot.name);
            return Flow::Normal;
        }

        write_slot(slot, std::move(value), assign->target->span);
        return Flow::Normal;
    }

    if (const ExprStmt *e = std::get_if<ExprStmt>(&stmt)) {
        if (e->expr)
            eval(*e->expr);
        return Flow::Normal;
    }

    if (const Return *ret = std::get_if<Return>(&stmt)) {
        if (ret->value) {
            ValuePtr v = eval(*ret->value);
            if (failed())
                return Flow::Normal;
            returned_ = v;
        } else {
            returned_ = make_value(std::monostate{});
        }
        return Flow::Return;
    }

    if (const Block *block = std::get_if<Block>(&stmt))
        return exec_block(*block);

    if (const If *node = std::get_if<If>(&stmt)) {
        bool test = false;
        if (!node->condition || !condition(*node->condition, test))
            return Flow::Normal;
        if (test)
            return node->then_branch ? exec(*node->then_branch) : Flow::Normal;
        return node->else_branch ? exec(*node->else_branch) : Flow::Normal;
    }

    if (const While *node = std::get_if<While>(&stmt)) {
        for (;;) {
            bool test = false;
            if (!node->condition || !condition(*node->condition, test))
                return Flow::Normal;
            if (!test)
                return Flow::Normal;
            if (node->body && exec(*node->body) == Flow::Return)
                return Flow::Return;
            if (failed())
                return Flow::Normal;
        }
    }

    if (const For *node = std::get_if<For>(&stmt)) {
        if (node->init && exec(*node->init) == Flow::Return)
            return Flow::Return;
        for (;;) {
            if (failed())
                return Flow::Normal;
            // A missing condition means "always true", so for(;;) loops.
            bool test = true;
            if (node->condition && !condition(*node->condition, test))
                return Flow::Normal;
            if (!test)
                return Flow::Normal;
            if (node->body && exec(*node->body) == Flow::Return)
                return Flow::Return;
            if (failed())
                return Flow::Normal;
            if (node->step && exec(*node->step) == Flow::Return)
                return Flow::Return;
        }
    }

    fail(stmt.span, "unhandled statement");
    return Flow::Normal;
}

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------

} // namespace satellite
