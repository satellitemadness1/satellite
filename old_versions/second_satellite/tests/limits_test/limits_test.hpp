#pragma once

// The harness, and the four sections. Three functions and a counter, no
// framework -- the shape words_test, lexer_test, parser_test, satc_test and
// reporter_test already use.
//
// WHAT IS BEING PROVED. M6 has four halves and each is checkable in a different
// way, which is why the sections are what they are rather than one per file:
//
//   the file      read_config() over strings and over the two files in
//                 example/, checking the VALUE, the UNIT, the ORIGIN and the
//                 CODE -- one assertion per row of errors.def's S08xx block,
//                 which is the check MILESTONES/M5.md §5 says a message
//                 registry needs and bespoke strings could not have.
//   the readers   the machine's own answers, checked against each other and
//                 against /proc, because there is nothing else to check them
//                 against -- see section_facts for what that can and cannot
//                 catch.
//   the pool      that it builds, that it parks, that the floor is enforced,
//                 and that a batch computes the right answer. Nothing in the
//                 tree calls run_over() yet, so this suite IS the pool's only
//                 caller and a batch runner nothing has run is one that does
//                 not work.
//   the dials     that the four names in this file are the four in words.def,
//                 which is the transcription check words_test makes about
//                 WORD_NUMBERS.md, one registry over.
//
// THE WATCHDOG IS NOT IN HERE, AND THAT IS THE ONE GAP WORTH NAMING. Its
// demonstration is a process that dies -- `satl --watchdog` with MEMORY_MAX
// below what satl is already using, exit 4 in about a second -- and a unit test
// cannot make that assertion without forking a binary and waiting a second,
// which is what MILESTONES/M6.md §7 records as the check that lives in the
// milestone note rather than in the suite. What IS checked here is everything
// the watchdog reads: process_memory_bytes(), mem_available_mb() and the dial.

#include "error_reporter/report.hpp"
#include "machine_limits/limits.hpp"

#include <string>
#include <vector>

namespace limits_test {

extern int failures;

void check(bool ok, const std::string &what);

// Where example/ is. Taken from argv[1] so the suite runs from anywhere, the
// same argument words_test takes for WORD_NUMBERS.md and for the same reason.
extern std::string example_directory;

// Read a config from TEXT, and say what came out. `problems` is filled either
// way, so a caller can check the codes on a file that failed and the values on
// one that did not.
bool reads(const std::string &text, satellite::limits::Held &into,
           std::vector<satellite::errors::Diagnostic> &problems);

// The codes a text raises, in order. What one assertion per errors.def row is
// written against.
std::vector<satellite::errors::Code> codes_in(const std::string &text);

// The whole of a file in example/, or "" -- and a test whose subject is a file
// must FAIL LOUDLY when it cannot read that file rather than skipping, which is
// the rule words_test set for WORD_NUMBERS.md and lexer_test kept.
std::string example(const std::string &name);

// Called in THIS order, and the last two are load-bearing -- limits_test.cpp
// says why.
void section_reading();   // the file: values, units, origins, and every code
void section_facts();     // the machine readers, against each other and /proc
void section_pool();      // the floor, a real batch, and the pool parking
void section_examples();  // the two files in example/, and the clamp

} // namespace limits_test
