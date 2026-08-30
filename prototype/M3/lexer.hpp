#pragma once

// The lexical analyzer prototype -- Milestone 3.
//
// Turns satellite source into a flat vector of tokens.
// Known words carry their PathId from M2's words registry;
// user-owned bare words carry their text and kNoPath.
//
// Never throws (DESIGN §5.6).

#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace satellite {

enum class TokenKind : uint8_t {
    Word,     // satellite, my_time, _leading, x2_y
    Number,   // 3, 3.14
    Bits,     // x00FF, b1010 -- DESIGN §8.5
    String,   // "hello, world!"
    Punct,    // . = ( ) [ ] { } < > , : + - * / % == <= >= != ; &
    Newline,  // statement terminator -- DESIGN §6
    End,      // always the last token
    Error,    // lexing stopped; text holds the reason
};

struct Token {
    TokenKind kind = TokenKind::End;

    // Word:   the identifier spelling, e.g. "satellite" or "my_time"
    // Punct:  the operator spelling,   e.g. "." or ">="
    // Number: the digits as written,   e.g. "3.14"
    // String: the body without quotes, unescaped raw in `text`, expanded in `str`
    // Bits:   the literal as written,  e.g. "x00FF" or "b1010"
    // Error:  human-readable error description
    std::string text;

    // String literals only: body with escapes expanded.
    SatString str;

    // Bits only: 2 for `b` literal, 16 for `x` literal.
    unsigned radix = 0;

    // Byte offsets in original source.
    uint32_t start = 0;
    uint32_t end = 0;

    // 1-based line number.
    uint32_t line = 1;

    // Word identity from M2's registry (DESIGN §5.6).
    // words::kNoPath for user-owned names.
    words::PathId word_id = words::kNoPath;
};

std::vector<Token> lex(const std::string &source);
std::vector<Token> lex(const SatString &source);

const char *kind_name(TokenKind kind);
std::string describe(const Token &token);

bool split_punct(std::vector<Token> &tokens, size_t index);

} // namespace satellite

