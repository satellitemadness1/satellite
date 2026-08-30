// Expressions: precedence climbing, the postfix loop, and the primaries.
// See parser/parser.hpp for what the parser promises.
//
// THE PRECEDENCE TABLE WAS THIS MILESTONE'S DECISION AND IS NOW DESIGN §6.6.
// The grammar wrote the whole expression rule as
// `expression := ... precedence climbing ...` and stopped, so the levels below
// were chosen here on 2026-08-30 and written into DESIGN the same day. The
// grounds are worth keeping, because a table like this is impossible to change
// later without changing what programs mean:
//
//   * / %      4      the four operations, and DESIGN §8.6 specifies them
//   + -        3
//   < > <= >=  2      the greedy two-character operators DESIGN §5.5 permits
//   == !=      1
//
// C's levels, minus every operator satellite does not have. That is not
// deference to C: it is the only table a reader of this language already knows,
// and §1.1's tie-breaker spends the language's surprise budget on things the
// user gains something from. `<<` and `>>` are absent because DESIGN §5.5
// refuses them permanently, and the bitwise and logical rows are absent because
// no document in this tree has said what `&` means -- the lexer will hand one
// over as a Punct and this table gives it precedence 0, so it ends an
// expression and is reported rather than guessed at.
//
// THE TABLE ITSELF LIVES IN abstract_syntax_tree/ast.hpp, not here, because the
// unparser reads it too -- it is what decides whether `(a + b) * c` needs its
// brackets printed back. One table, two readers.
//
// UNARY IS `-` AND `!`. The minus is required rather than chosen: DESIGN §5.6
// refuses to fold a sign into a Number so that `a-1` stays a subtraction, which
// makes unary minus an expression rule by construction. `!` is the one this
// file decided, on the grounds that `!=` exists and a language with an
// inequality has a negation -- and DESIGN §13 now carries it as OPEN, together
// with `&` and with the fact that the language has no `and`, `or` or `not` at
// all. It is one question, and this file guessed the smallest part of it.

#include "parser/parser_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "lexical_analyzer/lexer.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace satellite {

NodeIndex Parser::expression(int min_precedence)
{
    NodeIndex left = unary();
    if (left == kNoNode)
        return kNoNode;

    for (;;) {
        if (peek().kind != TokenKind::Punct)
            return left;
        const int precedence = precedence_of(peek().text);
        if (precedence == 0 || precedence < min_precedence)
            return left;

        const uint32_t op = here();
        advance();
        // LEFT-ASSOCIATIVE, and `precedence + 1` is the whole of what says so:
        // the right-hand side may only contain operators that bind TIGHTER, so
        // `a - b - c` groups as `(a - b) - c`. Passing `precedence` instead
        // makes every operator right-associative and subtraction wrong.
        const NodeIndex right = expression(precedence + 1);
        if (right == kNoNode)
            return kNoNode;
        left = ast_.add(NodeKind::Binary, op, left, right);
    }
}

NodeIndex Parser::unary()
{
    if (peek().kind == TokenKind::Punct && (peek().text == "-" || peek().text == "!")) {
        const uint32_t op = here();
        advance();
        const NodeIndex operand = unary();
        if (operand == kNoNode)
            return kNoNode;
        return ast_.add(NodeKind::Unary, op, operand);
    }
    return postfix();
}

// DESIGN §6.2, and the loop is the design's own five lines with the errors
// filled in. ONE NODE VARIABLE, THREE FORMS, EACH REPLACING THE NODE: that is
// what makes `satellite.time.now().some_function()` and `my_list[0].f()[1:2]`
// fall out with no rule of their own. The sketch it replaces -- parse a primary
// and then `while (peek == '.')` -- cannot parse `foo().bar()` at all.
NodeIndex Parser::postfix()
{
    NodeIndex node = primary();
    if (node == kNoNode)
        return kNoNode;

    for (;;) {
        if (at_punct(".")) {
            advance();
            // The member name may be any word, INCLUDING one the language owns:
            // `arguments.machine.threads` is six numbers deep and every segment
            // after the first is a word in words.def. Nothing here looks any of
            // them up -- DESIGN §6.3 -- so nothing here has to know that.
            const uint32_t name = expect_word("a member name after '.'");
            if (panic_)
                return kNoNode;
            node = ast_.add(NodeKind::Member, name, node);
            continue;
        }
        if (at_punct("(")) {
            const uint32_t opener = here();
            const ListId args = argument_list();
            if (panic_)
                return kNoNode;
            node = ast_.add(NodeKind::Call, opener, node, args);
            continue;
        }
        if (at_punct("[")) {
            const uint32_t opener = here();
            node = subscript(node, opener);
            if (node == kNoNode)
                return kNoNode;
            continue;
        }
        return node;
    }
}

