// The token cursor, the lookahead predicates, and error recovery.
//
// Part of parser/, split from a 1022-line parser.cpp.

#include "parser_internal.hpp"

namespace satellite {

const Token &Parser::peek(size_t ahead) const
{
    size_t i = pos_ + ahead;
    return i < toks_.size() ? toks_[i] : toks_.back();
}

const Token &Parser::previous() const
{
    return pos_ > 0 ? toks_[pos_ - 1] : toks_.front();
}

bool Parser::at_end() const
{
    return peek().kind == TokenKind::End || peek().kind == TokenKind::Error;
}

const Token &Parser::advance()
{
    const Token &t = peek();
    if (!at_end())
        pos_++;
    return t;
}

bool Parser::is_punct(size_t ahead, const char *text) const
{
    const Token &t = peek(ahead);
    return t.kind == TokenKind::Punct && t.text == text;
}

bool Parser::is_word(size_t ahead, const char *text) const
{
    const Token &t = peek(ahead);
    return t.kind == TokenKind::Word && t.text == text;
}

bool Parser::match_punct(const char *text)
{
    if (!is_punct(0, text))
        return false;
    advance();
    return true;
}

void Parser::error(const Token &at, const std::string &message)
{
    if (panic_ || errors_.size() >= MAX_ERRORS)
        return; // one report per construct; the rest would be noise

    // Pointing at End is useless — it is past the last line, so the caret
    // lands on nothing. Report the last real token instead, which for an
    // unclosed construct is the '(' or '{' that was never closed.
    const Token &where =
        (at.kind == TokenKind::End && pos_ > 0) ? previous() : at;
    errors_.push_back(ParseError{message, span_of(where)});
    panic_ = true;
}

void Parser::expect_statement_end()
{
    if (at_end() || is_punct(0, "}"))
        return;
    if (peek().line != previous().line)
        return;
    error(peek(), "unexpected " + describe(peek()) +
                      " after the end of a statement");
}

bool Parser::expect_punct(const char *text, const std::string &what)
{
    if (match_punct(text))
        return true;
    error(peek(), "expected '" + std::string(text) + "' " + what);
    return false;
}

std::string Parser::expect_word(const std::string &what)
{
    if (peek().kind == TokenKind::Word)
        return advance().text;
    error(peek(), "expected " + what);
    return {};
}

void Parser::synchronize()
{
    panic_ = false;
    const size_t line = previous().line;
    while (!at_end()) {
        if (is_punct(0, "}"))
            return;
        if (peek().line != line && starts_construct())
            return;
        advance();
    }
}

bool Parser::starts_construct() const
{
    return at_type() || at_language_path("statement") ||
           at_language_path("return") || at_language_path("include") ||
           at_language_path("capsule") || at_language_path("spacesuit") ||
           at_language_path("protected") || at_language_path("public") ||
           is_punct(0, "{");
}

bool Parser::at_language_path(const char *segment) const
{
    return is_word(0, "satellite") && is_punct(1, ".") &&
           is_word(2, segment);
}

bool Parser::at_type() const
{
    if (at_language_path("variable") || at_language_path("container"))
        return true;
    // the bare `satellite` singleton type, used in a type position
    if (is_word(0, "satellite") && !is_punct(1, "."))
        return true;

    // A spacesuit type: a bare name followed by the name it types. §5
    // already established that TWO ADJACENT WORDS are a shape no
    // expression can produce, which is what makes this test safe where a
    // dotted path followed by a Word is not — `main.x foo` and
    // `satellite.control.return my_time` both have a Punct between the
    // path and the name, so neither reaches here.
    //
    // Same line, for the same reason the type of an ordinary declaration
    // must be: without it a bare name at the end of one line silently
    // swallows the name that starts the next.
    return peek(0).kind == TokenKind::Word &&
           peek(1).kind == TokenKind::Word &&
           peek(1).line == peek(0).line;
}

bool Parser::at_statement_keyword(const char *name) const
{
    return at_language_path("statement") && is_punct(3, ".") &&
           is_word(4, name);
}

void Parser::take_statement_keyword()
{
    advance(); // satellite
    advance(); // .
    advance(); // statement
    advance(); // .
    advance(); // if / else / while / for
}

Span Parser::span_from(size_t first) const
{
    const Token &a = first < toks_.size() ? toks_[first] : toks_.back();
    return span_join(span_of(a), span_of(previous()));
}

} // namespace satellite
