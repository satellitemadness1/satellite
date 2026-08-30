// Statements: the §6.1 dispatch, blocks, and the four statement forms that
// have a keyword. See parser/parser.hpp for what the parser promises.
//
// THIS FILE IS WHERE DESIGN §6.1 EARNS ITS PAGE. The rule it refuses --
// "a dotted path followed by a bare word is a declaration" -- is the one any
// parser writes first, and it is wrong in a way no test of a working program
// finds: `satellite.control.return my_time` and `satellite.variable.time
// my_time` are the same four tokens, so the structural rule declares a variable
// of type `satellite.control.return` and says nothing. What decides instead is
// the word at segment 1, and after DESIGN §4 that word is an integer the lexer
// already interned -- so the whole of the collision costs one switch.

#include "parser/parser_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace satellite {

namespace {

// Whether a node names somewhere a value can be written.
//
// DESIGN §6's `assign := postfix "=" expression` is the whole rule, and the
// point of checking it here rather than letting M6 find out is §6.4's: a
// mutating method needs a receiver that names a storage slot, and `foo() = x`
// has nowhere to write back. A Binary or a Call on the left is a program that
// cannot mean anything, and the earliest place it can be said so is here.
bool names_a_place(NodeKind kind)
{
    return kind == NodeKind::Name || kind == NodeKind::Member ||
           kind == NodeKind::Index || kind == NodeKind::Slice;
}

} // namespace

NodeIndex Parser::statement()
{
    const NodeIndex node = [&]() -> NodeIndex {
        switch (opening()) {
        case Segment1::Variable:
        case Segment1::Container: {
            // A TYPE PATH, SO A FOLLOWING BARE WORD IS A DECLARATION -- and
            // nothing else in this switch may reach that conclusion.
            const NodeIndex declared = type();
            if (declared == kNoNode)
                return kNoNode;
            return var_decl(declared);
        }
        case Segment1::Statement:
            // `if`, `else`, `while`, `for` -- 1 13 1 to 1 13 4.
            if (!at_punct(".", 3) || !at_word(4)) {
                error<errors::Code::PARSE_STATEMENT_NEEDS_A_WORD>(here());
                return kNoNode;
            }
            switch (peek(4).spelling) {
            case words::spelling_id(words::NodeId::STATEMENT_IF):
                return if_stmt();
            case words::spelling_id(words::NodeId::STATEMENT_WHILE):
                return while_stmt();
            case words::spelling_id(words::NodeId::STATEMENT_FOR):
                return for_stmt();
            case words::spelling_id(words::NodeId::STATEMENT_ELSE):
                // NAMED RATHER THAN LUMPED IN WITH "no such statement",
                // because a stray `else` is a real thing a person writes and
                // the useful sentence is about the `if` and not about the word.
                error<errors::Code::PARSE_ELSE_WITHOUT_IF>(here() + 4);
                return kNoNode;
            default:
                // DESIGN §4.6's OWN WORKED EXAMPLE, one node further down the
                // trie than the one it uses. `satellite.statement.wihle` is not
                // a statement; `satellite.statement`'s four children are the
                // candidate list, and suggest.cpp's transposition arm is what
                // makes the answer `while` rather than nothing.
                error<errors::Code::PARSE_NO_SUCH_STATEMENT>(here() + 4,
                                                             peek(4).text);
                suggest(here() + 4,
                        static_cast<words::PathId>(words::NodeId::STATEMENT));
                return kNoNode;
            }
        case Segment1::Return:
            return return_stmt();
        case Segment1::Include:
        case Segment1::Capsule:
        case Segment1::Spacesuit:
        case Segment1::Returns:
        case Segment1::Protected:
        case Segment1::Public:
            // DESIGN §6 puts all six under `top_level` or inside a suit block,
            // never inside a `block`. Saying which one was written is what
            // makes this better than "unexpected token": the person wrote a
            // real word of the language in a place it does not go.
            error<errors::Code::PARSE_DECLARATION_IN_BLOCK>(here() + 2,
                                                            peek(2).text);
            return kNoNode;
        case Segment1::Library:
        case Segment1::None:
            // A value path or a module path. Both are expressions, and DESIGN
            // §6.3 says the parser does not resolve either.
            break;
        }

        if (at_punct("{"))
            return block();

        if (at_declaration()) {
            const NodeIndex declared = type();
            if (declared == kNoNode)
                return kNoNode;
            return var_decl(declared);
        }

        return assign_or_expression();
    }();

    // THE FOUR COMPOUND FORMS END WITH A BRACE AND NOT WITH A NEWLINE, so they
    // are the ones this must not ask a terminator of. Written as one test over
    // the node that came back rather than as a call at the end of each simple
    // rule, because the rule that forgets it is the one nobody notices: a
    // missing terminator check does not fail, it silently lets two statements
    // share a line.
    if (node != kNoNode) {
        const NodeKind kind = ast_[node].kind;
        if (kind != NodeKind::If && kind != NodeKind::While &&
            kind != NodeKind::For && kind != NodeKind::Block)
            end_of_statement();
    }
    return node;
}

