#pragma once

#include "bignum.hpp"
#include "satellite_string.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace satellite {

// The lexer turns satellite source into a flat token vector. It knows nothing
// about the language: 'satellite', 'variable' and 'capsule' are ordinary Words
// and the parser is what gives them meaning. There is deliberately no keyword
// enum — the one reserved word the language has ('satellite') is enforced at
// the declaration site, not here.
enum class TokenKind {
    Word,     // satellite, my_time, _leading, x2_y
    Number,   // 3, 3.14
    String,   // "hello, world!"
    Punct,    // . = ( ) [ ] { } < > , : == <= >= !=
    End,      // always the last token
    Error,    // lexing stopped; text holds the reason
};

struct Token {
    TokenKind kind = TokenKind::End;

    // Word:   the identifier spelling, e.g. "satellite" or "my_time"
    // Punct:  the operator spelling,   e.g. "." or ">="
    // Number: the digits as written,   e.g. "3.14"
    // String: the body exactly as written, without the quotes and with
    //         escapes NOT expanded — the expanded value is in `str`. Both are
    //         kept because unparse needs the spelling: "a\nb" must come back
    //         as a backslash and an n, not as a real newline.
    // Error:  a human-readable message
    //
    // Safe to keep as plain text because a Word can only hold letters, digits
    // and underscore, and those decode back one-for-one. The same is not true
    // of satellite strings, which is why String bodies stay encoded.
    std::string text;

    // String literals only: the body with escapes already expanded.
    SatString str;

    // Number only, and exact: the digits as written become an exact decimal
    // with nothing rounded on the way in (§8.1). strtod here would have thrown
    // away precision before the parser ever saw the literal.
    Number number;

    // Half-open [start, end) offsets into the source. Because encode_raw()
    // emits exactly one SatChar per input byte, these are also byte offsets
    // into the original std::string — which is what error carets need, and
    // why carets must never be computed by decoding.
    size_t start = 0;
    size_t end = 0;

    // 1-based. Load-bearing beyond diagnostics: the parser needs it to require
    // that a postfix '[' sits on the same line as what it subscripts, so that
    // a line starting with a list literal is not silently absorbed as a
    // subscript of the line above.
    size_t line = 1;
};

// Lex satellite source. Never throws.
//
// The result always ends with an End token. If lexing stopped early, an Error
// token holding the reason sits immediately before it, so a parser can always
// peek one token past whatever it is looking at.
std::vector<Token> lex(const std::string &source);

// Same, for source already converted with encode_raw().
std::vector<Token> lex(const SatString &source);

const char *kind_name(TokenKind kind);

// "Word(satellite)", "Punct(>=)", "Number(3.14)" — for tests and diagnostics.
std::string describe(const Token &token);

// Split a two-character Punct into two one-character Puncts, inserting the
// second after it. Returns false if that token is not a two-character Punct.
//
// This exists for one case: a generic close is a single '>', but the lexer
// matches '>=' greedily without knowing it is inside a type. A parser that
// wants a '>' and finds '>=' calls this and takes the '>'. Unreachable in the
// v1 grammar, since a type is always followed by a name, but the guard costs
// nothing and the alternative is a baffling parse error years from now.
bool split_punct(std::vector<Token> &tokens, size_t index);

} // namespace satellite
