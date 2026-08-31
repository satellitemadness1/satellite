// The walk over statements and expressions, and one capsule's frame being
// filled -- pass 4 of DESIGN §7.3. See name_resolver/resolve_internal.hpp for
// the split and name_resolver/resolve.hpp for why the depth bound here is not
// DESIGN §7.5's.
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

void Resolver::expression(NodeIndex node)
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
        expression(n.a);
        expression(n.b);
        break;

    case NodeKind::Slice:
        expression(n.a);
        expression(n.b);
        expression(n.c);
        break;

    case NodeKind::Unary:
        expression(n.a);
        break;

    case NodeKind::Binary:
        expression(n.a);
        expression(n.b);
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

void Resolver::statement(NodeIndex node)
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
        // that reachable, because the shadowed x is still there.
        expression(n.b);
        declare(node, n.token, type);
        break;
    }

    case NodeKind::Assign:
        // THE VALUE BEFORE THE TARGET, for the same reason and one form
        // further: `x = x + 1` reads the old x on the right.
        expression(n.b);
        expression(n.a);
        break;

    case NodeKind::ExprStmt:
        expression(n.a);
        break;

    case NodeKind::Return:
        statement_form(node, words::NodeId::SATELLITE, "return");
        break;

    case NodeKind::Block:
        open_scope();
        for (uint32_t i = 0; i < ast_.list_size(n.a); i++)
            statement(ast_.list_at(n.a, i));
        close_scope();
        break;

    case NodeKind::If:
        expression(n.a);
        statement(n.b);
        statement(n.c);
        break;

    case NodeKind::While:
        expression(n.a);
        statement(n.b);
        break;

    case NodeKind::For:
        // THE INITIALISER'S SCOPE IS THE LOOP AND NOT THE BLOCK INSIDE IT, so
        // `for` opens one of its own. Without it a counter declared in the head
        // would outlive the loop, and the next `for` in the same body would
        // find it already bound -- which §7.4 makes harmless and confusing at
        // once, since it would silently take a second slot.
        open_scope();
        statement(n.a);
        expression(n.b);
        statement(n.c);
        statement(n.d);
        close_scope();
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