// TWO ADJACENT WORD TOKENS WITH NOTHING BETWEEN THEM -- DESIGN §6.1's own
// words, and "nothing between them" is doing more work here than it looks.
// A newline is a token in this stream (DESIGN §5.6), so two words on two lines
// are not adjacent and this test needs no comparison of `line` to say so. The
// design states the rule about whitespace being load-bearing; the token stream
// is where it became structural.
bool Parser::at_declaration() const
{
    return at_word(0) && at_word(1);
}

NodeIndex Parser::block()
{
    // THE BRACE MAY BE ON ITS OWN LINE, and every program in example/ writes it
    // that way -- `satellite.capsule satellite.main(...)` then `{` on the next
    // line. A block is never a statement's whole line, so crossing the newline
    // before one cannot swallow anything: what precedes it is always a header
    // that is not finished.
    skip_newlines();
    const uint32_t opener = here();
    if (!expect_punct("{", "to open a block"))
        return kNoNode;

    std::vector<NodeIndex> statements;
    skip_newlines();
    while (!at_end() && !at_punct("}") && !stop()) {
        const size_t before = pos_;
        const NodeIndex node = statement();
        if (node != kNoNode)
            statements.push_back(node);
        if (pos_ == before) {
            error<errors::Code::PARSE_EXPECTED_STATEMENT>(here(), describe(peek()));
            advance();
        }
        if (panic_)
            synchronise();
        skip_newlines();
    }

    if (!expect_punct("}", "to close the block", opener))
        return kNoNode;
    return ast_.add(NodeKind::Block, opener, ast_.add_list(statements));
}

NodeIndex Parser::var_decl(NodeIndex declared_type)
{
    const uint32_t name = expect_word("a name for the variable");
    if (panic_)
        return kNoNode;

    // A LOCAL'S NAME TAKES NO NUMBER, and that is not an omission. WORD_NUMBERS
    // §3 numbers user capsules and spacesuits, because those need identity
    // across a program; a local needs a FRAME SLOT, which DESIGN §7.2 resolves
    // to an integer index in M6's pass and which is per call rather than per
    // program. Numbering locals here would put one slot per local per program
    // back -- which is §7.1's verified blocker, where a recursive `fact`
    // returns 1 for every input.
    NodeIndex init = kNoNode;
    if (take_punct("=")) {
        init = expression();
        if (init == kNoNode)
            return kNoNode;
    }
    return ast_.add(NodeKind::VarDecl, name, declared_type, init);
}

NodeIndex Parser::return_stmt()
{
    const uint32_t at = here();
    advance();  // satellite
    advance();  // .
    advance();  // return

    const uint32_t opener = here();
    if (!expect_punct("(", "after satellite.return"))
        return kNoNode;
    open_bracket();

    NodeIndex value = kNoNode;
    if (!at_punct(")")) {
        value = expression();
        if (value == kNoNode) {
            close_bracket();
            return kNoNode;
        }
    }

    close_bracket();
    if (!expect_punct(")", "to close satellite.return", opener))
        return kNoNode;
    return ast_.add(NodeKind::Return, at, value);
}

NodeIndex Parser::assign_or_expression()
{
    const NodeIndex left = expression();
    if (left == kNoNode)
        return kNoNode;

    if (!at_punct("="))
        return ast_.add(NodeKind::ExprStmt, ast_[left].token, left);

    const uint32_t op = here();
    if (!names_a_place(ast_[left].kind)) {
        error<errors::Code::PARSE_ASSIGN_TARGET>(op, kind_name(ast_[left].kind));
        return kNoNode;
    }
    advance();

    const NodeIndex value = expression();
    if (value == kNoNode)
        return kNoNode;
    return ast_.add(NodeKind::Assign, op, left, value);
}

} // namespace satellite
