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

// THE HEAD ONLY, AND THE BODY IS THE BLOCK THE MACHINE OPENS NEXT -- M8.5.
// Each of these was a whole statement that called block() in the middle of
// itself, which is the statement cycle DESIGN §7.5 does not allow on the C++
// stack. What is left is the part that reads tokens; parser_statements.cpp's
// loop pushes the block after it and builds the node when that block closes.
// `false` means nothing was pushed and the statement is kNoNode.
bool Parser::if_head()
{
    const uint32_t at = take_statement_keyword();

    const NodeIndex test = condition("after satellite.statement.if");
    if (test == kNoNode)
        return false;

    Open frame;
    frame.kind = Open::Kind::If;
    frame.at = at;
    frame.a = test;
    open_.push_back(std::move(frame));
    return true;
}

bool Parser::while_head()
{
    const uint32_t at = take_statement_keyword();

    const NodeIndex test = condition("after satellite.statement.while");
    if (test == kNoNode)
        return false;

    Open frame;
    frame.kind = Open::Kind::While;
    frame.at = at;
    frame.a = test;
    open_.push_back(std::move(frame));
    return true;
}

bool Parser::for_head()
{
    const uint32_t at = take_statement_keyword();

    const uint32_t opener = here();
    if (!expect_punct("(", "after satellite.statement.for"))
        return false;
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
            return false;
        }
    }
    if (!expect_punct(";", "after the for loop's first part")) {
        close_bracket();
        return false;
    }

    NodeIndex test = kNoNode;
    if (!at_punct(";")) {
        test = expression();
        if (test == kNoNode) {
            close_bracket();
            return false;
        }
    }
    if (!expect_punct(";", "after the for loop's condition")) {
        close_bracket();
        return false;
    }

    NodeIndex step = kNoNode;
    if (!at_punct(")")) {
        step = assign_or_expression();
        if (step == kNoNode) {
            close_bracket();
            return false;
        }
    }

    close_bracket();
    if (!expect_punct(")", "to close the for loop's head", opener))
        return false;

    Open frame;
    frame.kind = Open::Kind::For;
    frame.at = at;
    frame.a = init;
    frame.b = test;
    frame.c = step;
    open_.push_back(std::move(frame));
    return true;
}

} // namespace satellite
