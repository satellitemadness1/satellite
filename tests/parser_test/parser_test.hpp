#pragma once

// The harness, and the five sections. Three functions and a counter, no
// framework -- the shape tests/words_test and tests/lexer_test already use,
// which is the first satellite's suite ported rather than reinvented.
//
// THIS ONE LINKS MORE THAN ANY TEST BEFORE IT, and that is a property of the
// subject rather than of the test: a parser is a function over a token stream,
// so it needs the lexer, the lexer needs the alphabet, and the tree it builds
// has a printer. 065-tests.mk names those five sources.
//
// WHAT IS BEING PROVED, AND WHY A ROUND-TRIP IS THE FORM OF IT. Nothing in this
// language runs until M8, so a tree cannot be checked by running it. It can be
// checked by printing it back: a parse that dropped an operand, a printer that
// forgot a bracket, and two rules that are not each other's inverse all show up
// as a program that comes back different. section_roundtrip() is that check
// over the four acceptance programs; the other four sections are the forms no
// program in example/ happens to use.

#include "abstract_syntax_tree/ast.hpp"
#include "parser/parser.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <string>

namespace parser_test {

extern int failures;

void check(bool ok, const std::string &what);

// Where example/ is. Taken from argv[1] so the suite runs from anywhere, the
// same argument words_test takes for WORD_NUMBERS.md and for the same reason.
extern std::string example_directory;

// A parse and the numbering it defined names into.
//
// THE TWO TRAVEL TOGETHER BECAUSE ONLY ONE OF THEM HOLDS THE NAMES. A user
// capsule's PathId is in the tree; what that number is spelled and what number
// was free when it was met are in the Words -- and WORD_NUMBERS §3 says the
// number is valid inside one run only, so a test that checked a number without
// the table it came from would be checking an integer against a literal.
struct Program {
    satellite::words::Words words;
    satellite::Parse parse;

    const satellite::Ast &ast() const { return parse.ast; }
    bool ok() const { return parse.ok(); }
    std::string first_error() const;
};

// Parse a whole file.
Program run(const std::string &source);

// Parse statements, by putting them in a capsule first -- DESIGN §6 allows only
// declarations at the top level, so a bare statement has nowhere to be.
Program in_capsule(const std::string &statements);

// The first node of a kind, or kNoNode. Written as a scan of the arena rather
// than as a walk of the tree, which is one of the things a flat arena is for.
satellite::NodeIndex first_of(const satellite::Ast &ast, satellite::NodeKind kind);
size_t count_of(const satellite::Ast &ast, satellite::NodeKind kind);

// One node, printed. `check` messages read better with this in them than with
// a node index.
std::string print(const satellite::Ast &ast, satellite::NodeIndex node);

void section_arena();         // the layout PLAN §2.2 asks for, and the lists
void section_expressions();   // §6.2's postfix loop, precedence, the same-line rule
void section_statements();    // §6.1's dispatch, and the collision it exists for
void section_declarations();  // the four top-level forms, and M2's name allocator
void section_roundtrip();     // the acceptance programs, printed back

} // namespace parser_test
