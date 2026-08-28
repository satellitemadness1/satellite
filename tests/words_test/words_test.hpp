#pragma once

// The harness, and the sections. Three functions and a counter, no framework --
// the shape the first satellite's suite used for nineteen test binaries, which
// is worth porting rather than reinventing.
//
// THE SECTIONS ARE DECLARED HERE AND DEFINED IN THEIR OWN FILES, so a change to
// this header relinks the binary. 065-tests.mk lists the headers as a separate
// wildcard for that reason: they are a DEPENDENCY and not an input, and the
// first satellite found the difference the hard way when a split test grew a
// header and a change to it stopped relinking anything.

#include <string>

namespace words_test {

extern int failures;

void check(bool ok, const std::string &what);

// Where WORD_NUMBERS.md is. Set from argv[1] if given, so the suite can be run
// from somewhere other than the tree root.
extern std::string authority_path;

void section_authority();   // all 222 rows of §2.2, and §2.3's aliases
void section_walking();     // the worked examples, PathIds, and failure reports
void section_runtime();     // the live child counter and user names

} // namespace words_test
