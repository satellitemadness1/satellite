#pragma once

// The satellite parser -- Milestone 4 Prototype.
//
// Recursive descent with 1-token lookahead past dotted paths.
// Builds a contiguous Arena AST with uint32_t indices (no shared_ptr).
// Integrates with words::Words to allocate dynamic PathIds for user capsules,
// spacesuits, and library globals as names are met (PLAN §8.1).
//
// Never throws (DESIGN §5.6).

#include "ast.hpp"
#include "lexer.hpp"
#include "satellite_words/words_runtime.hpp"

#include <string>
#include <vector>

namespace satellite {

struct ParseError {
    std::string message;
    Span span;
};

struct ParseResult {
    Program program;
    std::vector<ParseError> errors;

    bool ok() const { return errors.empty(); }
};

ParseResult parse(const std::string &source, AstArena &arena, words::Words &words, uint32_t file = 0);
ParseResult parse(const std::vector<Token> &tokens, AstArena &arena, words::Words &words, uint32_t file = 0);

std::string format_error(const ParseError &error, const SourceMap &sources);

} // namespace satellite

