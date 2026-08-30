// The cursor, the errors, and the loop over a file. See parser/parser.hpp for
// what the parser promises and parser_internal.hpp for the segment-1 table.
//
// THE SAME-LINE RULE OF DESIGN §6.2 IS NOT A COMPARISON IN THIS PARSER, and
// that is the one thing in this file a reader should not have to find out by
// grepping for `line`. §6.2 requires a postfix `(` or `[` to bind only when it
// opens on the same line as the thing it applies to, or a line ending in `x`
// followed by a line opening `(f(y))` parses as `x(f(y))` and swallows a
// statement. DESIGN §5.6 keeps the newline in the token stream, so the opener
// on the next line is separated from `x` by a token the postfix loop never
// crosses -- the rule holds because the terminator is a token, not because
// anything compares two integers. The `line` field is still load-bearing and
// still checked: tests/lexer_test/spans.cpp is where it is proved, and
// tests/parser_test/expressions.cpp asserts the two statements come out as two.
//
// NEWLINES ARE SKIPPED INSIDE BRACKETS AND NOWHERE ELSE. DESIGN §6 says
// statements are newline-terminated and says nothing about an argument list
// spanning lines, which every real program eventually wants. Skipping them
// while a bracket is open is what makes that legal, and it cannot weaken §6.2:
// the postfix loop runs at bracket depth 0, where nothing is skipped.

#include "parser/parser_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace satellite {

namespace {

// Enough errors to be worth reading and few enough to be worth printing. The
// number is arbitrary and is here rather than in a caller because a caller that
// could choose it would have to know what a cascade looks like.
constexpr size_t kMaxErrors = 20;

} // namespace

// ---------------------------------------------------------------------------
// The cursor
// ---------------------------------------------------------------------------

const Token &Parser::peek(size_t ahead) const
{
    const size_t at = pos_ + ahead;
    // CLAMPED TO THE LAST TOKEN, WHICH IS ALWAYS End. lexer.hpp guarantees the
    // stream ends with one, so lookahead past the end reads End rather than
    // needing a bounds test at every call site -- and End matches nothing, so
    // every rule below stops on it without being told to.
    return toks()[at < toks().size() ? at : toks().size() - 1];
}

const Token &Parser::advance()
{
    const Token &taken = peek();
    if (pos_ + 1 < toks().size())
        pos_++;
    if (brackets_ > 0)
        while (peek().kind == TokenKind::Newline && pos_ + 1 < toks().size())
            pos_++;
    return taken;
}

bool Parser::at_punct(std::string_view text, size_t ahead) const
{
    const Token &token = peek(ahead);
    return token.kind == TokenKind::Punct && token.text == text;
}

bool Parser::at_word(size_t ahead) const
{
    return peek(ahead).kind == TokenKind::Word;
}

bool Parser::take_punct(std::string_view text)
{
    if (!at_punct(text))
        return false;
    advance();
    return true;
}

bool Parser::expect_punct(std::string_view text, std::string_view what,
                          uint32_t opener)
{
    if (take_punct(text))
        return true;
    error<errors::Code::PARSE_EXPECTED_PUNCT>(here(), text, what, describe(peek()));
    // THE NOTE IS DRAWN HERE FOR ALL NINE CALLERS, and parser_internal.hpp says
    // why it is one place: a site composing its own would have to ask first
    // whether the error above was recorded at all.
    if (opener != kNoOpener)
        attach(errors::note<errors::Code::NOTE_OPENED_HERE>(span_of(opener),
                                                            toks()[opener].text));
    return false;
}

uint32_t Parser::expect_word(std::string_view what)
{
    if (at_word()) {
        const uint32_t at = here();
        advance();
        return at;
    }
    error<errors::Code::PARSE_EXPECTED_WORD>(here(), what, describe(peek()));
    return 0;
}

Segment1 Parser::opening() const
{
    if (!is_reserved_word(peek()) || !at_punct(".", 1) || !at_word(2))
        return Segment1::None;
    return segment1_of(peek(2).spelling);
}

void Parser::skip_newlines()
{
    while (peek().kind == TokenKind::Newline)
        advance();
}

void Parser::end_of_statement()
{
    if (peek().kind == TokenKind::Newline) {
        skip_newlines();
        return;
    }
    // A statement may also end where its enclosing block does, so `{ x = 1 }`
    // on one line is not an error. Nothing else ends one: two statements on one
    // line have no separator in this language, because there is no `;` outside
    // a `for` header.
    if (at_end() || at_punct("}"))
        return;
    error<errors::Code::PARSE_STATEMENT_ALREADY_ENDED>(here(), describe(peek()));
}

void Parser::open_bracket()
{
    brackets_++;
    while (peek().kind == TokenKind::Newline && pos_ + 1 < toks().size())
        pos_++;
}

void Parser::close_bracket()
{
    if (brackets_ > 0)
        brackets_--;
}

// ---------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------

