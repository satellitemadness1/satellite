// The walk over statements and expressions, and one capsule's frame being
// filled -- pass 4 of DESIGN §7.3. See name_resolver/resolve_internal.hpp for
// the split and for the actions this file's stack is made of.
//
// THE WALK KEEPS ITS OWN STACK ON THE HEAP, WHICH IS DESIGN §7.5's RULE. M7
// shipped this file as a recursive expression()/statement() pair with a fixed
// bound over it; the bound went on 2026-08-31 and the recursion goes here, at
// M8.5. What replaces it is one `std::vector<Work>`: the depth a program may
// reach is now the depth memory allows, which is the same bound a list's length
// has, and PLAN §2.6 puts this change ahead of M9 so the evaluator is written
// onto a stack that already exists rather than growing one afterwards.
//
// PUSHED IN REVERSE, POPPED IN SOURCE ORDER, and that is the whole translation.
// Everything the recursive version got for free is an explicit action instead --
// a scope that closes after its children, a name that enters scope after its
// initialiser, and the two halves of member() and call() that sit on either
// side of theirs. Those actions are resolve_internal.hpp's `Act` and they are
// the only subtlety in the change: the ORDER of the visits is unchanged, which
// is what keeps slot numbers, `walked` counts and every diagnostic identical.
//
// THE TABLE IN ast.hpp IS THE SPECIFICATION FOR EVERY LINE BELOW. A node's four
// payload words mean different things in each kind, and that table is the one
// place they are written down -- so every `.a` here is read out of it rather
// than remembered, and a kind whose children are not visited is a name that
// silently resolves to nothing.

#include "name_resolver/resolve_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"

#include <cstdint>
#include <string_view>

namespace satellite::resolve {

// The two entry points a pass calls, and they are the only two that DRAIN.
// Everything inside the walk pushes, so the stack is emptied exactly once per
// top-level expression or body and never re-entered from inside itself -- which
// is what makes the depth in this file a property of the vector and not of the
// C++ stack after all.
void Resolver::expression(NodeIndex node)
{
    visit_expression(node);
    run_work();
}

void Resolver::statement(NodeIndex node)
{
    visit_statement(node);
    run_work();
}

void Resolver::run_work()
{
    while (!work_.empty()) {
        const Work item = work_.back();
        work_.pop_back();

        switch (item.act) {
        case Act::Expression:
            expression_at(item.node);
            break;
        case Act::Statement:
            statement_at(item.node);
            break;
        case Act::CloseScope:
            close_scope();
            break;
        case Act::Declare:
            declare(item.node, ast_[item.node].token, item.type);
            break;
        case Act::MemberDone:
            member_done(item.node);
            break;
        case Act::CallTargetDone:
            call_target_done(item.node);
            break;
        }
    }
}

// A call's arguments, which three sites in names.cpp reach and each used to
// write out. Reversed on the way in so they come back off in source order.
void Resolver::visit_arguments(NodeIndex call)
{
    const ListId args = ast_[call].b;
    for (uint32_t i = ast_.list_size(args); i-- > 0;)
        visit_expression(ast_.list_at(args, i));
}

void Resolver::expression_at(NodeIndex node)
{
    if (node == kNoNode)
        return;

    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::Name:
        name(node);
        break;

    case NodeKind::Member:
        member(node);
        break;

    case NodeKind::Call:
        call(node);
        break;

    case NodeKind::Index:
        visit_expression(n.b);
        visit_expression(n.a);
        break;

    case NodeKind::Slice:
        visit_expression(n.c);
        visit_expression(n.b);
        visit_expression(n.a);
        break;

    case NodeKind::Unary:
        visit_expression(n.a);
        break;

    case NodeKind::Binary:
        visit_expression(n.b);
        visit_expression(n.a);
        break;

    case NodeKind::Type:
        type_of(node);
        break;

    // A LITERAL AND THE RESERVED WORD RESOLVE TO NOTHING, and that is the
    // answer rather than a gap. `satellite` used as a value is DESIGN §6's
    // `primary` -- `satellite.return(satellite)` is what it is for -- and it
    // names the language's root, which has no slot and needs none.
    case NodeKind::Number:
    case NodeKind::String:
    case NodeKind::Bits:
    case NodeKind::Satellite:
    default:
        break;
    }
}

