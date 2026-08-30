// DESIGN §6's top_level and the blocks that hold statements, written with the
// language's words as their numbers. See satellite_cache/write_internal.hpp.
//
// EVERY FORM HERE IS ONE THE PARSER GAVE A NODE OF ITS OWN, and that is why the
// file exists as a separate thing from write_expressions.cpp next door.
// `satellite.statement.if` is not a Member chain in the tree -- it is an If
// node -- so none of these reaches the chain matcher and each has to find its
// number by naming the path it always is. fixed() is that lookup and it is in
// write.cpp, with everything else that spells a number.

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
            out_ += "\n";
        declaration(ast_.list_at(items, i));
    }
}

void Writer::declaration(NodeIndex node)
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
        const PathMatch shape =
            shape_path(words::NodeId::SATELLITE, "include", 1, reserved);
        line(numbered_form(shape, n.a));
        return;
    }
    case NodeKind::Global: {
        const std::string head = global_name(node);
        line(n.b != kNoNode ? head + " = " + expression(n.b) : head);
        return;
    }
    case NodeKind::Capsule:
        capsule(node);
        return;
    case NodeKind::Spacesuit:
        spacesuit(node);
        return;
    default:
        statement(node);
        return;
    }
}

void Writer::capsule(NodeIndex node)
{
    const Node &n = ast_[node];
    std::string head = fixed("satellite.capsule") + " " + capsule_name(node) + "(";
    for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
        if (i > 0)
            head += ", ";
        const NodeIndex param = ast_.list_at(n.b, i);
        const std::string declared = type(ast_[param].a);
        head += declared + " " + text(param);
    }
    head += ")";
    if (n.c != kNoNode) {
        const std::string returns = fixed("satellite.returns");
        head += " " + returns + "(" + type(n.c) + ")";
    }
    line(head);
    block(n.d);
}

void Writer::spacesuit(NodeIndex node)
{
    const Node &n = ast_[node];
    std::string head = fixed("satellite.spacesuit") + " " + text(node);
    if (n.c != kNoNode)
        head += "(" + text(n.c) + ")";
    line(head);
    members(n.b);
}

void Writer::members(ListId items)
{
    line("{");
    indent_++;
    for (uint32_t i = 0; i < ast_.list_size(items); i++) {
        if (i > 0)
            out_ += "\n";
        const NodeIndex item = ast_.list_at(items, i);
        if (ast_[item].kind == NodeKind::Section) {
            line(fixed("satellite." + text(item)));
            members(ast_[item].a);
        } else {
            declaration(item);
        }
    }
    indent_--;
    line("}");
}

void Writer::block(NodeIndex node)
{
    line("{");
    indent_++;
    const ListId statements = ast_[node].a;
    for (uint32_t i = 0; i < ast_.list_size(statements); i++)
        declaration(ast_.list_at(statements, i));
    indent_--;
    line("}");
}

void Writer::statement(NodeIndex node)
{
    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::VarDecl: {
        const std::string declared = type(n.a);
        const std::string head = declared + " " + text(node);
        line(n.b != kNoNode ? head + " = " + expression(n.b) : head);
        return;
    }
    case NodeKind::Assign: {
        const std::string target = expression(n.a);
        const std::string value = expression(n.b);
        line(target + " = " + value);
        return;
    }
    case NodeKind::ExprStmt:
        line(expression(n.a));
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
        line(numbered_form(shape, n.a));
        return;
    }
    case NodeKind::Block:
        block(node);
        return;
    case NodeKind::If: {
        const std::string head = fixed("satellite.statement.if");
        line(head + " (" + expression(n.a) + ")");
        block(n.b);
        if (n.c != kNoNode) {
            line(fixed("satellite.statement.else"));
            statement(n.c);
        }
        return;
    }
    case NodeKind::While: {
        const std::string head = fixed("satellite.statement.while");
        line(head + " (" + expression(n.a) + ")");
        block(n.b);
        return;
    }
    case NodeKind::For: {
        const std::string head = fixed("satellite.statement.for");
        const std::string init = one(n.a);
        const std::string test = n.b != kNoNode ? expression(n.b) : std::string();
        const std::string step = one(n.c);
        line(head + " (" + init + "; " + test + "; " + step + ")");
        block(n.d);
        return;
    }
    default:
        line(expression(node));
        return;
    }
}

std::string Writer::one(NodeIndex node)
{
    if (node == kNoNode)
        return std::string();
    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::VarDecl: {
        const std::string declared = type(n.a);
        const std::string head = declared + " " + text(node);
        return n.b != kNoNode ? head + " = " + expression(n.b) : head;
    }
    case NodeKind::Assign: {
        const std::string target = expression(n.a);
        const std::string value = expression(n.b);
        return target + " = " + value;
    }
    case NodeKind::ExprStmt:
        return expression(n.a);
    default:
        return expression(node);
    }
}

} // namespace satellite::cache
