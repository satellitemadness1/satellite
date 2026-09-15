// What a `satellite.include(...)` names. See satellite_spaceship/shape.hpp.

#include "satellite_spaceship/shape.hpp"

namespace satellite::spaceship {

namespace {

// `ship` or `ship.satl`, and which Name node spells the spaceship. The
// extension is the one member a spaceship's name may carry: `ship.other` is a
// member, not a file, and is refused rather than read as `ship`. A quoted path
// is the other road to a file, and its String node is the spelling.
NodeIndex ship_name(const Ast &ast, NodeIndex node)
{
    if (node == kNoNode)
        return kNoNode;
    if (ast[node].kind == NodeKind::Name || ast[node].kind == NodeKind::String)
        return node;
    if (ast[node].kind == NodeKind::Member && ast.text_of(node) == "satl" &&
        ast[node].a != kNoNode && ast[ast[node].a].kind == NodeKind::Name)
        return ast[node].a;
    return kNoNode;
}

// The file name at the end of a path, less `.satl` when it is written:
// "../parts/ship.satl" -> "ship". A view into `text`.
std::string_view stem_of(std::string_view text)
{
    const size_t slash = text.rfind('/');
    std::string_view base = slash == std::string_view::npos ? text : text.substr(slash + 1);
    if (base.size() > 5 && base.substr(base.size() - 5) == ".satl")
        base.remove_suffix(5);
    return base;
}

// A spaceship's name is a name: a letter or `_` first, then letters, digits and
// `_`. The tree's text of a string keeps its escapes, so a backslash anywhere in
// the path fails here too -- a path is written plainly or not at all. So does a
// control byte: found by review, a raw NUL in "parts/ship<NUL>x" loaded the
// file named by the bytes before it.
bool a_name(std::string_view text)
{
    if (text.empty())
        return false;
    for (size_t i = 0; i < text.size(); i++) {
        const char c = text[i];
        const bool letter = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
        if (!letter && !(i > 0 && c >= '0' && c <= '9'))
            return false;
    }
    return true;
}

Shape spelled_by(const Ast &ast, NodeIndex name)
{
    Shape out;
    out.named = Named::Spaceship;
    out.name = name;
    if (ast[name].kind == NodeKind::String) {
        out.path = true;
        const std::string_view written = ast.text_of(name);
        bool plain = written.find('\\') == std::string_view::npos;
        for (const char c : written)
            plain = plain && static_cast<unsigned char>(c) >= 0x20 && c != 0x7f;
        if (!a_name(stem_of(written)) || !plain)
            out.named = Named::BadPath;
    }
    return out;
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
    if (name != kNoNode)
        return spelled_by(ast, name);

    // A CALL WHOSE TARGET SPELLS A SPACESHIP, AND NO NAMED OPTIONS. `ship(x)`
    // hands `x` to a launch capsule, and a launch is a capsule the program
    // wrote -- which S0527 already says takes no option.
    if (ast[what].kind == NodeKind::Call && ast[what].c == kNoList) {
        name = ship_name(ast, ast[what].a);
        if (name != kNoNode) {
            out = spelled_by(ast, name);
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
    if (shape.name == kNoNode)
        return std::string_view();
    return shape.path ? stem_of(ast.text_of(shape.name)) : ast.text_of(shape.name);
}

std::string_view written_of(const Ast &ast, const Shape &shape)
{
    return shape.name == kNoNode ? std::string_view() : ast.text_of(shape.name);
}

} // namespace satellite::spaceship
