// Expressions: precedence climbing, unary, postfix, subscripts, primaries.
//
// Part of parser/, split from a 1022-line parser.cpp.

#include "parser_internal.hpp"

namespace satellite {

ExprPtr Parser::parse_expression(int min_prec)
{
    ExprPtr left = parse_unary();
    if (!left)
        return nullptr;

    for (;;) {
        if (peek().kind != TokenKind::Punct)
            break;
        const int prec = precedence(peek().text);
        if (prec == 0 || prec < min_prec)
            break;

        const std::string op = advance().text;
        // Every binary operator is left-associative, so the right side
        // binds only tighter operators.
        ExprPtr right = parse_expression(prec + 1);
        if (!right)
            return nullptr;

        Span s = span_join(left->span, right->span);
        left = make_expr(Binary{op, std::move(left), std::move(right)}, s);
    }
    return left;
}

ExprPtr Parser::parse_unary()
{
    if (peek().kind == TokenKind::Punct && is_unary_op(peek().text)) {
        const size_t first = pos_;
        const std::string op = advance().text;
        ExprPtr operand = parse_unary();
        if (!operand)
            return nullptr;
        return make_expr(Unary{op, std::move(operand)}, span_from(first));
    }
    return parse_postfix();
}

ExprPtr Parser::parse_postfix()
{
    const size_t first = pos_;
    ExprPtr node = parse_primary();
    if (!node)
        return nullptr;

    for (;;) {
        if (match_punct(".")) {
            std::string name = expect_word("a member name after '.'");
            if (panic_)
                return nullptr;
            node = make_expr(Member{std::move(node), std::move(name)},
                             span_from(first));
            continue;
        }

        // A call or a subscript must open on the same line as the thing it
        // applies to. Without this, a line beginning '(' or '[' is silently
        // absorbed by the line above — the defect that forced JavaScript's
        // automatic-semicolon-insertion rules. Statements here are
        // newline-separated, so this is what keeps them separated.
        const bool same_line = peek().line == previous().line;

        if (same_line && match_punct("(")) {
            std::vector<ExprPtr> args;
            if (!is_punct(0, ")")) {
                do {
                    ExprPtr arg = parse_expression();
                    if (!arg)
                        return nullptr;
                    args.push_back(std::move(arg));
                } while (match_punct(","));
            }
            if (!expect_punct(")", "to close the argument list"))
                return nullptr;
            node = make_expr(Call{std::move(node), std::move(args)},
                             span_from(first));
            continue;
        }

        if (same_line && match_punct("[")) {
            node = parse_subscript(std::move(node), first);
            if (!node)
                return nullptr;
            continue;
        }

        return node;
    }
}

ExprPtr Parser::parse_subscript(ExprPtr target, size_t first)
{
    ExprPtr lo;
    if (!is_punct(0, ":")) {
        lo = parse_expression();
        if (!lo)
            return nullptr;
    }

    if (match_punct(":")) {
        ExprPtr hi;
        if (!is_punct(0, "]")) {
            hi = parse_expression();
            if (!hi)
                return nullptr;
        }
        if (!expect_punct("]", "to close the slice"))
            return nullptr;
        return make_expr(
            Slice{std::move(target), std::move(lo), std::move(hi)},
            span_from(first));
    }

    if (!lo) {
        error(peek(), "expected an index or a slice inside '[ ]'");
        return nullptr;
    }
    if (!expect_punct("]", "to close the index"))
        return nullptr;
    return make_expr(Index{std::move(target), std::move(lo)},
                     span_from(first));
}

ExprPtr Parser::parse_primary()
{
    const Token &t = peek();

    if (t.kind == TokenKind::Number) {
        advance();
        return make_expr(NumberLit{t.number, t.text}, span_of(t));
    }
    if (t.kind == TokenKind::String) {
        advance();
        return make_expr(StringLit{t.str, t.text}, span_of(t));
    }
    if (t.kind == TokenKind::Word) {
        advance();
        // `satellite` is the runtime singleton as a value, which is what
        // makes satellite.time.now() ordinary member access rather than a
        // special path form.
        if (t.text == "satellite")
            return make_expr(SatelliteLit{}, span_of(t));
        return make_expr(Name{t.text}, span_of(t));
    }
    if (is_punct(0, "(")) {
        advance();
        ExprPtr inner = parse_expression();
        if (!inner)
            return nullptr;
        if (!expect_punct(")", "to close the group"))
            return nullptr;
        return inner;
    }

    error(t, "expected an expression, found " + describe(t));
    return nullptr;
}

} // namespace satellite
