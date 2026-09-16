// Parser statements and types -- Milestone 4 Prototype.

#include "parser_internal.hpp"

#include <utility>
#include <vector>

namespace satellite {

namespace {
constexpr size_t kMaxErrors = 50;
}

Type Parser::parse_type()
{
    const size_t first = pos_;
    Type type;

    if (!is_word(0, "satellite")) {
        if (peek().kind != TokenKind::Word) {
            error(peek(), "expected a type name");
            return type;
        }
        type.name = advance().text;
        type.span = span_from(first);
        return type;
    }

    advance(); // satellite
    if (!is_punct(0, ".")) {
        type.span = span_from(first);
        return type; // bare singleton `satellite`
    }

    advance(); // .
    type.space = expect_word("'variable' or 'container'");
    expect_punct(".", "after type space");
    type.name = expect_word("a type name");

    if (match_punct("<")) {
        do {
            type.args.push_back(parse_type());
            if (panic_)
                return type;
        } while (match_punct(","));

        if (is_punct(0, ">="))
            split_punct(toks_, pos_);
        expect_punct(">", "to close generic arguments");
    }

    type.span = span_from(first);
    return type;
}

NodeIndex Parser::parse_block()
{
    while (!at_end() && peek().kind == TokenKind::Newline)
        advance();

    const size_t first = pos_;
    if (!expect_punct("{", "to open a block"))
        return kNullNode;

    std::vector<NodeIndex> stmts;
    while (!at_end() && !is_punct(0, "}")) {
        while (!at_end() && (peek().kind == TokenKind::Newline || is_punct(0, ";")))
            advance();
        if (at_end() || is_punct(0, "}"))
            break;

        const size_t before = pos_;
        NodeIndex s = parse_statement();
        if (s != kNullNode)
            stmts.push_back(s);
        if (pos_ == before)
            advance();
        if (panic_)
            synchronize();
        if (errors_.size() >= kMaxErrors)
            break;
    }
    expect_punct("}", "to close the block");
    return arena_.make_block(std::move(stmts), span_from(first));
}

NodeIndex Parser::parse_statement()
{
    while (!at_end() && (peek().kind == TokenKind::Newline || is_punct(0, ";")))
        advance();

    if (is_punct(0, "{"))
        return parse_block();
    if (at_statement_keyword("if"))
        return parse_if();
    if (at_statement_keyword("while"))
        return parse_while();
    if (at_statement_keyword("for"))
        return parse_for();

    NodeIndex s = at_language_path("return") ? parse_return() : parse_simple_statement();
    if (s != kNullNode)
        expect_statement_end();
    return s;
}

NodeIndex Parser::parse_simple_statement()
{
    const size_t first = pos_;

    if (at_type()) {
        Type type = parse_type();
        if (panic_)
            return kNullNode;

        std::string name = expect_word("a variable name after type");
        if (panic_)
            return kNullNode;

        NodeIndex init = kNullNode;
        if (is_punct(0, "(") && peek().line == previous().line && type.is_spacesuit()) {
            // Desugar constructor call: `my_suit x(arg)` -> `x = my_suit(arg)`
            const size_t call_first = pos_;
            NodeIndex callee = arena_.make_name(type.name, type.span);
            advance(); // (
            std::vector<NodeIndex> args;
            if (!is_punct(0, ")")) {
                do {
                    args.push_back(parse_expression());
                } while (match_punct(","));
            }
            expect_punct(")", "to close constructor arguments");
            init = arena_.make_call(callee, std::move(args), span_from(call_first));
        } else if (match_punct("=")) {
            init = parse_expression();
        }
        return arena_.make_var_decl(std::move(type), std::move(name), init, span_from(first));
    }

    NodeIndex target = parse_expression();
    if (target == kNullNode)
        return kNullNode;

    if (match_punct("=")) {
        NodeIndex value = parse_expression();
        return arena_.make_assign(target, value, span_from(first));
    }
    return arena_.make_expr_stmt(target, span_from(first));
}

NodeIndex Parser::parse_if()
{
    const size_t first = pos_;
    take_statement_keyword();

    if (!expect_punct("(", "after satellite.statement.if"))
        return kNullNode;
    NodeIndex condition = parse_expression();
    expect_punct(")", "after the condition");

    NodeIndex then_branch = parse_block();
    NodeIndex else_branch = kNullNode;

    // Skip any newlines before else
    while (!at_end() && peek().kind == TokenKind::Newline)
        advance();

    if (at_statement_keyword("else")) {
        take_statement_keyword();
        if (is_punct(0, "(") && is_punct(1, ")")) {
            advance(); advance();
        }
        while (!at_end() && peek().kind == TokenKind::Newline)
            advance();
        else_branch = at_statement_keyword("if") ? parse_if() : parse_block();
    }

    return arena_.make_if(condition, then_branch, else_branch, span_from(first));
}

NodeIndex Parser::parse_while()
{
    const size_t first = pos_;
    take_statement_keyword();

    if (!expect_punct("(", "after satellite.statement.while"))
        return kNullNode;
    NodeIndex condition = parse_expression();
    expect_punct(")", "after the condition");

    NodeIndex body = parse_block();
    return arena_.make_while(condition, body, span_from(first));
}

NodeIndex Parser::parse_for()
{
    const size_t first = pos_;
    take_statement_keyword();

    if (!expect_punct("(", "after satellite.statement.for"))
        return kNullNode;

    NodeIndex init = kNullNode;
    if (!is_punct(0, ";"))
        init = parse_simple_statement();
    expect_punct(";", "after for-loop initialiser");

    NodeIndex condition = kNullNode;
    if (!is_punct(0, ";"))
        condition = parse_expression();
    expect_punct(";", "after for-loop condition");

    NodeIndex step = kNullNode;
    if (!is_punct(0, ")"))
        step = parse_simple_statement();
    expect_punct(")", "to close for-loop header");

    NodeIndex body = parse_block();
    return arena_.make_for(init, condition, step, body, span_from(first));
}

NodeIndex Parser::parse_return()
{
    const size_t first = pos_;
    advance(); // satellite
    advance(); // .
    advance(); // return

    if (!expect_punct("(", "after satellite.return"))
        return kNullNode;

    NodeIndex value = kNullNode;
    if (!is_punct(0, ")"))
        value = parse_expression();
    expect_punct(")", "to close satellite.return");

    return arena_.make_return(value, span_from(first));
}

} // namespace satellite

