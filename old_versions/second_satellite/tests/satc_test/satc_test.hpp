#pragma once

// The harness, and the four sections. Three functions and a counter, no
// framework -- the shape tests/words_test, tests/lexer_test and
// tests/parser_test already use.
//
// WHAT IS BEING PROVED. A `.satc` is a program with its language-owned words
// replaced by numbers, and the failure that matters is not a crash: it is a
// file that still parses and MEANS SOMETHING ELSE. This suite was written
// alongside the writer and caught exactly that on its first run --
// `satellite.console.input(">>>", target)` written as
// `satellite.console.display`, with the arguments intact -- so every section
// below asserts on the NUMBER a form comes out as, never on the fact that a
// number came out.
//
// IT ASSERTS ON RENDERED TEXT rather than on the matcher's return value, and
// that is deliberate. paths.hpp's PathMatch is an implementation detail that
// M7 may well take over; SATC.md §1.1's file is the thing that has to be
// stable, because it is what a reader reads and what a later satl has to be
// able to open. A test written against the struct would have passed while the
// file said something else.

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satc_test {

extern int failures;

void check(bool ok, const std::string &what);

// Where example/ is. Taken from argv[1] so the suite runs from anywhere, the
// same argument words_test takes for WORD_NUMBERS.md and for the same reason.
extern std::string example_directory;

// A whole program, parsed and written. Fails loudly rather than returning
// something plausible when the fixture does not parse -- a test whose fixture
// is broken reports the writer as wrong.
std::string body(const std::string &source);

// Statements, wrapped in a capsule first: DESIGN §6 allows only declarations at
// the top level, so a bare statement has nowhere to be. What comes back is the
// written body of the capsule alone, one statement per line, with the wrapper
// removed -- so a check reads as the statement it is about.
std::string statements(const std::string &source);

// One statement in, the line it came out as -- the WHOLE line, because a check
// that only asked whether "1.5.4" appeared would pass over `1.5.4` printed with
// the wrong arguments beside it.
std::string one_line(const std::string &statement);

// One of the programs in example/, or false. Here rather than in the section
// that reads them because two sections now do -- section_examples() writes
// every acceptance program, and section_reading() reads each one back.
bool read_example(const std::string &name, std::string &into);

// The one line of `text` that contains `needle`, without its indent, or "" --
// so a check names the form it is about instead of a line number.
std::string line_with(const std::string &text, const std::string &needle);

// The comment on a line, or "" -- SATC.md §1.1's column, which is what a person
// checks a number against and is therefore worth checking itself.
std::string comment_of(const std::string &line);

// The line without its comment and trailing spaces: the part that IS the file.
std::string code_of(const std::string &line);

void section_shapes();      // §5.1 steps 1, 3 and 4 -- aliases, absorbing, arity
void section_ownership();   // §3 and §3.1 -- what may NOT become a number
void section_examples();    // every acceptance program, and §5.2's stability
void section_header();      // §2's three lines
void section_reading();     // §4's reading order, and the fixpoint
void section_writing();     // §5's write: tmp, fsync, rename, and the thread
void section_depth();       // M8.5: the depths a program may be written at

} // namespace satc_test
