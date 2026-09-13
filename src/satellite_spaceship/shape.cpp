// What a `satellite.include(...)` names. See satellite_spaceship/shape.hpp.

#include "satellite_spaceship/shape.hpp"

namespace satellite::spaceship {

namespace {

// `ship` or `ship.satl`, and which Name node spells the spaceship. The
// extension is the one member a spaceship's name may carry: `ship.other` is a
// path, not a file, and is refused rather than read as `ship`.
NodeIndex ship_name(const Ast &ast, NodeIndex node)
{
    if (node == kNoNode)
        return kNoNode;
    if (ast[node].kind == NodeKind::Name)
        return node;
    if (ast[node].kind == NodeKind::Member && ast.text_of(node) == "satl" &&
        ast[node].a != kNoNode && ast[ast[node].a].kind == NodeKind::Name)
        return ast[node].a;
    return kNoNode;
}

} // namespace

Shape shape_of(const Ast &ast, NodeIndex include)
{
    Shape out;
    const NodeIndex what = ast[include].a;
    if (what == kNoNode)
        return out;
    if (ast[what].kind == NodeKind::Satellite) {
        out.named = Named::Runtime;
        return out;
    }

    NodeIndex name = ship_name(ast, what);
    if (name != kNoNode) {
        out.named = Named::Spaceship;
        out.name = name;
        return out;
    }

    // A CALL WHOSE TARGET SPELLS A SPACESHIP, AND NO NAMED OPTIONS. `ship(x)`
    // hands `x` to a launch capsule, and a launch is a capsule the program
    // wrote -- which S0527 already says takes no option.
    if (ast[what].kind == NodeKind::Call && ast[what].c == kNoList) {
        name = ship_name(ast, ast[what].a);
        if (name != kNoNode) {
            out.named = Named::Spaceship;
            out.name = name;
            out.call = what;
            out.arguments = ast[what].b;
            return out;
        }
    }

    out.named = Named::NotAName;
    return out;
}

std::string_view name_of(const Ast &ast, const Shape &shape)
{
    return shape.name == kNoNode ? std::string_view() : ast.text_of(shape.name);
}

} // namespace satellite::spaceship
