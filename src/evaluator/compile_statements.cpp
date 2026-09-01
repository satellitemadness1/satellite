// Compiling DESIGN §6's statements. See compile_expressions.cpp for the other
// half and evaluator_internal.hpp for the walk.
//
// EVERY CASE ANSWERS AN OP THAT LEAVES NO VALUE, which is the statement half of
// the contract operations_control.cpp keeps at the other end.
//
// THE OPTIONAL CHILDREN ARE THE WHOLE SUBTLETY. A `for` has three of them and
// an `if` has one, so a case cannot take a fixed number of results back off the
// stack -- it has to take back exactly what it visited. Each one below counts
// as it visits and takes the same count, which is why the counting is written
// out rather than folded into a helper: a helper would have to be told the
// count anyway, and the bug this shape prevents is the one where the two
// numbers are computed in two places.

#include "evaluator/evaluator_internal.hpp"

#include <string>
#include <utility>

namespace satellite {
namespace eval {

bool Compiler::step_statement(NodeIndex node, uint32_t step_number)
{
    const Node &n = ast_[node];

    switch (n.kind) {
    case NodeKind::VarDecl: {
        // `b` IS THE INITIALISER AND `a` IS THE TYPE, which is ast.hpp's table
        // and is worth naming here because the two are both uint32_t and both
        // node indices, so nothing would catch the swap.
        if (n.b != kNoNode && step_number == 0) {
            again(1);
            visit(n.b);
            return true;
        }
        const OpIndex value = n.b == kNoNode ? kNoOp : take();
        const resolve::Info &about = info(node);
        if (!resolve::in_a_frame(about.slot)) {
            finish(not_built(node, "a declaration outside a capsule",
                             "DESIGN.md §7.2 reserves `satellite.library` for "
                             "shared state, and this grammar has no top-level "
                             "statement"));
            return true;
        }
        finish(emit(op_store, node, static_cast<uint32_t>(about.slot), value));
        return true;
    }

    case NodeKind::Assign: {
        if (step_number == 0) {
            again(1);
            visit(n.b);
            return true;
        }
        const OpIndex value = take();
        const resolve::Info &about = info(n.a);

        if (resolve::in_a_frame(about.slot)) {
            finish(emit(op_store, node, static_cast<uint32_t>(about.slot), value));
            return true;
        }

        const auto found = globals_.find(about.path);
        if (found != globals_.end()) {
            finish(emit(op_store_global, node, found->second, value));
            return true;
        }

        // ASSIGNING TO A SUBSCRIPT OR A FIELD. DESIGN §6.5 is the reason this
        // is a refusal with a milestone rather than a silent discard: "no
        // satellite code runs inside a mutation", and getting that rule right
        // is M16's work and M26's, not something to approximate here.
        finish(not_built(node, "assigning to this",
                         ast_[n.a].kind == NodeKind::Index ||
                                 ast_[n.a].kind == NodeKind::Slice
                             ? "PLAN.md §8 builds the containers at M16"
                             : "PLAN.md §8 builds the spacesuits at M26"));
        return true;
    }

    case NodeKind::ExprStmt:
        if (step_number == 0) {
            again(1);
            visit(n.a);
            return true;
        }
        finish(emit(op_expression, node, take()));
        return true;

    case NodeKind::Return:
        // THE THREE SHAPES ARE M10's AND THE MECHANISM IS THIS MILESTONE'S.
        // PLAN §8 gives `satellite.return()` `1 15 0`, `return(satellite)`
        // `1 15 1` and `return(value)` `1 15 2` to M10 as PATHS; what unwinds a
        // frame is the control stack, and that is here. The middle shape needs
        // the runtime singleton and comes back from step_expression as a
        // refusal naming M10, which is the boundary landing where it should.
        if (n.a != kNoNode && step_number == 0) {
            again(1);
            visit(n.a);
            return true;
        }
        finish(emit(op_return, node, n.a == kNoNode ? kNoOp : take()));
        return true;

    case NodeKind::Block:
        if (step_number == 0) {
            again(1);
            visit_reversed(n.a);
            return true;
        }
        finish(emit(op_block, node, out_.add_list(take_many(ast_.list_size(n.a)))));
        return true;

    case NodeKind::If:
        if (step_number == 0) {
            again(1);
            if (n.c != kNoNode)
                visit(n.c);
            visit(n.b);
            visit(n.a);
            return true;
        }
        {
            // TAKEN BACK IN REVERSE, because the children were visited so that
            // the FIRST one compiles first and a stack hands back what went on
            // last. The declarations below read bottom-up against the visits
            // above them, which is the one place in this file where source
            // order and code order disagree -- and it disagrees in exactly one
            // direction, everywhere, which is what makes it checkable.
            const OpIndex otherwise = n.c == kNoNode ? kNoOp : take();
            const OpIndex then = take();
            const OpIndex condition = take();
            finish(emit(op_if, node, condition, then, otherwise));
        }
        return true;

    case NodeKind::While:
        if (step_number == 0) {
            again(1);
            visit(n.b);
            visit(n.a);
            return true;
        }
        {
            const OpIndex body = take();
            const OpIndex condition = take();
            finish(emit(op_while, node, condition, body));
        }
        return true;

    case NodeKind::For:
        if (step_number == 0) {
            again(1);
            visit(n.d);
            if (n.c != kNoNode)
                visit(n.c);
            if (n.b != kNoNode)
                visit(n.b);
            if (n.a != kNoNode)
                visit(n.a);
            return true;
        }
        {
            const OpIndex body = take();
            const OpIndex stepping = n.c == kNoNode ? kNoOp : take();
            const OpIndex condition = n.b == kNoNode ? kNoOp : take();
            const OpIndex init = n.a == kNoNode ? kNoOp : take();
            finish(emit(op_for, node, init, condition, stepping, body));
        }
        return true;

    default:
        return false;
    }
}

} // namespace eval
} // namespace satellite
