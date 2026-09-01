// words_test -- the proof that src/satellite_words/ says what WORD_NUMBERS.md
// says. See tests/words_test/words_test.hpp for the harness.
//
// MOST OF THIS TEST ALREADY RAN. words.hpp ends in static_asserts over the
// X-macro lists, so a cycle, a forward-declared parent, a doubled bare shape or
// an alias shadowing a real row fails when this file is COMPILED, with the row
// named. What is left for run time is the half no static_assert can reach --
// and it is the half that matters most here, because it is the transcription.
//
// THE ONE THING A COMPILER CANNOT SEE IS A MISSING ROW. words.def numbers by
// POSITION, so a row left out does not leave a hole: it silently renumbers every
// sibling after it, and both files stay internally consistent while meaning
// different things. Nothing in the C++ can notice. The only thing that can is a
// comparison against the authority, which is section_authority() -- it opens
// WORD_NUMBERS.md §2.2 and walks all 223 of its paths.
//
// So this binary READS A MARKDOWN FILE, which is unusual and is the point.
// WORD_NUMBERS.md is not documentation about the numbering; it IS the
// numbering, and the project's rule is that prose may explain a number but may
// never be the only place the number lives. Checking the code against the
// authority is that rule pointed at the code.

#include "words_test.hpp"

#include <cstdio>

namespace words_test {

int failures = 0;
std::string authority_path = "WORD_NUMBERS.md";

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

} // namespace words_test

int main(int argc, char **argv)
{
    if (argc > 1)
        words_test::authority_path = argv[1];

    words_test::section_authority();
    words_test::section_walking();
    words_test::section_runtime();

    if (words_test::failures) {
        printf("words_test: %d failure(s)\n", words_test::failures);
        return 1;
    }
    printf("words_test: ok\n");
    return 0;
}