ListId Parser::argument_list()
{
    const uint32_t opener = here();
    advance();
    open_bracket();

    std::vector<NodeIndex> args;
    if (!at_punct(")")) {
        do {
            const NodeIndex arg = expression();
            if (arg == kNoNode) {
                close_bracket();
                return kNoList;
            }
            args.push_back(arg);
        } while (take_punct(","));
    }

    close_bracket();
    expect_punct(")", "to close the argument list", opener);
    return ast_.add_list(args);
}

NodeIndex Parser::subscript(NodeIndex target, uint32_t opener)
{
    advance();
    open_bracket();

    NodeIndex low = kNoNode;
    if (!at_punct(":") && !at_punct("]")) {
        low = expression();
        if (low == kNoNode) {
            close_bracket();
            return kNoNode;
        }
    }

    NodeIndex node = kNoNode;
    if (take_punct(":")) {
        // A SLICE AND AN INDEX ARE DIFFERENT KINDS, not one kind with an absent
        // half. `x[1]` is one element and `x[1:]` is a list of them, so they
        // have different types before they have different values -- and a
        // reader of the tree should not have to check a sentinel to find out
        // which was written.
        NodeIndex high = kNoNode;
        if (!at_punct("]")) {
            high = expression();
            if (high == kNoNode) {
                close_bracket();
                return kNoNode;
            }
        }
        node = ast_.add(NodeKind::Slice, opener, target, low, high);
    } else if (low == kNoNode) {
        error<errors::Code::PARSE_EXPECTED_SUBSCRIPT>(here(), describe(peek()));
        close_bracket();
        return kNoNode;
    } else {
        node = ast_.add(NodeKind::Index, opener, target, low);
    }

    close_bracket();
    if (!expect_punct("]", "to close the subscript", opener))
        return kNoNode;
    return node;
}

NodeIndex Parser::primary()
{
    const uint32_t at = here();

    switch (peek().kind) {
    case TokenKind::Number:
        advance();
        return ast_.add(NodeKind::Number, at);
    case TokenKind::String:
        advance();
        return ast_.add(NodeKind::String, at);
    case TokenKind::Bits:
        advance();
        return ast_.add(NodeKind::Bits, at);
    case TokenKind::Word:
        // ONE INTEGER COMPARE, which is DESIGN §2's reservation rule in the
        // place it is asked most often. Every path in the language begins with
        // this word, so a parser that compared strings here would compare a
        // string per path per program.
        advance();
        return ast_.add(is_reserved_word(toks()[at]) ? NodeKind::Satellite
                                                     : NodeKind::Name,
                        at);
    default:
        break;
    }

    if (at_punct("(")) {
        const uint32_t opener = here();
        advance();
        open_bracket();
        const NodeIndex inner = expression();
        close_bracket();
        if (inner == kNoNode)
            return kNoNode;
        if (!expect_punct(")", "to close the expression", opener))
            return kNoNode;
        // NO NODE RECORDS THAT A PARENTHESIS WAS WRITTEN, and that is a
        // decision with a consequence the unparser has to carry: `(a + b) * c`
        // and `a + b * c` differ in shape, so the printer puts the brackets
        // back from precedence rather than from memory. Storing them would put
        // a piece of punctuation in a tree that is about meaning, and would
        // make two trees for one program -- which is the thing M4.5's `.satc`
        // and M7's resolve both have to compare.
        return inner;
    }

    error<errors::Code::PARSE_EXPECTED_EXPRESSION>(here(), describe(peek()));
    return kNoNode;
}

} // namespace satellite
