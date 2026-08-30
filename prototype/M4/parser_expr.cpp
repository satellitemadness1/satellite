// Expression parsing: precedence climbing, unary, postfix, primary -- Milestone 4 Prototype.

#include "parser_internal.hpp"

#include <string>
#include <utility>
#include <vector>

namespace satellite {

NodeIndex Parser::parse_expression(int min_prec)
{
    NodeIndex left = parse_unary();
    if (left == kNullNode)
        return kNullNode;

    for (;;) {
        if (peek().kind != TokenKind::Punct)
            break;
        const int prec = precedence(peek().text);
        if (prec == 0 || prec < min_prec)
            break;

        const std::string op = advance().text;
        // Binary operators are left-associative, so the right side binds tighter
        NodeIndex right = parse_expression(prec + 1);
        if (right == kNullNode)
            return kNullNode;

        Span s = span_join(arena_.get(left).span, arena_.get(right).span);
        left = arena_.make_binary(op, left, right, s);
    }
    return left;
}

NodeIndex Parser::parse_unary()
{
    if (peek().kind == TokenKind::Punct && is_unary_op(peek().text)) {
        const size_t first = pos_;
        const std::string op = advance().text;
        NodeIndex operand = parse_unary();
        if (operand == kNullNode)
            return kNullNode;
        return arena_.make_unary(op, operand, span_from(first));
    }
    return parse_postfix();
}

NodeIndex Parser::parse_postfix()
{
    const size_t first = pos_;
    NodeIndex node = parse_primary();
    if (node == kNullNode)
        return kNullNode;

    for (;;) {
        if (match_punct(".")) {
            std::string name = expect_word("a member name after '.'");
            if (panic_)
                return kNullNode;
            node = arena_.make_member(node, std::move(name), span_from(first));
            continue;
        }

        // Postfix '(' and '[' must be on the same line as the receiver (DESIGN §6.2)
        const bool same_line = (peek().line == previous().line);

        if (same_line && match_punct("(")) {
            std::vector<NodeIndex> args;
            if (!is_punct(0, ")")) {
                do {
                    // Named argument: Word '=' expr
                    if (peek(0).kind == TokenKind::Word && is_punct(1, "=")) {
                        const size_t arg_first = pos_;
                        std::string name = advance().text;
                        advance(); // =
                        NodeIndex val = parse_expression();
                        if (val == kNullNode)
                            return kNullNode;
                        args.push_back(arena_.make_named_arg(std::move(name), val, span_from(arg_first)));
                        continue;
                    }

                    NodeIndex arg = parse_expression();
                    if (arg == kNullNode)
                        return kNullNode;
                    args.push_back(arg);
                } while (match_punct(","));
            }
            if (!expect_punct(")", "to close argument list"))
                return kNullNode;
            node = arena_.make_call(node, std::move(args), span_from(first));
            continue;
        }

        if (same_line && match_punct("[")) {
            node = parse_subscript(node, first);
            if (node == kNullNode)
                return kNullNode;
            continue;
        }

        return node;
    }
}

NodeIndex Parser::parse_subscript(NodeIndex target, size_t first)
{
    NodeIndex lo = kNullNode;
    if (!is_punct(0, ":")) {
        lo = parse_expression();
        if (lo == kNullNode)
            return kNullNode;
    }

    if (match_punct(":")) {
        NodeIndex hi = kNullNode;
        if (!is_punct(0, "]")) {
            hi = parse_expression();
            if (hi == kNullNode)
                return kNullNode;
        }
        if (!expect_punct("]", "to close slice"))
            return kNullNode;
        return arena_.make_slice(target, lo, hi, span_from(first));
    }

    if (lo == kNullNode) {
        error(peek(), "expected an index or a slice inside '[ ]'");
        return kNullNode;
    }
    if (!expect_punct("]", "to close index"))
        return kNullNode;
    return arena_.make_index(target, lo, span_from(first));
}

NodeIndex Parser::parse_primary()
{
    const Token &t = peek();

    if (t.kind == TokenKind::Number) {
        const size_t first = pos_;
        advance();

        // Check for Duration literals (e.g. 100ms, 100 ms)
        const Token &next = peek();
        if (next.kind == TokenKind::Word && next.line == t.line &&
            (next.text == "ms" || next.text == "us" || next.text == "ns" || next.text == "s")) {
            advance();
            uint64_t mult = kNsPerMs;
            if (next.text == "us") mult = kNsPerUs;
            else if (next.text == "ns") mult = 1ULL;
            else if (next.text == "s") mult = kNsPerS;

            uint64_t val = 0;
            try {
                val = std::stoull(t.text) * mult;
            } catch (...) {}
            return arena_.make_duration(t.text + next.text, val, span_from(first));
        }

        // Check for 800x600 style dimension literals (Number + adjacent 'x' Bits/Word)
        if (next.line == t.line && next.start == t.end &&
            (next.kind == TokenKind::Bits || next.kind == TokenKind::Word) &&
            !next.text.empty() && next.text[0] == 'x') {
            advance();
            return arena_.make_number(t.text + next.text, span_from(first));
        }

        return arena_.make_number(t.text, span_of(t));
    }

    if (t.kind == TokenKind::Bits) {
        advance();
        return arena_.make_bits(t.text, t.radix, span_of(t));
    }

    if (t.kind == TokenKind::String) {
        advance();
        return arena_.make_string(t.text, t.str, span_of(t));
    }

    if (t.kind == TokenKind::Word) {
        advance();
        if (t.text == "satellite")
            return arena_.make_satellite(span_of(t));
        return arena_.make_name(t.text, span_of(t));
    }

    if (is_punct(0, "{")) {
        const size_t first = pos_;
        advance();

        std::vector<NodeIndex> elements;
        if (match_punct("}"))
            return arena_.make_list(std::move(elements), span_from(first));

        for (;;) {
            NodeIndex el = parse_expression();
            if (el == kNullNode)
                return kNullNode;
            elements.push_back(el);

            if (match_punct(","))
                continue;

            if (!expect_punct("}", "to close list literal"))
                return kNullNode;
            break;
        }
        return arena_.make_list(std::move(elements), span_from(first));
    }

    if (is_punct(0, "(")) {
        advance();
        NodeIndex inner = parse_expression();
        if (inner == kNullNode)
            return kNullNode;
        if (!expect_punct(")", "to close group"))
            return kNullNode;
        return inner;
    }

    error(t, "expected an expression, found " + describe(t));
    return kNullNode;
}

} // namespace satellite