// ONE ERROR PER SYNCHRONISATION, and the rule lives in the template in
// parser_internal.hpp. A parser that is already lost reports the wreckage
// rather than the cause, and the first satellite's front end is the reason this
// is a rule here: a user reading twelve messages about one missing brace fixes
// the wrong thing first. synchronise() below clears it.

errors::Span Parser::span_of(uint32_t token) const
{
    // THE WHOLE CONVERSION FROM M4's SHAPE TO M5's IS THESE THREE FIELDS, which
    // is what M4's decision to store a token index rather than a message bought
    // -- parser.hpp says so beside the type that used to be here.
    const Token &at = toks()[token < toks().size() ? token : toks().size() - 1];
    return errors::Span{at.start, at.end, at.line};
}

void Parser::attach(errors::Note remark)
{
    if (!fresh_)
        return;
    errors_.back().notes.push_back(std::move(remark));
}

void Parser::suggest(uint32_t token, words::PathId under)
{
    if (!fresh_)
        return;
    const std::string_view word = toks()[token].text;
    errors_.back().suggestion = std::string(errors::suggest(under, word));
}

void Parser::synchronise()
{
    panic_ = false;
    // TO THE END OF THE STATEMENT, and a closing brace is left where it is so
    // the block that owns it can still close. Skipping past a `}` would make
    // one bad statement swallow the rest of a capsule.
    while (!at_end() && !at_punct("}")) {
        if (peek().kind == TokenKind::Newline) {
            skip_newlines();
            return;
        }
        advance();
    }
}

bool Parser::stop() const
{
    return errors_.size() >= kMaxErrors;
}

// ---------------------------------------------------------------------------
// The file
// ---------------------------------------------------------------------------

void Parser::run()
{
    std::vector<NodeIndex> items;

    skip_newlines();
    while (!at_end() && !stop()) {
        const size_t before = pos_;
        const NodeIndex item = top_level();
        if (item != kNoNode)
            items.push_back(item);

        // NO RULE MAY LEAVE THE CURSOR WHERE IT FOUND IT, and this is the guard
        // rather than a claim in a comment: a rule that returns without
        // consuming turns this loop into a hang, which is the one failure a
        // user cannot tell from a slow program.
        if (pos_ == before) {
            error<errors::Code::PARSE_EXPECTED_DECLARATION>(here(), describe(peek()));
            advance();
        }
        if (panic_)
            synchronise();
        skip_newlines();
    }

    ast_.set_root(ast_.add(NodeKind::Program, 0, ast_.add_list(items)));
}

// ---------------------------------------------------------------------------
// The entry points
// ---------------------------------------------------------------------------

Parse parse(std::vector<Token> tokens, words::Words &words)
{
    // AN EMPTY STREAM IS NOT A THING THE LEXER PRODUCES -- lex() always ends
    // with End -- so this is about a caller that built a vector by hand, which
    // tests do. One token costs less than a bounds check in peek().
    if (tokens.empty())
        tokens.push_back(Token{});

    Parse result;
    result.ast = Ast(std::move(tokens));

    // A LEXICAL ERROR IS REPORTED AND NOT PARSED AROUND. The lexer stops at the
    // first one (DESIGN §5.6), so what follows an Error token is not the rest
    // of the program -- it is the End the lexer appended. Parsing it would
    // produce a tree of everything before the error and no sign that the file
    // continued, which is worse than saying so.
    // AND THE LEXER IS WHAT SAYS WHAT ONE MEANS. diagnostics_of() is next door
    // in the lexer for the reason lexer.hpp gives: which caret and which note an
    // unterminated string wants is a fact about lexing, and a converter here
    // would be a second module that has to know it.
    result.errors = diagnostics_of(result.ast.tokens());
    if (!result.errors.empty())
        return result;

    Parser parser(result.ast, words);
    parser.run();

    // ASKED BEFORE take_errors() AND NOT AFTER, which is a one-line ordering
    // with a silent failure behind it: `gave_up()` answers by counting the
    // errors, and take_errors() MOVES them out, so a parser asked afterwards
    // has none and always says it finished. Found by the test that counts
    // them -- thirty bad statements came back as twenty-one errors and no note.
    const bool gave_up = parser.gave_up();
    result.errors = parser.take_errors();

    // SAID, RATHER THAN SHOWN BY THE OUTPUT SIMPLY STOPPING. The parser reports
    // at most kMaxErrors and then leaves the rest of the file unread, which
    // MILESTONES/M4.md §6 item 8 recorded as a number in this file and in no
    // document. It is still this file's number; what M5 adds is that the reader
    // is told it was reached.
    if (gave_up)
        result.errors.push_back(
            errors::make<errors::Code::PARSE_TOO_MANY_ERRORS>(
                errors::kNowhere, result.errors.size()));
    return result;
}

Parse parse(const std::string &source, words::Words &words)
{
    return parse(lex(source), words);
}

} // namespace satellite
