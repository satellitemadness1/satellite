// The three statements with a parenthesised head and a block: if, while, for.
// See parser/parser.hpp for what the parser promises.
//
// SPLIT FROM parser_statements.cpp BY SUBJECT AND THE SUBJECT IS REAL: these
// three are DESIGN §6's `stmt_kw` forms, they share an opening down to the
// token, and they are the only statements whose body is a block. What is left
// next door is the dispatch and the statements that end at a newline.
//
// THE KEYWORD IS FIVE TOKENS AND THEY ARE COUNTED RATHER THAN MATCHED --
// `satellite` `.` `statement` `.` `if`. The caller has already decided which
// form this is, by the one integer compare DESIGN §6.1 asks for, so re-testing
// the shape here would be a second place it lives and a second place it could
// come to disagree with words.def.

#include "parser/parser_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <vector>

namespace satellite {

uint32_t Parser::take_statement_keyword()
{
    const uint32_t at = here();
    for (int i = 0; i < 5; i++)
        advance();
    return at;
}

NodeIndex Parser::condition(const char *after)
{
    const uint32_t opener = here();
    if (!expect_punct("(", after))
        return kNoNode;
    open_bracket();
    const NodeIndex node = expression();
    close_bracket();
    if (node == kNoNode)
        return kNoNode;
    if (!expect_punct(")", "to close the condition", opener))
        return kNoNode;
    return node;
}

bool Parser::at_else() const
{
    return opening() == Segment1::Statement && at_punct(".", 3) && at_word(4) &&
           peek(4).spelling == words::spelling_id(words::NodeId::STATEMENT_ELSE);
}

NodeIndex Parser::if_stmt()
{
    const uint32_t at = take_statement_keyword();

    const NodeIndex test = condition("after satellite.statement.if");
    if (test == kNoNode)
        return kNoNode;

    const NodeIndex then_block = block();
    if (then_block == kNoNode)
        return kNoNode;

    // THE ELSE MAY BE ON ITS OWN LINE, so the newlines after the closing brace
    // are crossed to look for it -- and the cursor is put back when there is no
    // else, because those newlines belong to whatever comes next. Crossing them
    // without a way back is how an `if` at the end of a block eats the brace
    // that closes it.
    const size_t before = pos_;
    skip_newlines();
    if (!at_else()) {
        pos_ = before;
        return ast_.add(NodeKind::If, at, test, then_block, kNoNode);
    }
    take_statement_keyword();
    skip_newlines();

    // `( block | if_stmt )`, which is what keeps `else if` from being a form of
    // its own: the second arm is this rule again, and the tree that comes out
    // of `else if` is an If in an If.
    NodeIndex otherwise = kNoNode;
    if (at_punct("{"))
        otherwise = block();
    else if (opening() == Segment1::Statement)
        otherwise = statement();
    else {
        error<errors::Code::PARSE_ELSE_NEEDS_A_BLOCK>(here(), describe(peek()));
        return kNoNode;
    }
    if (otherwise == kNoNode)
        return kNoNode;

    return ast_.add(NodeKind::If, at, test, then_block, otherwise);
}

NodeIndex Parser::while_stmt()
{
    const uint32_t at = take_statement_keyword();

    const NodeIndex test = condition("after satellite.statement.while");
    if (test == kNoNode)
        return kNoNode;

    const NodeIndex body = block();
    if (body == kNoNode)
        return kNoNode;
    return ast_.add(NodeKind::While, at, test, body);
}

NodeIndex Parser::for_stmt()
{
    const uint32_t at = take_statement_keyword();

    const uint32_t opener = here();
    if (!expect_punct("(", "after satellite.statement.for"))
        return kNoNode;
    open_bracket();

    // ALL THREE PARTS ARE OPTIONAL AND THE TWO SEMICOLONS ARE NOT. `for (;;)`
    // is the shape that says so, and it is why each semicolon is expected after
    // its part rather than as part of it.
    NodeIndex init = kNoNode;
    if (!at_punct(";")) {
        const Segment1 word = opening();
        if (word == Segment1::Variable || word == Segment1::Container ||
            at_declaration()) {
            const NodeIndex declared = type();
            init = declared == kNoNode ? kNoNode : var_decl(declared);
        } else {
            init = assign_or_expression();
        }
        if (init == kNoNode) {
            close_bracket();
            return kNoNode;
        }
    }
    if (!expect_punct(";", "after the for loop's first part")) {
        close_bracket();
        return kNoNode;
    }

    NodeIndex test = kNoNode;
    if (!at_punct(";")) {
        test = expression();
        if (test == kNoNode) {
            close_bracket();
            return kNoNode;
        }
    }
    if (!expect_punct(";", "after the for loop's condition")) {
        close_bracket();
        return kNoNode;
    }

    NodeIndex step = kNoNode;
    if (!at_punct(")")) {
        step = assign_or_expression();
        if (step == kNoNode) {
            close_bracket();
            return kNoNode;
        }
    }

    close_bracket();
    if (!expect_punct(")", "to close the for loop's head", opener))
        return kNoNode;

    const NodeIndex body = block();
    if (body == kNoNode)
        return kNoNode;
    return ast_.add(NodeKind::For, at, init, test, step, body);
}

} // namespace satellite
