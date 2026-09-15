#pragma once

// The harness, and the sections. Three functions and a counter, no framework --
// the same shape tests/words_test uses, which is the first satellite's suite
// ported rather than reinvented.
//
// UNLIKE words_test, THIS ONE LINKS OBJECTS. The registry is constexpr data and
// pure functions, so its test compiles the headers and links nothing; a lexer
// is a function over a string and has to actually run. 065-tests.mk names the
// two .cpp files it needs, and the reason it can name them rather than wildcard
// them is that a lexer's dependencies are exactly its alphabet.

#include <string>

namespace lexer_test {

extern int failures;

void check(bool ok, const std::string &what);

// Where example/hello_world.satl is. Set from argv[1] so the suite can run from
// somewhere other than the tree root -- the same argument words_test takes for
// WORD_NUMBERS.md, and for the same reason.
extern std::string example_path;

void section_characters();  // DESIGN §5.1, §5.2, §5.5 and the comment rule
void section_literals();    // numbers, strings, escapes, and §8.5's Bits
void section_spellings();   // the words half: identity, aliases, §2's reserved word
void section_spans();       // spans, lines, errors, and the acceptance program

} // namespace lexer_test
