// AST Unparser implementation -- Milestone 4 Prototype.

#include "unparse.hpp"

#include <string>

namespace satellite {

namespace {

std::string pad(int level)
{
    return std::string(static_cast<size_t>(level) * 4, ' ');
}

std::string nest(NodeIndex child, int outer, bool on_right, const AstArena &arena)
{
    const AstNode &node = arena.get(child);
    int inner = 0;
    if (node.kind == NodeKind::Binary) {
        if (const auto *b = std::get_if<BinaryExpr>(&node.data))
            inner = precedence(b->op);
    }
    bool wrap = inner != 0 && (on_right ? inner <= outer : inner < outer);
    return wrap ? "(" + unparse_node(child, arena) + ")" : unparse_node(child, arena);
}

Access access_of(const SuitItem &item)
{
    if (const auto *f = std::get_if<FieldDecl>(&item))
        return f->access;
    return std::get<MethodDecl>(item).access;
}

std::string unparse_capsule_decl(const CapsuleDecl &c, const AstArena &arena, int level)
{
    std::string out = pad(level) + "satellite.capsule ";
    out += c.reserved ? "satellite." + c.name : c.name;
    out += "(";
    for (size_t i = 0; i < c.params.size(); ++i) {
        if (i) out += ", ";
        out += unparse(c.params[i].type) + " " + c.params[i].name;
    }
    out += ")";
    if (c.returns)
        out += " satellite.returns(" + unparse(*c.returns) + ")";
    out += "\n" + unparse_node(c.body, arena, level);
    return out;
}

} // namespace

std::string unparse(const Type &type)
{
    if (type.is_singleton())
        return "satellite";
    if (type.is_spacesuit())
        return type.name;

    std::string out = "satellite." + type.space + "." + type.name;
    if (!type.args.empty()) {
        out += "<";
        for (size_t i = 0; i < type.args.size(); ++i) {
            if (i) out += ", ";
            out += unparse(type.args[i]);
        }
        out += ">";
    }
    return out;
}

std::string unparse_node(NodeIndex idx, const AstArena &arena, int level)
{
    if (idx == kNullNode)
        return "";

    const AstNode &node = arena.get(idx);
    const std::string lead = pad(level);

    switch (node.kind) {
    case NodeKind::NumberLit:
        return std::get<NumberLit>(node.data).text;

    case NodeKind::DurationLit:
        return std::get<DurationLit>(node.data).text;

    case NodeKind::BitsLit:
        return std::get<BitsLit>(node.data).text;

    case NodeKind::StringLit:
        return "\"" + std::get<StringLit>(node.data).text + "\"";

    case NodeKind::SatelliteLit:
        return "satellite";

    case NodeKind::Name:
        return std::get<NameExpr>(node.data).text;

    case NodeKind::Member: {
        const auto &m = std::get<MemberExpr>(node.data);
        return unparse_node(m.target, arena) + "." + m.name;
    }

    case NodeKind::Call: {
        const auto &c = std::get<CallExpr>(node.data);
        std::string out = unparse_node(c.target, arena) + "(";
        for (size_t i = 0; i < c.args.size(); ++i) {
            if (i) out += ", ";
            out += unparse_node(c.args[i], arena);
        }
        return out + ")";
    }

    case NodeKind::Index: {
        const auto &i = std::get<IndexExpr>(node.data);
        return unparse_node(i.target, arena) + "[" + unparse_node(i.subscript, arena) + "]";
    }

    case NodeKind::Slice: {
        const auto &s = std::get<SliceExpr>(node.data);
        std::string out = unparse_node(s.target, arena) + "[";
        if (s.lo != kNullNode) out += unparse_node(s.lo, arena);
        out += ":";
        if (s.hi != kNullNode) out += unparse_node(s.hi, arena);
        return out + "]";
    }

    case NodeKind::Unary: {
        const auto &u = std::get<UnaryExpr>(node.data);
        bool wrap = (arena.get(u.operand).kind == NodeKind::Binary);
        return u.op + (wrap ? "(" + unparse_node(u.operand, arena) + ")" : unparse_node(u.operand, arena));
    }

    case NodeKind::Binary: {
        const auto &b = std::get<BinaryExpr>(node.data);
        int here = precedence(b.op);
        return nest(b.left, here, false, arena) + " " + b.op + " " + nest(b.right, here, true, arena);
    }

    case NodeKind::ListLit: {
        const auto &l = std::get<ListLit>(node.data);
        std::string out = "{";
        for (size_t i = 0; i < l.elements.size(); ++i) {
            if (i) out += ", ";
            out += unparse_node(l.elements[i], arena);
        }
        return out + "}";
    }

    case NodeKind::NamedArg: {
        const auto &a = std::get<NamedArg>(node.data);
        return a.name + " = " + unparse_node(a.value, arena);
    }

    case NodeKind::VarDecl: {
        const auto &v = std::get<VarDeclStmt>(node.data);
        std::string out = lead + unparse(v.type) + " " + v.name;
        if (v.init != kNullNode)
            out += " = " + unparse_node(v.init, arena);
        return out;
    }

    case NodeKind::Assign: {
        const auto &a = std::get<AssignStmt>(node.data);
        return lead + unparse_node(a.target, arena) + " = " + unparse_node(a.value, arena);
    }

    case NodeKind::ExprStmt: {
        const auto &e = std::get<ExprStmt>(node.data);
        return lead + unparse_node(e.expr, arena);
    }

    case NodeKind::Return: {
        const auto &r = std::get<ReturnStmt>(node.data);
        std::string out = lead + "satellite.return(";
        if (r.value != kNullNode)
            out += unparse_node(r.value, arena);
        return out + ")";
    }

    case NodeKind::Block: {
        const auto &b = std::get<BlockStmt>(node.data);
        std::string out = lead + "{\n";
        for (NodeIndex s : b.statements)
            out += unparse_node(s, arena, level + 1) + "\n";
        return out + lead + "}";
    }

    case NodeKind::If: {
        const auto &i = std::get<IfStmt>(node.data);
        std::string out = lead + "satellite.statement.if (" + unparse_node(i.condition, arena) + ")\n";
        out += unparse_node(i.then_branch, arena, level);
        if (i.else_branch != kNullNode) {
            out += "\n" + lead + "satellite.statement.else\n" + unparse_node(i.else_branch, arena, level);
        }
        return out;
    }

    case NodeKind::While: {
        const auto &w = std::get<WhileStmt>(node.data);
        return lead + "satellite.statement.while (" + unparse_node(w.condition, arena) + ")\n" +
               unparse_node(w.body, arena, level);
    }

    case NodeKind::For: {
        const auto &f = std::get<ForStmt>(node.data);
        std::string out = lead + "satellite.statement.for (";
        if (f.init != kNullNode) out += unparse_node(f.init, arena, 0);
        out += "; ";
        if (f.condition != kNullNode) out += unparse_node(f.condition, arena, 0);
        out += "; ";
        if (f.step != kNullNode) out += unparse_node(f.step, arena, 0);
        return out + ")\n" + unparse_node(f.body, arena, level);
    }

    case NodeKind::Include: {
        const auto &inc = std::get<IncludeDecl>(node.data);
        return "satellite.include(" + unparse_node(inc.what, arena) + ")";
    }

    case NodeKind::Capsule:
        return unparse_capsule_decl(std::get<CapsuleDecl>(node.data), arena, level);

    case NodeKind::GlobalDecl: {
        const auto &g = std::get<GlobalDecl>(node.data);
        std::string out = "satellite.library." + g.name;
        if (g.init != kNullNode)
            out += " = " + unparse_node(g.init, arena);
        return out;
    }

    case NodeKind::Spacesuit: {
        const auto &s = std::get<SpacesuitDecl>(node.data);
        std::string out = "satellite.spacesuit " + s.name;
        out += "(" + s.super + ")\n{\n";

        for (size_t i = 0; i < s.items.size();) {
            const Access access = access_of(s.items[i]);
            out += pad(1) + access_keyword(access) + "\n" + pad(1) + "{\n";

            for (; i < s.items.size() && access_of(s.items[i]) == access; ++i) {
                if (const auto *f = std::get_if<FieldDecl>(&s.items[i])) {
                    std::string fline = pad(2) + unparse(f->type) + " " + f->name;
                    if (f->init != kNullNode)
                        fline += " = " + unparse_node(f->init, arena);
                    out += fline + "\n";
                    continue;
                }
                const auto &m = std::get<MethodDecl>(s.items[i]);
                if (!m.constructor) {
                    out += unparse_capsule_decl(m.capsule, arena, 2) + "\n";
                } else {
                    std::string cline = pad(2) + m.capsule.name + "(";
                    for (size_t p = 0; p < m.capsule.params.size(); ++p) {
                        if (p) cline += ", ";
                        cline += unparse(m.capsule.params[p].type) + " " + m.capsule.params[p].name;
                    }
                    cline += ")\n" + unparse_node(m.capsule.body, arena, 2);
                    out += cline + "\n";
                }
            }
            out += pad(1) + "}\n";
        }
        return out + "}";
    }

    case NodeKind::None:
        return "";
    }
    return "";
}

std::string unparse(const Program &program, const AstArena &arena)
{
    std::string out;
    for (NodeIndex idx : program.items) {
        if (!out.empty())
            out += "\n\n";
        out += unparse_node(idx, arena);
    }
    if (!out.empty())
        out += "\n";
    return out;
}

} // namespace satellite

