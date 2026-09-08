#pragma once

// The harness, and the three sections. Three functions and a counter, no
// framework -- the shape every suite in this tree uses.
//
// WHAT IS BEING PROVED. PLAN M18's done-when asks for two things and calls them
// self-verifying, "which no earlier milestone is":
//
//   1. THE OUTPUT IS COMPARABLE TO THE NON-NULL ENTRIES OF handlers[] BY
//      CONSTRUCTION. Every row in that table is named by help, nothing help
//      names is missing from the four kinds that make a word built, and the
//      answer is swept over ALL 264 nodes rather than sampled -- section
//      predicate. A test that checked the predicate against a restatement of
//      the predicate would prove nothing; what is compared here is help's
//      ANSWER, node by node, against the tables it is supposed to be about.
//   2. `satellite.help(satellite.network)` REFUSES IN PLAIN WORDS rather than
//      printing seven shapes nobody has written -- section walking, and it is
//      swept too: every unbuilt node refuses and every built one answers.
//
// AND ONE THE DONE-WHEN DOES NOT ASK FOR, because it could not have known it
// was needed: the ARGUMENT IS NOT EVALUATED. Before M18 the second check passed
// through the wrong mechanism -- S0721, raised because the argument itself
// dispatched and died -- and gave the same answer for a module that IS built.
// section walking asserts the code as well as the refusal, so the check cannot
// pass that way again.
//
// IT REDIRECTS ITS OWN STDOUT for console_test's reason exactly: help's whole
// answer is bytes reaching descriptor 1 through DESIGN §10.1's printer thread,
// and a fake sink would be a test of the walk with the half that can fail
// removed.

#include <functional>
#include <string>

namespace help_test {

extern int failures;

void check(bool ok, const std::string &what);

// Everything written to stdout while `body` runs, with the console shut down
// before the descriptor goes back.
std::string capture(const std::function<void()> &body);

// Whether `text` holds `needle`.
bool holds(const std::string &text, const std::string &needle);

// Run a source's `satellite.main`. Answers what it printed; `complained` is set
// when the machine refused, and the diagnostic's code is left in `code`.
std::string run(const std::string &source, bool *complained, int *code);

// Every module's rows in the dispatch table, help's included. Defined beside
// the predicate section, which is what needs them first.
void install_every_module();

void section_predicate(); // built() over the four kinds, swept over all 264
void section_entries();   // one entry per node, and the groups partition
void section_walking();   // the three shapes, and the refusals, through satl

} // namespace help_test
