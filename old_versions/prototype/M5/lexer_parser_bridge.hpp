#pragma once

// Lexer & Parser Diagnostic Bridge -- Milestone 5 Error Reporter.
//
// Bridges M3 Lexer tokens and M4 Parser errors into structured M5 Diagnostics.
// Integrates with M2 Words trie to perform path validation and did-you-mean
// suggestions over failing trie nodes (DESIGN §4.6).

#include "diagnostic.hpp"
#include "reporter.hpp"
#include "source_view.hpp"
#include "lexer.hpp"
#include "ast.hpp"
#include "parser.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace satellite {

// Validates a dotted path against M2 Words trie and emits rich diagnostics on failure.
Diagnostic validate_path(
    std::string_view path,
    Span base_span = {},
    words::Words *runtime_words = nullptr);

// Translates an M3 Lexer error token into an M5 Diagnostic.
Diagnostic translate_lexer_error(const Token &tok, uint32_t file_id = 0);

// Translates an M4 ParseError into an M5 Diagnostic.
Diagnostic translate_parser_error(const ParseError &err, const std::string &source = "");

// High-level driver: lexes, parses, validates paths, and collects all diagnostics.
struct CompileAnalysis {
    bool ok = false;
    Program program;
    AstArena arena;
    words::Words words;
    SourceMap sources;
    DiagnosticReporter reporter;
};

CompileAnalysis diagnose_source(
    const std::string &source,
    const std::string &filename = "input.satl");

} // namespace satellite

