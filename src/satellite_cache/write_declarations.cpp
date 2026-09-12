// DESIGN §6's top_level and the blocks that hold statements, written with the
// language's words as their numbers. See satellite_cache/write_internal.hpp.
//
// EVERY FORM HERE IS ONE THE PARSER GAVE A NODE OF ITS OWN, and that is why the
// file exists as a separate thing from write_expressions.cpp next door.
// `satellite.statement.if` is not a Member chain in the tree -- it is an If
// node -- so none of these reaches the chain matcher and each has to find its
// number by naming the path it always is. fixed() is that lookup and it is in
// write.cpp, with everything else that spells a number.
//
// A CASE NAMES ITS PIECES IN SOURCE ORDER -- line_starts(), the pieces of the
// line, line_ends() -- and write.cpp's flush() is the only place that order is
// reversed. So a case still reads the way its output does, which is the
// property the recursive version had for free and the one worth paying for.

#include "satellite_cache/write_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>

namespace satellite::cache {

void Writer::program(NodeIndex node)
{
    const ListId items = ast_[node].a;
    for (uint32_t i = 0; i < ast_.list_size(items); i++) {
        if (i > 0)
            newline();
        decl(ast_.list_at(items, i));
    }
    run();
}

void Writer::expand_declaration(NodeIndex node)
{
    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::Include: {
        // §5.1 STEP 3 IN ITS FIRST OF TWO PLACES. The argument is not
        // written when the row's own argument list is the reserved word,
        // because 1.1.1 already says `include(satellite)`; a spaceship's
        // name is a user name and stays one, at 1.1.2.
        const bool reserved =
            n.a != kNoNode && ast_[n.a].kind == NodeKind::Satellite;
        // THE ARITY IS COUNTED AND NOT ASSUMED, which it was until M17: this
        // read `1` flat, so `satellite.include()` would have been written out
        // as `#1.1.2` -- the SPACESHIP row -- had the parser been able to
        // build one. It could not, so the wrong constant was unreachable
        // rather than wrong, and it became reachable the moment `1 1 0`
        // parsed. form() then prints a zero-arity row with no parentheses of
        // its own, and unnumber.cpp reads `#1.1.0` back as the same three
        // words.
        const int argc = n.a != kNoNode ? 1 : 0;
        const PathMatch shape =
            shape_path(words::NodeId::SATELLITE, "include", argc, reserved);
        line_starts();
        form(shape, n.a);
        line_ends();
        return;
    }
    case NodeKind::Global:
        line_starts();
        global_name(node);
        if (n.b != kNoNode) {
            say(" = ");
            expr(n.b);
        }
        line_ends();
        return;
    case NodeKind::Capsule:
        capsule(node);
        return;
    case NodeKind::Spacesuit:
        spacesuit(node);
        return;
    default:
        stmt(node);
        return;
    }
}

// `counter tally("hello")`'s arguments, or nothing when none were written --
// 2026-09-12. An empty `()` is written back as `()`, so the file says what the
// program said.
void Writer::constructor_arguments(ListId arguments)
{
    if (arguments == kNoList)
        return;
    say("(");
    for (uint32_t i = 0; i < ast_.list_size(arguments); i++) {
        if (i > 0)
            say(", ");
        expr(ast_.list_at(arguments, i));
    }
    say(")");
}

void Writer::capsule(NodeIndex node)
{
    const Node &n = ast_[node];
    line_starts();
    if (ast_.is_constructor(node)) {
        fixed("satellite.constructor");
    } else {
        fixed("satellite.capsule");
        say(" ");
        capsule_name(node);
    }
    say("(");
    for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
        if (i > 0)
            say(", ");
        const NodeIndex param = ast_.list_at(n.b, i);
        type_of(ast_[param].a);
        say(" " + text(param));
    }
    say(")");
    if (n.c != kNoNode) {
        say(" ");
        fixed("satellite.returns");
        say("(");
        type_of(n.c);
        say(")");
    }
    line_ends();
    block_of(n.d);
}

void Writer::spacesuit(NodeIndex node)
{
    const Node &n = ast_[node];
    line_starts();
    fixed("satellite.spacesuit");
    say(" " + text(node));
    if (n.c != kNoNode)
        say("(" + text(n.c) + ")");
    line_ends();
    members_of(n.b);
}

void Writer::expand_members(ListId items)
{
    line_starts();
    say("{");
    line_ends();
    indent();
    for (uint32_t i = 0; i < ast_.list_size(items); i++) {
        if (i > 0)
            newline();
        const NodeIndex item = ast_.list_at(items, i);
        if (ast_[item].kind == NodeKind::Section) {
            line_starts();
            fixed("satellite." + text(item));
            line_ends();
            members_of(ast_[item].a);
        } else {
            decl(item);
        }
    }
    dedent();
    line_starts();
    say("}");
    line_ends();
}

void Writer::expand_block(NodeIndex node)
{
    line_starts();
    say("{");
    line_ends();
    indent();
    const ListId statements = ast_[node].a;
    for (uint32_t i = 0; i < ast_.list_size(statements); i++)
        decl(ast_.list_at(statements, i));
    dedent();
    line_starts();
    say("}");
    line_ends();
}

void Writer::expand_statement(NodeIndex node)
{
    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::VarDecl:
        line_starts();
        type_of(n.a);
        say(" " + text(node));
        constructor_arguments(n.c);
        if (n.b != kNoNode) {
            say(" = ");
            expr(n.b);
        }
        line_ends();
        return;
    case NodeKind::Assign:
        line_starts();
        expr(n.a);
        say(" = ");
        expr(n.b);
        line_ends();
        return;
    case NodeKind::ExprStmt:
        line_starts();
        expr(n.a);
        line_ends();
        return;
    case NodeKind::Return: {
        // §5.1 STEP 3 IN ITS SECOND AND LAST PLACE. `satellite.return()` is
        // 1 15 0, `satellite.return(satellite)` is 1 15 1 with nothing
        // written after it, and `satellite.return(x)` is 1 15 2 with the
        // value still to print. Three rows and three readings, slotted by
        // counting -- which is what §5.1 step 4 means by "counting, not
        // resolving".
        const bool reserved =
            n.a != kNoNode && ast_[n.a].kind == NodeKind::Satellite;
        const PathMatch shape = shape_path(words::NodeId::SATELLITE, "return",
                                           n.a == kNoNode ? 0 : 1, reserved);
        line_starts();
        form(shape, n.a);
        line_ends();
        return;
    }
    case NodeKind::Block:
        block_of(node);
        return;
    case NodeKind::If:
        line_starts();
        fixed("satellite.statement.if");
        say(" (");
        expr(n.a);
        say(")");
        line_ends();
        block_of(n.b);
        if (n.c != kNoNode) {
            line_starts();
            fixed("satellite.statement.else");
            line_ends();
            stmt(n.c);
        }
        return;
    case NodeKind::While:
        line_starts();
        fixed("satellite.statement.while");
        say(" (");
        expr(n.a);
        say(")");
        line_ends();
        block_of(n.b);
        return;
    case NodeKind::For:
        line_starts();
        fixed("satellite.statement.for");
        say(" (");
        inline_of(n.a);
        say("; ");
        if (n.b != kNoNode)
            expr(n.b);
        say("; ");
        inline_of(n.c);
        say(")");
        line_ends();
        block_of(n.d);
        return;
    default:
        line_starts();
        expr(node);
        line_ends();
        return;
    }
}

void Writer::expand_inline(NodeIndex node)
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

} // namespace satellite::cache
