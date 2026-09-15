// lexer_test -- the proof that src/lexical_analyzer/ does what DESIGN §5 says.
// See tests/lexer_test/lexer_test.hpp for the harness.
//
// WHAT THIS TEST IS FOR IS THE FIVE INHERITED RULES. DESIGN §5 opens by saying
// its rules "each fixed a verified defect in the first satellite's original
// plan -- they are inherited as rules, not rediscovered". A rule inherited and
// not checked is a rule that gets rediscovered, so every one of the five has a
// check here that names it: §5.1 underscore, §5.2 whitespace in the raw area,
// §5.3 lex raw, §5.4 real escapes, §5.5 no `<<` and no `>>`.
//
// AND FOR THE ONE DISTINCTION NO COMPILER CAN SEE. A SpellingId and a PathId
// are the same 32 bits -- words_spellings.hpp says `using SpellingId = PathId`
// -- so a lexer that hands the parser the wrong one of the two compiles clean
// and dispatches on a number that means something else. section_spellings()
// checks it the only way it can be checked: `console` is spelled by TWO nodes,
// so its spelling id is one number and its two paths are two others.

#include "lexer_test.hpp"

#include <cstdio>

namespace lexer_test {

int failures = 0;
std::string example_path = "example/hello_world.satl";

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

} // namespace lexer_test

int main(int argc, char **argv)
{
    if (argc > 1)
        lexer_test::example_path = argv[1];

    lexer_test::section_characters();
    lexer_test::section_literals();
    lexer_test::section_spellings();
    lexer_test::section_spans();

    if (lexer_test::failures) {
        printf("lexer_test: %d failure(s)\n", lexer_test::failures);
        return 1;
    }
    printf("lexer_test: ok\n");
    return 0;
}
