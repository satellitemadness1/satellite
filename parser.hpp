#pragma once

#include "ast.hpp"
#include "lexer.hpp"

#include <string>
#include <vector>

// Recursive descent, one token of lookahead past a dotted path, no
// backtracking and no tentative parsing.
//
// That is affordable because of the naming rule rather than by luck. A type
// always starts with `satellite` and a user name never does, so a type and a
// variable can never have the same token shape — which means satellite has
// none of C's "is this a declaration or an expression?" problem, and the
// parser never has to ask a symbol table what a name means. The lexer stays a
// pure function with no feedback edge, and there is no equivalent of clang's
// TryParseDeclarator anywhere in here.
//
// What a statement IS gets decided by segment 1 of its leading path:
//
//     satellite.variable.*     a type; a Word after it makes it a declaration
//     satellite.container.*    likewise
//     satellite.statement.*    if / else / while / for
//     satellite.return(...)    a return
//     satellite.include(...)   an include (top level only)
//     satellite.capsule ...    a capsule definition (top level only)
//     satellite.library.*      a path to a shared variable
//     anything else            a module path; an ordinary expression
//
// Dispatching on shape instead would be wrong, not merely fragile:
// `satellite.statement.if (x)` and `satellite.variable.time my_time` are
// identical in shape, so a structural rule would quietly declare a variable
// named `if`.

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

// Parse a whole program. Never throws. On a syntax error the parser records
// it, skips to the next plausible starting point and keeps going, so one
// missing brace does not bury every later mistake.
ParseResult parse(const std::string &source);
ParseResult parse(const std::vector<Token> &tokens);

// Renders an error with the offending line and a caret under it. Correct
// because token offsets are byte offsets into the source — decode() could not
// be used here, since it is neither injective nor stable.
std::string format_error(const ParseError &error, const std::string &source);

} // namespace satellite
