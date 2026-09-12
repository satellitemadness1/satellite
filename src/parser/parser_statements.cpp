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
#include <utility>
#include <vector>

namespace satellite {

namespace {

// Whether a node names somewhere a value can be written.
//
// DESIGN §6's `assign := postfix "=" expression` is the whole rule, and the
// point of checking it here rather than letting M7 find out is §6.4's: a
// mutating method needs a receiver that names a storage slot, and `foo() = x`
// has nowhere to write back. A Binary or a Call on the left is a program that
// cannot mean anything, and the earliest place it can be said so is here.
bool names_a_place(NodeKind kind)
{
    return kind == NodeKind::Name || kind == NodeKind::Member ||
           kind == NodeKind::Index || kind == NodeKind::Slice;
}

} // namespace

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

bool Parser::open_block()
{
    // THE BRACE MAY BE ON ITS OWN LINE, and every program in example/ writes it
    // that way -- `satellite.capsule satellite.main(...)` then `{` on the next
    // line. A block is never a statement's whole line, so crossing the newline
    // before one cannot swallow anything: what precedes it is always a header
    // that is not finished.
    skip_newlines();
    const uint32_t opener = here();
    if (!expect_punct("{", "to open a block"))
        return false;
    Open frame;
    frame.kind = Open::Kind::Block;
    frame.at = opener;
    open_.push_back(std::move(frame));
    return true;
}

// A block, every statement in it, and everything nested inside those -- one
// loop over `open_` where M4 wrote block() and statement() calling each other.
//
// THREE STEPS, AND THEY ARE THE THREE PLACES THE RECURSION USED TO BE:
//
//   Item      the innermost block decides -- another statement, or its `}`
//   Statement one statement's dispatch. It either finishes, or pushes the
//             frames of a compound form and leaves the body to Item
//   Deliver   a finished node handed to the frame that was waiting for it,
//             which is a block collecting statements or one of the three
//             compound forms collecting the block it opened
//
// `floor` IS WHY A CAPSULE INSIDE A SPACESUIT PARSES. `open_` is a member, so a
// block that runs while some outer parse is part way through must not read the
// frames underneath it -- and the only such parse today is a suit body, which
// holds a capsule, which holds this.
NodeIndex Parser::block()
{
    const size_t floor = open_.size();
    if (!open_block())
        return kNoNode;

    enum class Step : uint8_t { Item, Statement, Deliver };
    Step step = Step::Item;
    NodeIndex value = kNoNode;

    for (;;) {
        switch (step) {

        case Step::Item: {
            skip_newlines();
            if (!at_end() && !at_punct("}") && !stop()) {
                // NO RULE MAY LEAVE THE CURSOR WHERE IT FOUND IT, and each
                // block keeps its own mark because each has its own loop.
                open_.back().before = pos_;
                step = Step::Statement;
                break;
            }
            const Open done = std::move(open_.back());
            open_.pop_back();
            value = expect_punct("}", "to close the block", done.at)
                        ? ast_.add(NodeKind::Block, done.at,
                                   ast_.add_list(done.items))
                        : kNoNode;
            step = Step::Deliver;
            break;
        }

        case Step::Statement: {
            value = kNoNode;
            step = Step::Deliver;

            switch (opening()) {
            case Segment1::Variable:
            case Segment1::Container: {
                // A TYPE PATH, SO A FOLLOWING BARE WORD IS A DECLARATION -- and
                // nothing else in this switch may reach that conclusion.
                const NodeIndex declared = type();
                if (declared == kNoNode)
                    break;
                value = var_decl(declared);
                if (value != kNoNode)
                    end_of_statement();
                break;
            }
            case Segment1::Statement:
                // `if`, `else`, `while`, `for` -- 1 13 1 to 1 13 4.
                if (!at_punct(".", 3) || !at_word(4)) {
                    error<errors::Code::PARSE_STATEMENT_NEEDS_A_WORD>(here());
                    break;
                }
                switch (peek(4).spelling) {
                case words::spelling_id(words::NodeId::STATEMENT_IF):
                    // THE HEAD PUSHES A FRAME AND THE BODY IS THE NEXT BLOCK,
                    // which is what replaces `then_block = block()` inside a
                    // rule that had already been entered. A block that will not
                    // open takes the frame with it, exactly as if_stmt()
                    // returning kNoNode did.
                    if (!if_head())
                        break;
                    if (!open_block()) {
                        open_.pop_back();
                        break;
                    }
                    step = Step::Item;
                    break;
                case words::spelling_id(words::NodeId::STATEMENT_WHILE):
                    if (!while_head())
                        break;
                    if (!open_block()) {
                        open_.pop_back();
                        break;
                    }
                    step = Step::Item;
                    break;
                case words::spelling_id(words::NodeId::STATEMENT_FOR):
                    if (!for_head())
                        break;
                    if (!open_block()) {
                        open_.pop_back();
                        break;
                    }
                    step = Step::Item;
                    break;
                case words::spelling_id(words::NodeId::STATEMENT_ELSE):
                    // NAMED RATHER THAN LUMPED IN WITH "no such statement",
                    // because a stray `else` is a real thing a person writes and
                    // the useful sentence is about the `if` and not about the
                    // word.
                    error<errors::Code::PARSE_ELSE_WITHOUT_IF>(here() + 4);
                    break;
                default:
                    // DESIGN §4.6's OWN WORKED EXAMPLE, one node further down
                    // the trie than the one it uses. `satellite.statement.wihle`
                    // is not a statement; `satellite.statement`'s four children
                    // are the candidate list, and suggest.cpp's transposition
                    // arm is what makes the answer `while` rather than nothing.
                    error<errors::Code::PARSE_NO_SUCH_STATEMENT>(here() + 4,
                                                                 peek(4).text);
                    suggest(here() + 4,
                            static_cast<words::PathId>(words::NodeId::STATEMENT));
                    break;
                }
                break;
            case Segment1::Return:
                value = return_stmt();
                if (value != kNoNode)
                    end_of_statement();
                break;
            case Segment1::Include:
            case Segment1::Capsule:
            case Segment1::Spacesuit:
            case Segment1::Returns:
            case Segment1::Protected:
            case Segment1::Public:
                // DESIGN §6 puts all six under `top_level` or inside a suit
                // block, never inside a `block`. Saying which one was written is
                // what makes this better than "unexpected token": the person
                // wrote a real word of the language in a place it does not go.
                error<errors::Code::PARSE_DECLARATION_IN_BLOCK>(here() + 2,
                                                                peek(2).text);
                break;
            case Segment1::Constructor:
                error<errors::Code::PARSE_CONSTRUCTOR_OUTSIDE_SPACESUIT>(here() + 2);
                break;
            case Segment1::Library:
            case Segment1::None:
                // A value path or a module path. Both are expressions, and
                // DESIGN §6.3 says the parser does not resolve either.
                if (at_punct("{")) {
                    if (open_block())
                        step = Step::Item;
                    break;
                }
                if (at_declaration()) {
                    const NodeIndex declared = type();
                    if (declared == kNoNode)
                        break;
                    value = var_decl(declared);
                    if (value != kNoNode)
                        end_of_statement();
                    break;
                }
                value = assign_or_expression();
                if (value != kNoNode)
                    end_of_statement();
                break;
            }
            break;
        }

        case Step::Deliver: {
            if (open_.size() == floor)
                return value;

            switch (open_.back().kind) {
            case Open::Kind::Block:
                if (value != kNoNode)
                    open_.back().items.push_back(value);
                if (pos_ == open_.back().before) {
                    error<errors::Code::PARSE_EXPECTED_STATEMENT>(
                        here(), describe(peek()));
                    advance();
                }
                if (panic_)
                    synchronise();
                step = Step::Item;
                break;

            case Open::Kind::If:
                if (!open_.back().otherwise) {
                    if (value == kNoNode) {
                        open_.pop_back();
                        break;
                    }
                    open_.back().b = value;

                    // THE ELSE MAY BE ON ITS OWN LINE, so the newlines after the
                    // closing brace are crossed to look for it -- and the cursor
                    // is put back when there is no else, because those newlines
                    // belong to whatever comes next. Crossing them without a way
                    // back is how an `if` at the end of a block eats the brace
                    // that closes it.
                    const size_t before = pos_;
                    skip_newlines();
                    if (!at_else()) {
                        pos_ = before;
                        const Open done = open_.back();
                        open_.pop_back();
                        value = ast_.add(NodeKind::If, done.at, done.a, done.b,
                                         kNoNode);
                        break;
                    }
                    take_statement_keyword();
                    skip_newlines();
                    open_.back().otherwise = true;

                    // `( block | if_stmt )`, which is what keeps `else if` from
                    // being a form of its own: the second arm is a statement
                    // again, and the tree that comes out of `else if` is an If
                    // in an If.
                    if (at_punct("{")) {
                        if (open_block()) {
                            step = Step::Item;
                            break;
                        }
                        open_.pop_back();
                        value = kNoNode;
                        break;
                    }
                    if (opening() == Segment1::Statement) {
                        step = Step::Statement;
                        break;
                    }
                    error<errors::Code::PARSE_ELSE_NEEDS_A_BLOCK>(here(),
                                                                  describe(peek()));
                    open_.pop_back();
                    value = kNoNode;
                    break;
                }
                {
                    const Open done = open_.back();
                    open_.pop_back();
                    value = value == kNoNode
                                ? kNoNode
                                : ast_.add(NodeKind::If, done.at, done.a, done.b,
                                           value);
                }
                break;

            case Open::Kind::While: {
                const Open done = open_.back();
                open_.pop_back();
                value = value == kNoNode
                            ? kNoNode
                            : ast_.add(NodeKind::While, done.at, done.a, value);
                break;
            }

            case Open::Kind::For: {
                const Open done = open_.back();
                open_.pop_back();
                value = value == kNoNode
                            ? kNoNode
                            : ast_.add(NodeKind::For, done.at, done.a, done.b,
                                       done.c, value);
                break;
            }
            }
            break;
        }
        }
    }
}

NodeIndex Parser::var_decl(NodeIndex declared_type)
{
    const uint32_t name = expect_word("a name for the variable");
    if (panic_)
        return kNoNode;

    // A LOCAL'S NAME TAKES NO NUMBER, and that is not an omission. WORD_NUMBERS
    // §3 numbers user capsules and spacesuits, because those need identity
    // across a program; a local needs a FRAME SLOT, which DESIGN §7.2 resolves
    // to an integer index in M7's pass and which is per call rather than per
    // program. Numbering locals here would put one slot per local per program
    // back -- which is §7.1's verified blocker, where a recursive `fact`
    // returns 1 for every input.
    // `counter tally("hello")` -- A SPACESUIT'S CONSTRUCTOR ARGUMENTS, 2026-09-12.
    // Written where the declaration is, and handed to the suit's
    // `satellite.constructor(args)`; the resolver refuses them on any type that
    // is not a spacesuit, because this parser cannot tell a user's type name
    // from any other word. The list is `c`, and an empty `()` is a list with
    // nothing in it rather than kNoList, so the two spellings stay apart in the
    // tree even though both hand the constructor nothing.
    ListId arguments = kNoList;
    if (at_punct("(")) {
        const uint32_t opener = here();
        advance();
        open_bracket();
        std::vector<NodeIndex> given;
        while (!at_punct(")")) {
            const NodeIndex argument = expression();
            if (argument == kNoNode) {
                close_bracket();
                return kNoNode;
            }
            given.push_back(argument);
            if (!take_punct(","))
                break;
        }
        close_bracket();
        if (!expect_punct(")", "to close the constructor's arguments", opener))
            return kNoNode;
        arguments = ast_.add_list(given);
    }

    NodeIndex init = kNoNode;
    if (arguments == kNoList && take_punct("=")) {
        init = expression();
        if (init == kNoNode)
            return kNoNode;
    }
    return ast_.add(NodeKind::VarDecl, name, declared_type, init, arguments);
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
