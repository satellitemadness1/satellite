// Expressions: precedence climbing, unary, postfix, subscripts, primaries.
//
// Part of src/syntax_parser/, split from a 1022-line parser.cpp.

#include "syntax_parser/parser_internal.hpp"

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
                    // `name = value` — a named argument. A bare word followed
                    // by a single '=' cannot be anything else here: assignment
                    // is a statement in this language, never an expression, so
                    // there is no form this steals. `==` is one token, so a
                    // comparison is not caught by it.
                    if (peek(0).kind == TokenKind::Word && is_punct(1, "=")) {
                        const size_t first = pos_;
                        std::string name = advance().text;
                        advance();  // =
                        ExprPtr value = parse_expression();
                        if (!value)
                            return nullptr;
                        args.push_back(make_expr(
                            NamedArg{std::move(name), std::move(value)},
                            span_from(first)));
                        continue;
                    }

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
        const size_t first = pos_;
        advance();

        // `100ms` and `100 ms`. The lexer stops a number at the first
        // non-digit and a word may not start with a digit, so BOTH spellings
        // arrive here as Number then Word("ms") and one rule covers them --
        // there is no lexer change behind this literal at all.
        //
        // Same line, for the reason a postfix '[' must be: a `ms` alone at the
        // start of the next line is a name, and a rule that reached across the
        // newline would swallow it. Nothing that parsed before parses
        // differently now either way -- a number followed by a word was a
        // syntax error in every position the grammar has.
        const Token &unit = peek();
        if (unit.kind == TokenKind::Word && unit.text == "ms" &&
            unit.line == t.line) {
            advance();
            return make_expr(
                DurationLit{Number::mul(t.number, Number(kNsPerMs)),
                            t.text + unit.text},
                span_from(first));
        }

        return make_expr(NumberLit{t.number, t.text}, span_of(t, file_));
    }
    // §21. The lexer has already decided this is a literal and not a name, so
    // there is nothing to disambiguate here: the node carries the spelling and
    // the radix, and the digits are normalised when it is evaluated.
    if (t.kind == TokenKind::Bits) {
        advance();
        return make_expr(BitsLit{t.radix, t.text}, span_of(t, file_));
    }
    if (t.kind == TokenKind::String) {
        advance();
        return make_expr(StringLit{t.str, t.text}, span_of(t, file_));
    }
    if (t.kind == TokenKind::Word) {
        advance();
        // `satellite` is the runtime singleton as a value, which is what
        // makes satellite.time.now() ordinary member access rather than a
        // special path form.
        if (t.text == "satellite")
            return make_expr(SatelliteLit{}, span_of(t, file_));
        return make_expr(Name{t.text}, span_of(t, file_));
    }
    // { a, b, c } — a list literal. Checked before the '(' group below only
    // because it is a literal and groups are not; the two cannot be confused.
    if (is_punct(0, "{")) {
        const size_t first = pos_;
        advance();

        ListLit node;

        // Empty is `{}` and takes the early exit, so the element loop below
        // never has to ask whether it is looking at a first element or at the
        // closing brace.
        if (match_punct("}"))
            return make_expr(std::move(node), span_from(first));

        for (;;) {
            ExprPtr element = parse_expression();
            if (!element)
                return nullptr;
            node.elements.push_back(std::move(element));

            if (match_punct(","))
                continue;

            // Not a comma, so this is the end of the list — and if it is not a
            // brace either, say which of the two was expected rather than
            // reporting whatever token happens to be sitting there. A missing
            // comma between two elements is the mistake this catches.
            if (!expect_punct("}", "to close the list, or ',' before the next "
                                   "element"))
                return nullptr;
            break;
        }

        return make_expr(std::move(node), span_from(first));
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
