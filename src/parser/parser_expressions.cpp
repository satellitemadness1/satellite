// Expressions: precedence climbing, the postfix loop, and the primaries -- all
// of it on stacks this file keeps rather than on the C++ one. See
// parser/parser.hpp for what the parser promises.
//
// THE PRECEDENCE TABLE WAS M4's DECISION AND IS NOW DESIGN §6.6.
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
// makes unary minus an expression rule by construction. `!` is the one M4
// decided, on the grounds that `!=` exists and a language with an inequality has
// a negation -- and DESIGN §13 now carries it as OPEN, together with `&` and
// with the fact that the language has no `and`, `or` or `not` at all.
//
// ---------------------------------------------------------------------------
//
// THE SIX FUNCTIONS BECAME ONE MACHINE AT M8.5, AND DESIGN §7.5 IS WHY. The
// cycle was expression -> unary -> postfix -> primary -> '(' expression, with
// an argument list and a subscript as two more ways back in, so a program could
// choose how deep this parser went and 32,000 brackets ended it with signal 11.
// Three stacks and a step replace it:
//
//   operands / operators   precedence climbing, done by folding rather than by
//                          calling: an operator waits on `operators` until one
//                          that binds no tighter arrives, which is the same
//                          left-associativity `expression(precedence + 1)` had.
//   frames                 one per OPEN BRACKET -- a '(' group, an argument
//                          list, a subscript. It remembers what the brackets
//                          interrupted, and its two marks are where this
//                          bracket's own operators and unaries start, so a fold
//                          can never reach past the bracket it is inside.
//   unaries                the '-' and '!' waiting for the operand they apply
//                          to, which is the whole postfix chain and not the
//                          primary: `-a.b()` is `-(a.b())`.
//
// A STEP AND NOT A RETURN ADDRESS. `Operand` wants a primary, `Postfix` has one
// and is looking for `.`, `(` or `[`, and `Binary` has a finished operand and
// is looking for an operator or for the end of the bracket it is in. The three
// are the three places the recursive version could be suspended, which is what
// says the translation is complete rather than convenient.

#include "parser/parser_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "lexical_analyzer/lexer.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace satellite {

namespace {

// A binary operator waiting for its right-hand side.
struct Operator {
    uint32_t token = 0;
    int precedence = 0;
};

// One open bracket, and what it interrupted.
constexpr uint32_t kNoName = UINT32_MAX;

struct Frame {
    enum class Kind : uint8_t { Paren, Call, Subscript };

    Kind kind = Kind::Paren;
    uint32_t opener = 0;
    NodeIndex target = kNoNode;         // Call and Subscript: what is being applied
    size_t operators = 0;               // the two marks

    size_t unaries = 0;
    std::vector<NodeIndex> arguments;   // Call
    std::vector<NodeIndex> named;       // Call: `name=value`, M30
    bool argument_start = false;        // Call: the next operand begins an argument
    uint32_t name = kNoName;            // Call: the name this argument was given
    uint32_t started = 0;               // Call: this argument's first token
    NodeIndex low = kNoNode;            // Subscript
    bool colon = false;                 // Subscript: a ':' was taken
};

} // namespace

