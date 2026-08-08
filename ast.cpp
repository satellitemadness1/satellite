#include "ast.hpp"

#include <utility>

namespace satellite {

Span span_of(const Token &token)
{
    return Span{token.start, token.end, token.line};
}

Span span_join(Span first, Span last)
{
    return Span{first.start, last.end, first.line};
}

ExprPtr make_expr(ExprBase node, Span span)
{
    Expr e(std::move(node));
    e.span = span;
    return std::make_shared<const Expr>(std::move(e));
}

StmtPtr make_stmt(StmtBase node, Span span)
{
    Stmt s(std::move(node));
    s.span = span;
    return std::make_shared<const Stmt>(std::move(s));
}

// ---------------------------------------------------------------------------
// Operators
// ---------------------------------------------------------------------------

int precedence(const std::string &op)
{
    if (op == "==" || op == "!=" || op == "<" || op == ">" ||
        op == "<=" || op == ">=")
        return 1;
    if (op == "+" || op == "-")
        return 2;
    if (op == "*" || op == "/" || op == "%")
        return 3;
    return 0;
}

bool is_unary_op(const std::string &op)
{
    return op == "-" || op == "!";
}

// ---------------------------------------------------------------------------
// Unparse
// ---------------------------------------------------------------------------

static std::string pad(int level)
{
    return std::string(static_cast<size_t>(level) * 4, ' ');
}

// A child needs parentheses when dropping them would reassociate the tree.
// Every binary operator here is left-associative, so the right child needs
// them at equal precedence and the left child does not.
static std::string nest(const Expr &child, int outer, bool on_right)
{
    int inner = 0;
    if (const Binary *b = std::get_if<Binary>(&child))
        inner = precedence(b->op);

    bool wrap = inner != 0 && (on_right ? inner <= outer : inner < outer);
    return wrap ? "(" + unparse(child) + ")" : unparse(child);
}

std::string unparse(const Type &type)
{
    if (type.is_singleton())
        return "satellite";

    // A spacesuit is user-owned, so §1 makes it bare. It never takes generic
    // arguments, which is what keeps a bare type out of §4's ambiguity.
    if (type.is_spacesuit())
        return type.name;

    std::string out = "satellite." + type.space + "." + type.name;
    if (!type.args.empty()) {
        out += "<";
        for (size_t i = 0; i < type.args.size(); i++) {
            if (i)
                out += ", ";
            out += unparse(type.args[i]);
        }
        out += ">";
    }
    return out;
}

std::string unparse(const Expr &expr)
{
    if (const NumberLit *n = std::get_if<NumberLit>(&expr))
        return n->text;

    if (const StringLit *s = std::get_if<StringLit>(&expr))
        return "\"" + s->source + "\"";

    if (std::holds_alternative<SatelliteLit>(expr))
        return "satellite";

    if (const Name *n = std::get_if<Name>(&expr))
        return n->text;

    if (const Member *m = std::get_if<Member>(&expr))
        return unparse(*m->target) + "." + m->name;

    if (const Call *c = std::get_if<Call>(&expr)) {
        std::string out = unparse(*c->target) + "(";
        for (size_t i = 0; i < c->args.size(); i++) {
            if (i)
                out += ", ";
            out += unparse(*c->args[i]);
        }
        return out + ")";
    }

    if (const Index *i = std::get_if<Index>(&expr))
        return unparse(*i->target) + "[" + unparse(*i->subscript) + "]";

    if (const Slice *s = std::get_if<Slice>(&expr)) {
        std::string out = unparse(*s->target) + "[";
        if (s->lo)
            out += unparse(*s->lo);
        out += ":";
        if (s->hi)
            out += unparse(*s->hi);
        return out + "]";
    }

    if (const Unary *u = std::get_if<Unary>(&expr)) {
        // Unary binds tighter than every binary operator, so a binary operand
        // always needs wrapping.
        const Expr &operand = *u->operand;
        bool wrap = std::holds_alternative<Binary>(operand);
        return u->op + (wrap ? "(" + unparse(operand) + ")" : unparse(operand));
    }

    const Binary &b = std::get<Binary>(expr);
    int here = precedence(b.op);
    return nest(*b.left, here, false) + " " + b.op + " " +
           nest(*b.right, here, true);
}

std::string unparse(const Stmt &stmt, int level)
{
    const std::string lead = pad(level);

    if (const Block *b = std::get_if<Block>(&stmt)) {
        std::string out = lead + "{\n";
        for (const StmtPtr &s : b->statements)
            out += unparse(*s, level + 1) + "\n";
        return out + lead + "}";
    }

    if (const VarDecl *d = std::get_if<VarDecl>(&stmt)) {
        std::string out = lead + unparse(d->type) + " " + d->name;
        if (d->init)
            out += " = " + unparse(*d->init);
        return out;
    }

    if (const Assign *a = std::get_if<Assign>(&stmt))
        return lead + unparse(*a->target) + " = " + unparse(*a->value);

    if (const ExprStmt *e = std::get_if<ExprStmt>(&stmt))
        return lead + unparse(*e->expr);

    if (const Return *r = std::get_if<Return>(&stmt)) {
        std::string out = lead + "satellite.return(";
        if (r->value)
            out += unparse(*r->value);
        return out + ")";
    }

    if (const If *i = std::get_if<If>(&stmt)) {
        std::string out = lead + KW_IF + "(" + unparse(*i->condition) + ")\n";
        out += unparse(*i->then_branch, level);
        if (i->else_branch)
            out += "\n" + lead + KW_ELSE + "\n" +
                   unparse(*i->else_branch, level);
        return out;
    }

    if (const While *w = std::get_if<While>(&stmt))
        return lead + KW_WHILE + "(" + unparse(*w->condition) + ")\n" +
               unparse(*w->body, level);

    const For &f = std::get<For>(stmt);
    // The header parts render at level 0 so they carry no indent of their own,
    // which is what lets a VarDecl and an Assign be reused verbatim here.
    std::string out = lead + KW_FOR + "(";
    if (f.init)
        out += unparse(*f.init, 0);
    out += "; ";
    if (f.condition)
        out += unparse(*f.condition);
    out += "; ";
    if (f.step)
        out += unparse(*f.step, 0);
    return out + ")\n" + unparse(*f.body, level);
}

std::string unparse(const Include &include)
{
    return "satellite.include(" + unparse(*include.what) + ")";
}

std::string unparse(const Capsule &capsule, int level)
{
    std::string out = pad(level) + "satellite.capsule ";
    out += capsule.reserved ? "satellite." + capsule.name : capsule.name;
    out += "(";
    for (size_t i = 0; i < capsule.params.size(); i++) {
        if (i)
            out += ", ";
        out += unparse(capsule.params[i].type) + " " + capsule.params[i].name;
    }
    out += ")";
    if (capsule.returns)
        out += " satellite.returns(" + unparse(*capsule.returns) + ")";
    return out + "\n" + unparse(*capsule.body, level);
}

std::string unparse(const Field &field, int level)
{
    std::string out = pad(level) + unparse(field.type) + " " + field.name;
    if (field.init)
        out += " = " + unparse(*field.init);
    return out;
}

static Access access_of(const SuitItem &item)
{
    if (const Field *f = std::get_if<Field>(&item))
        return f->access;
    return std::get<Method>(item).access;
}

std::string unparse(const Spacesuit &suit)
{
    std::string out = "satellite.spacesuit " + suit.name + "(";
    out += suit.super + ")\n{\n";

    // Consecutive members of one access share one block, which is what makes
    // the two-block form in the design round-trip. Emitting a block per member
    // would also be legal source and would not.
    for (size_t i = 0; i < suit.items.size();) {
        const Access access = access_of(suit.items[i]);
        out += pad(1) + access_keyword(access) + "\n" + pad(1) + "{\n";

        for (; i < suit.items.size() && access_of(suit.items[i]) == access; i++) {
            if (const Field *f = std::get_if<Field>(&suit.items[i])) {
                out += unparse(*f, 2) + "\n";
                continue;
            }
            const Method &method = std::get<Method>(suit.items[i]);
            if (!method.constructor) {
                out += unparse(method.capsule, 2) + "\n";
                continue;
            }
            // A constructor wears the spacesuit's name with no keyword in
            // front of it, which is the whole of how it is recognised.
            std::string line = pad(2) + method.capsule.name + "(";
            for (size_t p = 0; p < method.capsule.params.size(); p++) {
                if (p)
                    line += ", ";
                line += unparse(method.capsule.params[p].type) + " " +
                        method.capsule.params[p].name;
            }
            out += line + ")\n" + unparse(*method.capsule.body, 2) + "\n";
        }
        out += pad(1) + "}\n";
    }
    return out + "}";
}

std::string unparse(const TopLevel &item)
{
    if (const Include *i = std::get_if<Include>(&item))
        return unparse(*i);
    if (const Capsule *c = std::get_if<Capsule>(&item))
        return unparse(*c);
    if (const Spacesuit *s = std::get_if<Spacesuit>(&item))
        return unparse(*s);
    return unparse(*std::get<StmtPtr>(item), 0);
}

std::string unparse(const Program &program)
{
    std::string out;
    for (const TopLevel &item : program.items) {
        if (!out.empty())
            out += "\n";
        out += unparse(item) + "\n";
    }
    return out;
}

} // namespace satellite
