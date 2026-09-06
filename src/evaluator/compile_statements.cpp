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

// A value crossing into a float-declared name converts on the way in --
// op_to_float, DESIGN §8.6's exact and always-successful direction -- and
// EVERY OTHER DECLARED TYPE STORES CHECKLESS, exactly as before. M12 §2.1's
// argument against a test on the hottest store in the machine stands: this
// is not a check, it is one extra op compiled only where the program wrote
// `satellite.variable.float`, and a non-numeric value stopping there is
// S0713 with a caret rather than a slot quietly holding the wrong arm.
// Declarations and both assignment arms share it so the three cannot drift.
OpIndex Compiler::into_declared(const resolve::Info &about, NodeIndex node,
                                OpIndex value)
{
    if (value == kNoOp)
        return value; // declared without an initialiser: nothing, any type
    if (about.type !=
        static_cast<words::PathId>(words::NodeId::VARIABLE_FLOAT))
        return value;
    return emit(op_to_float, node, value);
}

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
        finish(emit(op_store, node, static_cast<uint32_t>(about.slot),
                    into_declared(about, node, value)));
        return true;
    }

    case NodeKind::Assign: {
        // AN INDEX TARGET HAS A SUBSCRIPT TO COMPILE AND EVERY OTHER TARGET
        // HAS NOTHING, which is why the shape is decided once here rather
        // than asked twice below.
        const bool into_subscript = ast_[n.a].kind == NodeKind::Index;
        if (step_number == 0) {
            again(1);
            visit(n.b);
            // The subscript is written before the `=`, so it compiles first
            // and is therefore pushed last -- the Index case's inversion,
            // arriving here because an index assignment is one expression
            // split across a statement.
            if (into_subscript)
                visit(ast_[n.a].b);
            return true;
        }
        const OpIndex value = take();
        const OpIndex subscript = into_subscript ? take() : kNoOp;
        const resolve::Info &about = info(n.a);

        if (resolve::in_a_frame(about.slot)) {
            finish(emit(op_store, node, static_cast<uint32_t>(about.slot),
                        into_declared(about, node, value)));
            return true;
        }

        const auto found = globals_.find(about.path);
        if (found != globals_.end()) {
            finish(emit(op_store_global, node, found->second,
                        into_declared(about, node, value)));
            return true;
        }

        // ASSIGNING TO A NUMBERED LANGUAGE PATH IS THE RETUNE -- M15's, the
        // meaning M11 declined to invent mid-milestone (its §6 carries the
        // hand-off). Which paths accept one is the Assigners table's question
        // and is asked at RUN time, so the answer can widen without this arm
        // moving. The spelling handed along is the registry's, for the same
        // reason op_dispatch's is: S0724's sentence quotes the path as the
        // language writes it, and the file's own spelling is already under
        // the caret.
        if (about.path != words::kNoPath &&
            words::is_language_word(about.path) &&
            ast_[n.a].kind == NodeKind::Member) {
            finish(emit(op_retune, node, about.path, value,
                        out_.add_text(std::string(words::path_text(
                            static_cast<words::NodeId>(about.path))))));
            return true;
        }

        // ASSIGNING TO A SUBSCRIPT -- M16, and DESIGN §6.5 is what shapes it:
        // "no satellite code runs inside a mutation", so both operands are
        // fully reduced to values BEFORE the container is touched. The op
        // carries the receiver's SLOT, decided here exactly as a mutating
        // method's is, because an index assignment writes the whole container
        // back and a temporary names nowhere to write it -- `foo()[0] = 1` is
        // refused for the same reason `foo().append(x)` is.
        //
        // ONE LEVEL, AND SAYING SO IS THE WORK. `l[0][1] = x` has an Index as
        // its own target, which names no slot, so it lands in the refusal
        // below with a sentence about naming the inner container first. That
        // is the same one-hop boundary names.cpp draws for selectors, and it
        // moves when a later milestone gives assignment a general place
        // expression rather than by being approximated here.
        if (into_subscript) {
            const resolve::Info &holder = info(ast_[n.a].a);
            if (resolve::in_a_frame(holder.slot)) {
                finish(emit(op_index_store, node,
                            static_cast<uint32_t>(holder.slot), subscript,
                            value));
                return true;
            }
            if (const auto place = globals_.find(holder.path);
                place != globals_.end()) {
                finish(emit(op_index_store_global, node, place->second,
                            subscript, value));
                return true;
            }
            finish(not_built(node, "assigning to this subscript",
                             "an index assignment writes the whole container "
                             "back, so what is indexed has to be a name -- "
                             "DESIGN.md §6.4's storage-slot rule"));
            return true;
        }

        finish(not_built(node, "assigning to this",
                         ast_[n.a].kind == NodeKind::Slice
                             ? "a slice names a run of elements and not one "
                               "place, so there is nothing for a single value "
                               "to be written into"
                             : "PLAN.md §8 builds the spacesuits at M26"));
        return true;
    }

    case NodeKind::ExprStmt:
        if (step_number == 0) {
            // THE ONE PLACE A CALL LEARNS IT IS A STATEMENT. M14's
            // `input(prompt, target)` `1 5 4` is legal HERE and nowhere else
            // -- "writes a place and yields nothing", so a position that
            // could read its result is refused (S1003) -- and the compiler's
            // call() answers that question by comparing its node against
            // this marker. Set per expression-statement and read only while
            // this statement's subtree is being compiled, so nesting cannot
            // confuse it: a call INSIDE the root expression is not the root.
            statement_root_ = n.a;
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