void Resolver::statement_at(NodeIndex node)
{
    if (node == kNoNode)
        return;

    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::VarDecl: {
        const words::PathId type = type_of(n.a);
        // THE INITIALISER IS RESOLVED BEFORE THE NAME ENTERS SCOPE, which
        // decides one real program: `satellite.variable.number x = x` names the
        // OUTER x if there is one and is an unknown name if there is not.
        // Declaring first would make it name itself, reading a slot that has
        // never been written -- and §7.4's fresh slot is exactly what makes
        // that reachable, because the shadowed x is still there. The Declare
        // action is that sentence: the recursive version got the ordering from
        // where the call sat, and this one has to say it.
        work_.push_back({Act::Declare, node, type});
        visit_expression(n.b);
        break;
    }

    case NodeKind::Assign:
        // THE VALUE BEFORE THE TARGET, for the same reason and one form
        // further: `x = x + 1` reads the old x on the right.
        visit_expression(n.a);
        visit_expression(n.b);
        break;

    case NodeKind::ExprStmt:
        visit_expression(n.a);
        break;

    case NodeKind::Return:
        statement_form(node, words::NodeId::SATELLITE, "return");
        break;

    case NodeKind::Block:
        open_scope();
        work_.push_back({Act::CloseScope, node, words::kNoPath});
        for (uint32_t i = ast_.list_size(n.a); i-- > 0;)
            visit_statement(ast_.list_at(n.a, i));
        break;

    case NodeKind::If:
        visit_statement(n.c);
        visit_statement(n.b);
        visit_expression(n.a);
        break;

    case NodeKind::While:
        visit_statement(n.b);
        visit_expression(n.a);
        break;

    case NodeKind::For:
        // THE INITIALISER'S SCOPE IS THE LOOP AND NOT THE BLOCK INSIDE IT, so
        // `for` opens one of its own. Without it a counter declared in the head
        // would outlive the loop, and the next `for` in the same body would
        // find it already bound -- which §7.4 makes harmless and confusing at
        // once, since it would silently take a second slot.
        open_scope();
        work_.push_back({Act::CloseScope, node, words::kNoPath});
        visit_statement(n.d);
        visit_statement(n.c);
        visit_expression(n.b);
        visit_statement(n.a);
        break;

    default:
        break;
    }
}

void Resolver::body_of(NodeIndex capsule, Frame &frame)
{
    const Node &n = ast_[capsule];

    frame_ = &frame;
    bindings_.clear();
    scopes_.clear();
    open_scope();

    // THE PARAMETERS ARE SLOTS 0..n AND THAT IS §7.1's SENTENCE IN THE TREE.
    // "This cannot be reframed as deliberate capsule-static semantics, because
    // parameters are locals too -- `arguments` would be a program-wide static."
    // The parser already agrees: a parameter is a VarDecl with no initialiser,
    // and parser_types.cpp says it reuses the kind for exactly this reason.
    for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
        const NodeIndex param = ast_.list_at(n.b, i);
        const words::PathId type = type_of(ast_[param].a);
        const std::string_view spelling = ast_.text_of(param);

        if (const Binding *first = lookup(spelling)) {
            problem<errors::Code::RESOLVE_PARAMETER_TWICE>(param, spelling);
            attach(errors::note<errors::Code::NOTE_DECLARED_FIRST_HERE>(
                span_of(first->at), spelling));
            continue;
        }

        const Slot slot = declare(param, ast_[param].token, type);
        main_parameter(param, spelling, slot);
    }
    frame.parameters = static_cast<uint32_t>(frame.names.size());

    // The returns clause is a type like any other and is checked here rather
    // than in the loop above, because it is not a slot -- nothing is stored in
    // it and M9's `Value` is what will carry it back.
    type_of(n.c);

    statement(n.d);

    close_scope();
    bindings_.clear();
    frame_ = nullptr;
}

} // namespace satellite::resolve
