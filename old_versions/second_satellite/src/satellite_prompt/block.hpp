#pragma once

// What a typed line IS, decided with the language's own lexer.
//
// THROUGH lex() AND NOT THROUGH A CHARACTER SCAN, and that is the whole point of
// this file. Counting braces by walking bytes means re-implementing what a
// string literal is, what an escape is and where a comment ends -- three rules
// DESIGN §5 already fixes and `lexical_analyzer/` already keeps. A second
// implementation of them would be a second place the language's comment syntax
// is decided, and it would disagree the first time either moved. So: lex the
// line, count `{` and `}` TOKENS, and a brace inside a string is not a brace.
//
// TWO QUESTIONS, ONE PASS, because both are answered from the same token list:
//
//   HOW DEEP does this line leave us -- is the block finished, or does the
//   prompt owe the user a continuation line? Control flow in this language is
//   braced (`satellite.statement.if (x) { ... }`, example/scalars.satl), so a
//   prompt that could not continue a line could not run an `if`.
//
//   WHERE DOES IT GO -- inside the wrapper capsule, or beside it? S0204 names
//   the four things a file holds: satellite.include, satellite.capsule,
//   satellite.spacesuit and satellite.library.<name>. Those four cannot go
//   INSIDE a capsule, and everything else cannot go outside one. The parser
//   already dispatches on segment 1 (DESIGN §6.1) and this asks the same
//   question of the same tokens.

#include <string>

namespace satellite::prompt {

// Where a line belongs in the program the session builds.
enum class Placement {
    Statement,  // inside satellite.main's body -- the ordinary case
    TopLevel,   // beside it: include, capsule, spacesuit, library.<name>
};

struct Scan {
    // How much this line opens (positive) or closes (negative). The prompt adds
    // it to a running depth and keeps asking for lines while that is above zero.
    int depth = 0;

    Placement placement = Placement::Statement;

    // THE LINE IS A HEAD THAT OWES A BODY, and this flag exists because this
    // language puts the brace on its OWN line. Every example in the tree writes
    //
    //     satellite.statement.for (satellite.variable.number i = 0; i < 5; ...)
    //     {
    //
    // so the head line contains no `{` at all and a prompt counting only braces
    // reads it as a finished entry -- and runs a `for` with no body, which fails
    // with the loop variable "not in scope" one line later. Measured exactly
    // that way before this flag existed; MILESTONES/M22.md §3 has the transcript.
    //
    // THE FOUR STATEMENT WORDS ARE ALL OF THEM. `satellite.statement` has
    // exactly four children in words.def -- `if` `1 13 1`, `for` `1 13 2`,
    // `while` `1 13 3`, `else` `1 13 4` -- and every one takes a body, so the
    // test is the segment and not a list of spellings that could fall behind.
    bool opens_body = false;

    // The line did not lex. Reported rather than swallowed: a prompt that
    // silently treated an unterminated string as depth 0 would run half a line.
    bool lex_error = false;

    // Nothing but whitespace and comments. Not an error, and not worth running.
    bool empty = false;

    // For `satellite.library.<name>`, the <name>. Empty for everything else.
    //
    // IT IS HERE BECAUSE THE SAME TEXT IS TWO DIFFERENT STATEMENTS DEPENDING ON
    // WHERE IT SITS, and this scanner cannot see where it sits.
    // `satellite.library.counter = 5` at the top of a file DECLARES a global;
    // the identical line inside a capsule ASSIGNS to one, and both are legal --
    // measured. So a prompt that always placed it at the top level made the
    // second one a redeclaration (S0291) and the user could never change a
    // global they had just made. The name is what lets session.cpp tell them
    // apart: the first mention declares, every later one assigns.
    std::string library_name;
};

Scan scan(const std::string &line);

} // namespace satellite::prompt
