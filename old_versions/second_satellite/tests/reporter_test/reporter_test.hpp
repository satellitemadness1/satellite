#pragma once

// The harness, and the five sections. Three functions and a counter, no
// framework -- the shape tests/words_test, lexer_test, parser_test and
// satc_test already use.
//
// WHAT IS BEING PROVED, AND WHY IT IS NOT "the message is right". A message
// cannot be tested for being useful; it can be tested for being ADDRESSABLE,
// which is what DESIGN §9 says the first satellite's were not: "a code to look
// up, a stack to place it in, a suggestion to act on." So the sections below
// check that a code exists and is stable, that the block a diagnostic renders
// to puts the caret under the right character, that the suggester answers the
// question DESIGN §4.6 poses, and that the two passes which now report through
// this module actually produce the codes they claim to.
//
// AND IT RENDERS DIAGNOSTICS NOTHING PRODUCES, deliberately. A call stack has
// no producer until M9 and a note with no span is only reachable from the
// `.satc` reader, so those arms of the renderer would otherwise be written and
// never run -- which is the same as not having written them. section_rendering
// builds them by hand, which is possible only because the reporter takes three
// integers and a string and knows nothing about a tree.

#include "error_reporter/report.hpp"

#include <string>
#include <vector>

namespace reporter_test {

extern int failures;

void check(bool ok, const std::string &what);

// Where example/ is. Taken from argv[1] so the suite runs from anywhere, the
// same argument words_test takes for WORD_NUMBERS.md and for the same reason.
extern std::string example_directory;

// Whether `text` holds `needle` -- so a check names the thing it is looking for
// rather than a character offset.
bool holds(const std::string &text, const std::string &needle);

// The `n`th line of a rendered block, counting from 0, or "".
std::string line(const std::string &text, size_t n);

// Every diagnostic a source produces, through the ordinary parse.
std::vector<satellite::errors::Diagnostic> problems_in(const std::string &source);

// The whole rendered block for a source, exactly as `satl --check` prints it.
std::string rendered(const std::string &source, const std::string &path = "t.satl");

// The two rows of an excerpt, built rather than written out.
//
// A CHECK THAT SPELLS THE GUTTER OUT IS A CHECK ABOUT THE GUTTER, and every one
// of them would have to be edited the day the margin changes -- which happened
// once while this suite was being written, and turned nine real assertions into
// nine assertions about spaces. These two say "line 3 reads `three four`" and
// "the caret is at column 7", which is what the checks are actually about.
std::string source_row(unsigned line_number, const std::string &text);
std::string caret_row(size_t column, size_t width = 1);

void section_codes();       // the registry: the numbers, the blocks, the holes
void section_rendering();   // DESIGN §9's block, including the arms nothing raises
void section_suggesting();  // DESIGN §4.6, and the transposition that earns it
void section_lexing();      // the one lexical error, its span and its note
void section_parsing();     // the parser's codes, its notes and its suggestions

} // namespace reporter_test
