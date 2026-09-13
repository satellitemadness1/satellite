// DESIGN §6's expression and §6's type, printed back. See
// abstract_syntax_tree/unparse_internal.hpp for the Printer and why it is three
// files.
//
// BRACKETS ARE PUT BACK FROM PRECEDENCE AND NOT FROM MEMORY, because no node
// records that a parenthesis was written (parser_expressions.cpp says why). A
// child needs brackets when it binds LOOSER than its parent, and the right-hand
// child needs them when it binds EQUALLY TOO -- `a - (b - c)` must not come
// back as `a - b - c`, which is the one case that silently changes an answer.
//
// EVERY LITERAL IS PRINTED FROM ITS TOKEN'S `text`, WHICH IS THE SPELLING THE
// FILE USED. lexer.hpp keeps both halves of a string literal for exactly this
// reason and says so: `str` is the value the program means and `text` is what
// the file says, expansion is not reversible -- "a\nb" and a body with a real
// newline in it expand to the same SatString -- so a printer that read `str`
// would rewrite the source it was handed. The same rule is why `x0009` comes
// back as `x0009` and not as `x9`: DESIGN §8.5 makes the width part of the
// value, and the token is where the width is.

#include "abstract_syntax_tree/unparse_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>

namespace satellite {

void Printer::expand_expression(NodeIndex node)
{
    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::Number:
    case NodeKind::Bits:
    case NodeKind::Name:
        say(text(node));
        return;
    case NodeKind::String:
        // The body as written, quotes back around it. The escapes in it
        // were never expanded on this side of the tree.
        say("\"" + text(node) + "\"");
        return;
    case NodeKind::Satellite:
        say("satellite");
        return;
    case NodeKind::Member:
        expr(n.a);
        say("." + text(node));
        return;
    case NodeKind::Call:
        expr(n.a);
        say("(");
        for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
            if (i > 0)
                say(", ");
            expr(ast_.list_at(n.b, i));
        }
        // Named arguments after the positional ones, which is the only order
        // the parser accepts -- M30.
        for (uint32_t i = 0; n.c != kNoList && i < ast_.list_size(n.c); i++) {
            if (i > 0 || ast_.list_size(n.b) > 0)
                say(", ");
            const NodeIndex named = ast_.list_at(n.c, i);
            say(text(named) + "=");
            expr(ast_[named].a);
        }
        say(")");
        return;
    case NodeKind::Index:
        expr(n.a);
        say("[");
        expr(n.b);
        say("]");
        return;
    case NodeKind::Slice:
        expr(n.a);
        say("[");
        if (n.b != kNoNode)
            expr(n.b);
        say(":");
        if (n.c != kNoNode)
            expr(n.c);
        say("]");
        return;
    case NodeKind::Unary:
        // ABOVE EVERY BINARY LEVEL, so a binary operand is always
        // bracketed: `-(a + b)` is not `-a + b`.
        say(text(node));
        bracketed(n.a, kBindsTighterThanAny, false);
        return;
    case NodeKind::Binary: {
        const int level = precedence_of(text(node));
        bracketed(n.a, level, false);
        say(" " + text(node) + " ");
        bracketed(n.b, level, true);
        return;
    }
    case NodeKind::Type:
        type_of(node);
        return;
    default:
        // NOT REACHABLE FROM A PARSE, and named rather than silently
        // printed as something plausible: every kind above is one DESIGN
        // §6's `expression` can produce, and a statement in an expression
        // slot is a tree that was built by hand and is wrong.
        say("<" + std::string(kind_name(n.kind)) + " is not an expression>");
        return;
    }
}

void Printer::expand_type(NodeIndex node)
{
    if (node == kNoNode)
        return;
    const Node &n = ast_[node];
    // No type space: either the singleton `satellite` or a spacesuit named
    // bare, and the token says which without a third field.
    if (n.a == words::kNoSpelling) {
        // `ship.box` -- M25's qualified spacesuit, printed as it was written.
        if (const uint32_t qualifier = ast_.qualifier_of(node); qualifier != 0)
            say(std::string(ast_.token(qualifier).text) + ".");
        say(text(node));
        return;
    }

    say("satellite." +
        std::string(words::spelling_of(static_cast<words::NodeId>(n.a))) +
        "." + text(node));
    if (n.b == kNoList)
        return;
    say("<");
    for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
        if (i > 0)
            say(", ");
        type_of(ast_.list_at(n.b, i));
    }
    say(">");
}

void Printer::expand_inline(NodeIndex node)
{
    if (node == kNoNode)
        return;
    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::VarDecl:
        type_of(n.a);
        say(" " + text(node));
        constructor_arguments(n.c);
        if (n.b != kNoNode) {
            say(" = ");
            expr(n.b);
        }
        return;
    case NodeKind::Assign:
        expr(n.a);
        say(" = ");
        expr(n.b);
        return;
    case NodeKind::ExprStmt:
        expr(n.a);
        return;
    default:
        expr(node);
        return;
    }
}

// A child, with brackets when it binds looser than its parent -- or as
// tightly, on the right, which is what makes `a - (b - c)` survive. The
// decision is the PARENT's and is taken here, before the child is pushed,
// which is what keeps it out of the child's own case.
void Printer::bracketed(NodeIndex node, int level, bool on_the_right)
{
    const Node &n = ast_[node];
    const int child = n.kind == NodeKind::Binary ? precedence_of(text(node))
                                                 : 0;
    const bool needs = n.kind == NodeKind::Binary &&
                       (child < level || (on_the_right && child == level));
    if (!needs) {
        expr(node);
        return;
    }
    say("(");
    expr(node);
    say(")");
}
} // namespace satellite
