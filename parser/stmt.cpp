// Statements and the four control-flow forms.
//
// Part of parser/, split from a 1022-line parser.cpp.

#include "parser_internal.hpp"

namespace satellite {

StmtPtr Parser::parse_block()
{
    const size_t first = pos_;
    if (!expect_punct("{", "to open a block"))
        return nullptr;

    Block block;
    while (!at_end() && !is_punct(0, "}")) {
        const size_t before = pos_;
        StmtPtr s = parse_statement();
        if (s)
            block.statements.push_back(std::move(s));
        if (panic_)
            synchronize();
        if (pos_ == before)
            advance();
        if (errors_.size() >= MAX_ERRORS)
            break;
    }
    if (!expect_punct("}", "to close the block"))
        return nullptr;
    return make_stmt(std::move(block), span_from(first));
}

StmtPtr Parser::parse_statement()
{
    if (is_punct(0, "{"))
        return parse_block();
    if (at_statement_keyword("if"))
        return parse_if();
    if (at_statement_keyword("while"))
        return parse_while();
    if (at_statement_keyword("for"))
        return parse_for();

    StmtPtr s = at_language_path("return") ? parse_return()
                                           : parse_simple_statement();
    if (s)
        expect_statement_end();
    return s;
}

StmtPtr Parser::parse_simple_statement()
{
    const size_t first = pos_;

    if (at_type()) {
        Type type = parse_type();
        if (panic_)
            return nullptr;

        // The name must sit on the same line as its type. Without this a
        // truncated declaration reaches onto the next line and swallows
        // whatever starts it, so one bad line takes the following capsule
        // down with it.
        if (peek().kind != TokenKind::Word ||
            peek().line != previous().line) {
            error(peek(), "expected a variable name after the type");
            return nullptr;
        }
        std::string name = advance().text;

        if (name == "satellite") {
            error(previous(),
                  "'satellite' is reserved as the language root and "
                  "cannot name a variable");
            return nullptr;
        }
        ExprPtr init;

        // `my_suit x("data")` — constructor arguments at the declaration.
        //
        // SUGAR, desugared here rather than carried in the tree: it becomes
        // `my_suit x = my_suit("data")`, which is the same statement with
        // the construction written out. Doing it in the parser means the
        // resolver's arity check, the type check and the evaluator all see
        // one form instead of two, and the only thing lost is that the
        // sugar is not what unparse emits — which is what "canonical" means.
        //
        // Same line, like every other postfix '(': a '(' starting the next
        // line belongs to that line.
        if (is_punct(0, "(") && peek().line == previous().line) {
            if (!type.is_spacesuit()) {
                error(peek(), "only a spacesuit takes constructor "
                              "arguments; " + unparse(type) +
                              " is initialised with '='");
                return nullptr;
            }
            const size_t call_first = pos_;
            ExprPtr callee = make_expr(Name{type.name}, type.span);
            advance(); // (

            std::vector<ExprPtr> args;
            if (!is_punct(0, ")")) {
                do {
                    ExprPtr arg = parse_expression();
                    if (!arg)
                        return nullptr;
                    args.push_back(std::move(arg));
                } while (match_punct(","));
            }
            if (!expect_punct(")", "to close the constructor arguments"))
                return nullptr;

            init = make_expr(Call{std::move(callee), std::move(args)},
                             span_from(call_first));
        } else if (match_punct("=")) {
            init = parse_expression();
            if (!init)
                return nullptr;
        }
        return make_stmt(
            VarDecl{std::move(type), std::move(name), std::move(init)},
            span_from(first));
    }

    ExprPtr target = parse_expression();
    if (!target)
        return nullptr;

    if (match_punct("=")) {
        ExprPtr value = parse_expression();
        if (!value)
            return nullptr;
        return make_stmt(Assign{std::move(target), std::move(value)},
                         span_from(first));
    }
    return make_stmt(ExprStmt{std::move(target)}, span_from(first));
}

StmtPtr Parser::parse_if()
{
    const size_t first = pos_;
    take_statement_keyword();

    if (!expect_punct("(", "after satellite.statement.if"))
        return nullptr;
    ExprPtr condition = parse_expression();
    if (!condition)
        return nullptr;
    if (!expect_punct(")", "after the condition"))
        return nullptr;

    StmtPtr then_branch = parse_block();
    if (!then_branch)
        return nullptr;

    StmtPtr else_branch;
    if (at_statement_keyword("else")) {
        take_statement_keyword();
        // else-if chains: the else branch may be another if.
        else_branch = at_statement_keyword("if") ? parse_if() : parse_block();
        if (!else_branch)
            return nullptr;
    }

    return make_stmt(If{std::move(condition), std::move(then_branch),
                        std::move(else_branch)},
                     span_from(first));
}

StmtPtr Parser::parse_while()
{
    const size_t first = pos_;
    take_statement_keyword();

    if (!expect_punct("(", "after satellite.statement.while"))
        return nullptr;
    ExprPtr condition = parse_expression();
    if (!condition)
        return nullptr;
    if (!expect_punct(")", "after the condition"))
        return nullptr;

    StmtPtr body = parse_block();
    if (!body)
        return nullptr;
    return make_stmt(While{std::move(condition), std::move(body)},
                     span_from(first));
}

StmtPtr Parser::parse_for()
{
    const size_t first = pos_;
    take_statement_keyword();

    if (!expect_punct("(", "after satellite.statement.for"))
        return nullptr;

    // Each of the three parts may be empty, so for(;;) is the infinite
    // loop. init and step are ordinary statements, which is why the loop
    // needs no forms the language did not already have.
    StmtPtr init;
    if (!is_punct(0, ";")) {
        init = parse_simple_statement();
        if (!init)
            return nullptr;
    }
    if (!expect_punct(";", "after the for-loop initialiser"))
        return nullptr;

    ExprPtr condition;
    if (!is_punct(0, ";")) {
        condition = parse_expression();
        if (!condition)
            return nullptr;
    }
    if (!expect_punct(";", "after the for-loop condition"))
        return nullptr;

    StmtPtr step;
    if (!is_punct(0, ")")) {
        step = parse_simple_statement();
        if (!step)
            return nullptr;
    }
    if (!expect_punct(")", "to close the for-loop header"))
        return nullptr;

    StmtPtr body = parse_block();
    if (!body)
        return nullptr;

    return make_stmt(For{std::move(init), std::move(condition),
                         std::move(step), std::move(body)},
                     span_from(first));
}

StmtPtr Parser::parse_return()
{
    const size_t first = pos_;
    advance(); // satellite
    advance(); // .
    advance(); // return

    if (!expect_punct("(", "after satellite.return"))
        return nullptr;

    ExprPtr value;
    if (!is_punct(0, ")")) {
        value = parse_expression();
        if (!value)
            return nullptr;
    }
    if (!expect_punct(")", "to close satellite.return"))
        return nullptr;

    return make_stmt(Return{std::move(value)}, span_from(first));
}

} // namespace satellite
