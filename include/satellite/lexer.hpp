// The SATELLITE lexer.
//
// Two rules give the language its character, and both live here:
//
//   1. `satellite` is the ONLY reserved word -- and only when it is not
//      preceded by a dot.  After a dot it is an ordinary identifier, which is
//      what makes the escape hatch work:
//
//          satellite.library.my_capsule.satellite
//          ^keyword                     ^just a name
//
//      Every other word in the language is free for the user.
//
//   2. A newline ends a statement, but only when the previous token could
//      actually finish one.  A line ending in `+` or `(` continues.  This is
//      Go's rule; it avoids the traps JavaScript's version is known for.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "satellite/container.hpp"

namespace satellite {

enum class Tok : uint8_t {
    End,          // end of input
    Term,         // statement terminator (a newline that counted)
    Satellite,    // the one reserved word
    Ident,
    Int,
    Real,
    Str,

    Dot, Comma, Colon,
    LParen, RParen,
    LBrace, RBrace,
    LBracket, RBracket,

    Plus, Minus, Star, Slash, Percent,
    Assign,
    Eq, Ne, Lt, Gt, Le, Ge,
    And, Or, Not,

    Unknown
};

const char* tok_name(Tok t);

struct Token {
    Tok         kind = Tok::End;
    std::string text;      // identifier name, or the decoded string body
    int64_t     ival = 0;
    double      dval = 0;
    int         line = 0;
    int         col  = 0;
};

struct LexError {
    std::string message;
    int line = 0;
    int col  = 0;
};

class Lexer {
public:
    explicit Lexer(std::string source) : src_(std::move(source)) {}

    // Tokenizes the whole input.  Always terminates; errors are collected
    // rather than thrown, so one bad string literal does not hide the rest.
    std::vector<Token> scan();

    const std::vector<LexError>& errors() const { return errors_; }
    bool ok() const { return errors_.empty(); }

private:
    std::string           src_;
    size_t                pos_  = 0;
    int                   line_ = 1;
    int                   col_  = 1;
    std::vector<LexError> errors_;

    bool at_end() const { return pos_ >= src_.size(); }
    char peek(size_t ahead = 0) const {
        return pos_ + ahead < src_.size() ? src_[pos_ + ahead] : '\0';
    }
    char advance();
    bool match(char c);
    void error(const std::string& msg, int l, int c);

    void scan_number(std::vector<Token>& out);
    void scan_string(std::vector<Token>& out);
    void scan_word(std::vector<Token>& out, bool after_dot);
};

// True when a newline following this token should end the statement.
bool ends_statement(Tok t);

// The satellite math token for an operator, or 0 if this token is not one.
// This is where the ambiguity is resolved for good: a '-' the lexer saw INSIDE
// a string literal never reaches here -- it was already stored as text code 73
// by scan_string.  A '-' outside one arrives as Tok::Minus and becomes code 99.
char32_t op_code_for(Tok t);

}  // namespace satellite