NodeIndex Parser::expression()
{
    std::vector<Frame> frames;
    std::vector<NodeIndex> operands;
    std::vector<Operator> operators;
    std::vector<uint32_t> unaries;

    enum class Step : uint8_t { Operand, Postfix, Binary, Close };
    Step step = Step::Operand;
    NodeIndex current = kNoNode;

    // A FAILURE UNWINDS THE BRACKETS IT IS INSIDE, which is what the recursive
    // version did by returning through them: every frame opened one and
    // close_bracket() is how the cursor stops skipping newlines again.
    const auto give_up = [&]() -> NodeIndex {
        for (size_t i = frames.size(); i-- > 0;)
            close_bracket();
        return kNoNode;
    };

    // The innermost bracket's marks, or the bottom of each stack.
    const auto operator_mark = [&]() { return frames.empty() ? 0 : frames.back().operators; };
    const auto unary_mark = [&]() { return frames.empty() ? 0 : frames.back().unaries; };

    const auto fold = [&]() {
        const Operator op = operators.back();
        operators.pop_back();
        const NodeIndex right = operands.back();
        operands.pop_back();
        const NodeIndex left = operands.back();
        operands.pop_back();
        operands.push_back(ast_.add(NodeKind::Binary, op.token, left, right));
    };

    const auto push_frame = [&](Frame::Kind kind, uint32_t opener, NodeIndex target) {
        Frame frame;
        frame.kind = kind;
        frame.opener = opener;
        frame.target = target;
        frame.operators = operators.size();
        frame.unaries = unaries.size();
        frames.push_back(std::move(frame));
    };

    for (;;) {
        switch (step) {

        case Step::Operand: {
            // `name=value` -- M30's named argument. ONLY AT THE START OF AN
            // ARGUMENT, which is what keeps `f(a == b)` a comparison: `==` is
            // one token, and a word followed by `=` anywhere else is not an
            // expression at all. Two tokens of lookahead, asked once per
            // argument and never inside one.
            if (!frames.empty() && frames.back().kind == Frame::Kind::Call &&
                frames.back().argument_start) {
                Frame &call = frames.back();
                call.argument_start = false;
                call.started = here();
                if (peek().kind == TokenKind::Word && peek(1).kind == TokenKind::Punct &&
                    peek(1).text == "=") {
                    const uint32_t name = here();
                    for (const NodeIndex earlier : call.named) {
                        if (toks()[ast_[earlier].token].text == toks()[name].text) {
                            error<errors::Code::PARSE_NAMED_TWICE>(
                                name, std::string(toks()[name].text));
                            return give_up();
                        }
                    }
                    call.name = name;
                    advance();
                    advance();
                }
            }

            // `unary := ( "-" | "!" ) unary`, collected rather than nested. They
            // apply to the whole postfix chain that follows, so they wait.
            while (peek().kind == TokenKind::Punct &&
                   (peek().text == "-" || peek().text == "!")) {
                unaries.push_back(here());
                advance();
            }

            const uint32_t at = here();
            switch (peek().kind) {
            case TokenKind::Number:
                advance();
                current = ast_.add(NodeKind::Number, at);
                step = Step::Postfix;
                continue;
            case TokenKind::String:
                advance();
                current = ast_.add(NodeKind::String, at);
                step = Step::Postfix;
                continue;
            case TokenKind::Bits:
                advance();
                current = ast_.add(NodeKind::Bits, at);
                step = Step::Postfix;
                continue;
            case TokenKind::Word:
                // ONE INTEGER COMPARE, which is DESIGN §2's reservation rule in
                // the place it is asked most often. Every path in the language
                // begins with this word, so a parser that compared strings here
                // would compare a string per path per program.
                advance();
                current = ast_.add(is_reserved_word(toks()[at]) ? NodeKind::Satellite
                                                                : NodeKind::Name,
                                   at);
                step = Step::Postfix;
                continue;
            default:
                break;
            }

            if (at_punct("(")) {
                advance();
                open_bracket();
                push_frame(Frame::Kind::Paren, at, kNoNode);
                step = Step::Operand;
                continue;
            }

            error<errors::Code::PARSE_EXPECTED_EXPRESSION>(here(), describe(peek()));
            return give_up();
        }

        case Step::Postfix: {
            // DESIGN §6.2, and the loop is the design's own five lines. ONE NODE
            // VARIABLE, THREE FORMS, EACH REPLACING THE NODE: that is what makes
            // `satellite.time.now().some_function()` and `my_list[0].f()[1:2]`
            // fall out with no rule of their own.
            if (at_punct(".")) {
                advance();
                // The member name may be any word, INCLUDING one the language
                // owns: `arguments.machine.threads` is six numbers deep and every
                // segment after the first is a word in words.def. Nothing here
                // looks any of them up -- DESIGN §6.3 -- so nothing here has to
                // know that.
                const uint32_t name = expect_word("a member name after '.'");
                if (panic_)
                    return give_up();
                current = ast_.add(NodeKind::Member, name, current);
                continue;
            }

            if (at_punct("(")) {
                const uint32_t opener = here();
                advance();
                open_bracket();
                if (at_punct(")")) {
                    // The empty argument list, closed where it opened. A frame
                    // for it would be pushed and popped with nothing in between.
                    close_bracket();
                    expect_punct(")", "to close the argument list", opener);
                    if (panic_)
                        return give_up();
                    current = ast_.add(NodeKind::Call, opener, current,
                                       ast_.add_list({}));
                    continue;
                }
                push_frame(Frame::Kind::Call, opener, current);
                frames.back().argument_start = true;
                step = Step::Operand;
                continue;
            }

            if (at_punct("[")) {
                const uint32_t opener = here();
                advance();
                open_bracket();
                push_frame(Frame::Kind::Subscript, opener, current);
                if (!at_punct(":") && !at_punct("]")) {
                    step = Step::Operand;
                    continue;
                }
                // `x[:2]` and `x[]` have no low part, so there is no operand to
                // fold and the frame is asked to close on nothing. That is the
                // one way into Close that did not come through Binary.
                current = kNoNode;
                step = Step::Close;
                continue;
            }

            // The chain has ended, so the unaries this operand collected apply
            // -- innermost first, which is what popping gives.
            while (unaries.size() > unary_mark()) {
                current = ast_.add(NodeKind::Unary, unaries.back(), current);
                unaries.pop_back();
            }
            step = Step::Binary;
            continue;
        }

        case Step::Binary: {
            if (peek().kind == TokenKind::Punct) {
                const int precedence = precedence_of(peek().text);
                if (precedence > 0) {
                    operands.push_back(current);
                    // LEFT-ASSOCIATIVE, and `>=` is the whole of what says so:
                    // an operator that binds as tightly as the one waiting
                    // folds it first, so `a - b - c` groups as `(a - b) - c`.
                    // `>` alone makes every operator right-associative and
                    // subtraction wrong.
                    while (operators.size() > operator_mark() &&
                           operators.back().precedence >= precedence)
                        fold();
                    operators.push_back({here(), precedence});
                    advance();
                    step = Step::Operand;
                    continue;
                }
            }

            // Nothing else binds here, so this bracket's operators all fold and
            // what comes out is the operand the frame around it was waiting for.
            operands.push_back(current);
            while (operators.size() > operator_mark())
                fold();
            current = operands.back();
            operands.pop_back();
            step = Step::Close;
            continue;
        }

        case Step::Close: {
            if (frames.empty())
                return current;

            switch (frames.back().kind) {
            case Frame::Kind::Paren: {
                const uint32_t opener = frames.back().opener;
                frames.pop_back();
                close_bracket();
                if (!expect_punct(")", "to close the expression", opener))
                    return give_up();
                // NO NODE RECORDS THAT A PARENTHESIS WAS WRITTEN, and that is a
                // decision with a consequence the unparser has to carry:
                // `(a + b) * c` and `a + b * c` differ in shape, so the printer
                // puts the brackets back from precedence rather than from
                // memory. Storing them would put a piece of punctuation in a
                // tree that is about meaning, and would make two trees for one
                // program -- which is the thing M4.5's `.satc` and M7's resolve
                // both have to compare.
                step = Step::Postfix;
                continue;
            }

            case Frame::Kind::Call: {
                Frame &call = frames.back();
                if (call.name != kNoName) {
                    call.named.push_back(ast_.add(NodeKind::Named, call.name, current));
                    call.name = kNoName;
                } else if (!call.named.empty()) {
                    error<errors::Code::PARSE_POSITIONAL_AFTER_NAMED>(call.started);
                    return give_up();
                } else {
                    call.arguments.push_back(current);
                }
                if (take_punct(",")) {
                    call.argument_start = true;
                    step = Step::Operand;
                    continue;
                }
                const Frame done = std::move(frames.back());
                frames.pop_back();
                close_bracket();
                expect_punct(")", "to close the argument list", done.opener);
                if (panic_)
                    return give_up();
                current = ast_.add(NodeKind::Call, done.opener, done.target,
                                   ast_.add_list(done.arguments),
                                   done.named.empty() ? kNoList
                                                      : ast_.add_list(done.named));
                step = Step::Postfix;
                continue;
            }

            case Frame::Kind::Subscript: {
                if (!frames.back().colon) {
                    frames.back().low = current;
                    if (take_punct(":")) {
                        frames.back().colon = true;
                        if (!at_punct("]")) {
                            step = Step::Operand;
                            continue;
                        }
                        current = kNoNode;  // `x[1:]` -- no high part
                    } else if (frames.back().low == kNoNode) {
                        frames.pop_back();
                        error<errors::Code::PARSE_EXPECTED_SUBSCRIPT>(
                            here(), describe(peek()));
                        close_bracket();
                        return give_up();
                    } else {
                        // A SLICE AND AN INDEX ARE DIFFERENT KINDS, not one kind
                        // with an absent half. `x[1]` is one element and `x[1:]`
                        // is a list of them, so they have different types before
                        // they have different values -- and a reader of the tree
                        // should not have to check a sentinel to find out which
                        // was written.
                        const Frame done = std::move(frames.back());
                        frames.pop_back();
                        const NodeIndex node = ast_.add(NodeKind::Index, done.opener,
                                                        done.target, done.low);
                        close_bracket();
                        if (!expect_punct("]", "to close the subscript", done.opener))
                            return give_up();
                        current = node;
                        step = Step::Postfix;
                        continue;
                    }
                }

                const Frame done = std::move(frames.back());
                frames.pop_back();
                const NodeIndex node = ast_.add(NodeKind::Slice, done.opener,
                                                done.target, done.low, current);
                close_bracket();
                if (!expect_punct("]", "to close the subscript", done.opener))
                    return give_up();
                current = node;
                step = Step::Postfix;
                continue;
            }
            }
            continue;
        }
        }
    }
}

} // namespace satellite
