#pragma once

#include "ast.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "satellite_words/words_runtime.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace satellite {

class Parser {
public:
    Parser(const std::vector<Token> &toks, AstArena &arena, words::Words &words, uint32_t file = 0);

    ParseResult run();

    // Declarations & Top-level
    NodeIndex parse_include();
    NodeIndex parse_capsule();
    NodeIndex parse_spacesuit();
    NodeIndex parse_global_decl();
    void parse_signature(CapsuleDecl &capsule, bool allow_returns);
    void parse_access_block(SpacesuitDecl &suit);
    void parse_suit_member(SpacesuitDecl &suit, Access access);

    // Statements
    NodeIndex parse_statement();
    NodeIndex parse_simple_statement();
    NodeIndex parse_block();
    NodeIndex parse_if();
    NodeIndex parse_while();
    NodeIndex parse_for();
    NodeIndex parse_return();

    // Types
    Type parse_type();

    // Expressions (implemented in parser_expr.cpp)
    NodeIndex parse_expression(int min_prec = 1);
    NodeIndex parse_unary();
    NodeIndex parse_postfix();
    NodeIndex parse_primary();
    NodeIndex parse_subscript(NodeIndex target, size_t first);

    // Cursor & Lookahead
    bool at_end() const;
    const Token &peek(size_t offset = 0) const;
    const Token &previous() const;
    const Token &advance();

    bool is_punct(size_t offset, const std::string &text) const;
    bool is_word(size_t offset, const std::string &text) const;
    bool match_punct(const std::string &text);
    bool match_word(const std::string &text);

    bool at_language_path(const std::string &sub) const;
    bool at_statement_keyword(const std::string &kw) const;
    void take_statement_keyword();
    bool at_type() const;

    bool expect_punct(const std::string &text, const std::string &msg = "");
    std::string expect_word(const std::string &msg);
    void expect_statement_end();

    Span span_from(size_t first) const;
    Span span_of(const Token &tok) const;

    void error(const Token &tok, std::string msg);
    void synchronize();

private:
    std::vector<Token> toks_;
    AstArena &arena_;
    words::Words &words_;
    uint32_t file_ = 0;
    size_t pos_ = 0;
    bool panic_ = false;
    std::vector<ParseError> errors_;
};

} // namespace satellite

