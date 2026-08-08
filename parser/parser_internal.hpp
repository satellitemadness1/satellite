#pragma once

// Private to parser/. The public surface is ../parser.hpp and it has not
// changed.
//
// parser.cpp was 1022 lines, of which 957 were one class with every method
// defined inline. Splitting it means the class declaration lives here and the
// bodies live next door, grouped by what they parse. The comments stayed with
// the declarations, because they describe the contract rather than the code.
//
// The class was in an anonymous namespace when it was one translation unit; it
// needs external linkage now, so it is a plain namespace-scope class. Nothing
// outside parser/ may include this.

#include "../parser.hpp"

#include <utility>

namespace satellite {

constexpr size_t MAX_ERRORS = 32;

class Parser {
public:
    // The token vector is copied rather than referenced so that a generic
    // close welded into a '>=' can be split in place; see parse_type.
    explicit Parser(std::vector<Token> tokens) : toks_(std::move(tokens)) {}

    ParseResult run();

private:
    std::vector<Token> toks_;
    size_t pos_ = 0;
    std::vector<ParseError> errors_;
    bool panic_ = false;

    // --- token helpers -----------------------------------------------------

    const Token &peek(size_t ahead = 0) const;

    const Token &previous() const;

    bool at_end() const;

    const Token &advance();

    bool is_punct(size_t ahead, const char *text) const;

    bool is_word(size_t ahead, const char *text) const;

    bool match_punct(const char *text);

    // --- errors ------------------------------------------------------------

    void error(const Token &at, const std::string &message);

    // Statements are separated by newlines, so anything left on the line after
    // a statement ends is a mistake. Without this check
    // `satellite.nonsense.thing x = 1` parses silently as two statements —
    // a module path, then an assignment — instead of being reported.
    void expect_statement_end();

    bool expect_punct(const char *text, const std::string &what);

    std::string expect_word(const std::string &what);

    // Skip forward until something that can plausibly begin a new construct,
    // so a single bad line does not cascade into a page of errors.
    void synchronize();

    bool starts_construct() const;

    // --- the dispatch tests ------------------------------------------------

    // `satellite` '.' <segment>
    bool at_language_path(const char *segment) const;

    // A satellite-rooted type is exactly three tokens of prefix. This is the
    // whole reason '<' is unambiguous: type namespaces are never value
    // expressions, so a '<' reached from parse_type is always a generic opener
    // and a '<' reached from parse_expression is always less-than.
    bool at_type() const;

    bool at_statement_keyword(const char *name) const;

    void take_statement_keyword();

    Span span_from(size_t first) const;

    // --- types -------------------------------------------------------------

    Type parse_type();

    // --- expressions -------------------------------------------------------

    ExprPtr parse_expression(int min_prec = 1);

    ExprPtr parse_unary();

    ExprPtr parse_postfix();

    // Already past the '['. Distinguishes l[i] from l[a:b], where either bound
    // of a slice may be absent.
    ExprPtr parse_subscript(ExprPtr target, size_t first);

    ExprPtr parse_primary();

    // --- statements --------------------------------------------------------

    StmtPtr parse_block();

    StmtPtr parse_statement();

    // A declaration, an assignment, or a bare expression. Shared with the for
    // header, which is why it does not consume any terminator.
    StmtPtr parse_simple_statement();

    StmtPtr parse_if();

    StmtPtr parse_while();

    StmtPtr parse_for();

    StmtPtr parse_return();

    // --- top level ---------------------------------------------------------

    Include parse_include();

    Capsule parse_capsule();

    // The parameter list, the optional satellite.returns and the body — shared
    // by a capsule, a method and a constructor, which differ only in how they
    // are named and in whether a return type means anything.
    void parse_signature(Capsule &capsule, bool allow_returns);

    // --- spacesuits --------------------------------------------------------

    Spacesuit parse_spacesuit();

    void parse_access_block(Spacesuit &suit);

    void parse_suit_member(Spacesuit &suit, Access access);
};

} // namespace satellite
