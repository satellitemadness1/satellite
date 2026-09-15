#pragma once

// The harness, and the four sections. Three functions and a counter, no
// framework -- the shape every suite in this tree uses.
//
// WHAT IS BEING PROVED. PLAN M19's done-when is two programs, and
// example/persistence.satl is one of them -- so this suite is not a second copy
// of it. What a running program CANNOT check is the half of the milestone that
// refuses: a program that reaches S1201 stops, so a suite is the only place
// where all seven of the S12xx sentences can be raised in one run and the CODE
// asserted rather than the text. section_refusals is that.
//
// AND THE HALF A PROGRAM CANNOT SEE AT ALL: whether a failed open is a VALUE.
// `satellite.file.open("nothing", "read")` looks the same from inside a program
// whether it answered a not-ok handle or refused -- one of those ends the
// program, and a suite that only checked the printed output could not tell the
// difference between "answered false" and "never got there". Every fixture here
// asserts `complained` as well as the answer.
//
// IT RUNS IN A DIRECTORY OF ITS OWN AND REMOVES IT, which is not tidiness: the
// subject is a filesystem, `satellite.file.new` is O_EXCL, and a suite that left
// its files behind would pass once and fail on every run after. That is the
// exact defect PLAN M19 says the acceptance program must not have, applied to
// the thing that checks it.

#include <functional>
#include <string>

namespace file_test {

extern int failures;

void check(bool ok, const std::string &what);

// Everything written to stdout while `body` runs, with the console shut down
// before the descriptor goes back.
std::string capture(const std::function<void()> &body);

bool holds(const std::string &text, const std::string &needle);

// Run a source's `satellite.main`. Answers what it printed; `complained` is set
// when anything refused, and the first diagnostic's code is left in `code`.
std::string run(const std::string &source, bool *complained, int *code);

// A whole program around one body, indented into `satellite.main`.
std::string program(const std::string &body);

// Run a body and answer what it printed, asserting that nothing refused.
std::string ran(const std::string &body, const std::string &what);

// Run a body that is expected to refuse, and assert the code.
void refuses(const std::string &body, int expected, const std::string &what);

void install_every_module();

void section_round_trip(); // the lifecycle, and the failed open that is a value
void section_reading();    // the cursor -- read_line, read_all, and the end
void section_refusals();   // every S12xx code, by number
void section_listing();    // satellite.directory, and satellite.system.delete

} // namespace file_test
