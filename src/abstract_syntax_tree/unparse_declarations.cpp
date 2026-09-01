// DESIGN §6's top_level and the blocks that hold statements, printed back as a
// program. See abstract_syntax_tree/unparse_internal.hpp for the Printer and
// why it is three files.
//
// EVERY FORM HERE IS ONE THE PARSER GAVE A NODE OF ITS OWN, which is the same
// division satellite_cache/write_declarations.cpp is cut on one module over:
// `satellite.statement.if` is not a Member chain in the tree, it is an If node,
// so it is printed by naming the keyword it always is. What the file next door
// does differently is print a NUMBER there instead, and that is the whole of the
// difference between the two printers.
//
// A CASE NAMES ITS PIECES IN SOURCE ORDER and flush() reverses them once, so
// every case still reads the way its output does.

#include "abstract_syntax_tree/unparse_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>

namespace satellite {

void Printer::expand_program(NodeIndex node)
{
    const ListId items = ast_[node].a;
    for (uint32_t i = 0; i < ast_.list_size(items); i++) {
        if (i > 0)
            newline();
        decl(ast_.list_at(items, i));
    }
}

void Printer::expand_declaration(NodeIndex node)
{
    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::Include:
        pad();
        say("satellite.include(");
        expr(n.a);
        say(")");
        newline();
        return;
    case NodeKind::Global:
        pad();
        say("satellite.library." + text(node));
        if (n.b != kNoNode) {
            say(" = ");
            expr(n.b);
        }
        newline();
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

void Printer::capsule(NodeIndex node)
{
    const Node &n = ast_[node];
    pad();
    // THE RESERVED NAME IS THE ONE THE LANGUAGE HAS A NUMBER FOR, and
    // is_language_word on the path id is how that is asked -- one compare
    // against the frozen half's boundary, which words_nodes.hpp calls the
    // predicate anything about to write a PathId down has to ask first.
    say("satellite.capsule ");
    if (words::is_language_word(n.a))
        say("satellite.");
    say(text(node) + "(");
    for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
        if (i > 0)
            say(", ");
        const NodeIndex param = ast_.list_at(n.b, i);
        type_of(ast_[param].a);
        say(" " + text(param));
    }
    say(")");
    if (n.c != kNoNode) {
        say(" satellite.returns(");
        type_of(n.c);
        say(")");
    }
    newline();
    block_of(n.d);
}

void Printer::spacesuit(NodeIndex node)
{
    const Node &n = ast_[node];
    pad();
    say("satellite.spacesuit " + text(node));
    if (n.c != kNoNode)
        say("(" + text(n.c) + ")");
    newline();
    members_of(n.b);
}

// A spacesuit's body, or a section's -- one function, because the parser
// reads them with one loop and for the reason it records: a section holds
// what a suit block holds.
void Printer::expand_members(ListId items)
{
    pad();
    say("{");
    newline();
    indent();
    for (uint32_t i = 0; i < ast_.list_size(items); i++) {
        if (i > 0)
            newline();
        const NodeIndex item = ast_.list_at(items, i);
        if (ast_[item].kind == NodeKind::Section) {
            pad();
            say("satellite." + text(item));
            newline();
            members_of(ast_[item].a);
        } else {
            decl(item);
        }
    }
    dedent();
    pad();
    say("}");
    newline();
}

void Printer::expand_block(NodeIndex node)
{
    pad();
    say("{");
    newline();
    indent();
    const ListId statements = ast_[node].a;
    for (uint32_t i = 0; i < ast_.list_size(statements); i++)
        decl(ast_.list_at(statements, i));
    dedent();
    pad();
    say("}");
    newline();
}

void Printer::expand_statement(NodeIndex node)
{
    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::VarDecl:
        pad();
        type_of(n.a);
        say(" " + text(node));
        if (n.b != kNoNode) {
            say(" = ");
            expr(n.b);
        }
        newline();
        return;
    case NodeKind::Assign:
        pad();
        expr(n.a);
        say(" = ");
        expr(n.b);
        newline();
        return;
    case NodeKind::ExprStmt:
        pad();
        expr(n.a);
        newline();
        return;
    case NodeKind::Return:
        pad();
        say("satellite.return(");
        if (n.a != kNoNode)
            expr(n.a);
        say(")");
        newline();
        return;
    case NodeKind::Block:
        block_of(node);
        return;
    case NodeKind::If:
        pad();
        say("satellite.statement.if (");
        expr(n.a);
        say(")");
        newline();
        block_of(n.b);
        if (n.c != kNoNode) {
            pad();
            say("satellite.statement.else");
            newline();
            // AN ELSE-IF IS AN If IN THE else SLOT and is printed as one --
            // on its own line, which is legal because the parser crosses
            // the newline between `else` and what follows it. Printing
            // `else if` on one line would be the C shape and would make
            // the tree's own nesting invisible.
            stmt(n.c);
        }
        return;
    case NodeKind::While:
        pad();
        say("satellite.statement.while (");
        expr(n.a);
        say(")");
        newline();
        block_of(n.b);
        return;
    case NodeKind::For:
        // The init and the step are statements written inline, which is the
        // one place in the grammar where a statement has no line of its own.
        pad();
        say("satellite.statement.for (");
        inline_of(n.a);
        say("; ");
        if (n.b != kNoNode)
            expr(n.b);
        say("; ");
        inline_of(n.c);
        say(")");
        newline();
        block_of(n.d);
        return;
    default:
        pad();
        expr(node);
        newline();
        return;
    }
}
} // namespace satellite
