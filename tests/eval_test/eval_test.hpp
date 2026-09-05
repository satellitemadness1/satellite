#pragma once

// The harness, and the six sections. Three functions and a counter, no
// framework -- the shape every suite in this tree uses.
//
// WHAT IS BEING PROVED. M9's claim has three parts and they are checked
// separately because each can be true while the others are not:
//
//   1. THE COMPILE IS FAITHFUL. Every kind of node DESIGN §6 can produce
//      becomes an op, and the ones this evaluator does not run become a
//      REFUSAL naming a milestone rather than a wrong answer or a crash.
//   2. THE ANSWERS ARE RIGHT. Arithmetic, comparison, control flow, calls,
//      frames and returns -- the things a person would notice.
//   3. THERE IS NO DEPTH. A capsule that calls itself 100,000 times answers,
//      at the 8 MiB a login shell hands out, and a runaway one is refused in
//      words about RECURSION with a status a script can read.
//
// THE THIRD IS THE MILESTONE AND THE FIRST TWO ARE WHAT MAKE IT WORTH HAVING.
// DESIGN §7.5 says the language has no depth limit; M8.5 made that true of the
// four static passes and this makes it true of the evaluator. An interpreter
// that could not crash and could not add would prove nothing.
//
// THIS BINARY LINKS NO machine_limits, AND THAT IS LOAD-BEARING RATHER THAN
// TIDY. MILESTONES/M8.5.md §4.1 is the receipt: a 20,000-deep resolve fixture
// spent a day passing against a raised RLIMIT_STACK it had never been given,
// because the test binary did not link the module that raises one. Every depth
// fixture below therefore runs against 8 MiB, and an evaluator that still
// recursed on the C++ stack would fail them rather than pass. evaluator/
// machine.hpp's Policy is what makes that possible -- the ceiling is handed in,
// not read.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "evaluator/evaluate.hpp"
#include "evaluator/machine.hpp"
#include "name_resolver/resolve.hpp"
#include "parser/parser.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <vector>

namespace eval_test {

extern int failures;

void check(bool ok, const std::string &what);

// Where example/ is. Taken from argv[1] so the suite runs from anywhere.
extern std::string example_directory;

// A whole program, from text to a machine that can run it.
//
// THE NUMBERING, THE TREE AND THE CLOSURES ALL LIVE HERE AND THEY HAVE TO. A
// user's PathId is valid inside one run (words_runtime.hpp), an op's operands
// index the arena beside it, and Machine holds references to both -- so a
// fixture that let any of the three die would be asking about numbers and
// indices that no longer mean anything.
struct Run {
    satellite::words::Words words;
    satellite::Parse parsed;
    satellite::resolve::Resolved resolved;
    satellite::eval::Program program;

    bool built = false;
};

// Compile a source, with no cache behind it.
void build(const std::string &source, Run &into);

// Run one capsule by name and answer what it returned. `machine` is filled in
// so a caller can ask what stopped it.
satellite::Value call(Run &run, const std::string &capsule,
                      const std::vector<long long> &arguments,
                      unsigned long long ceiling = 1024ull * 1024 * 1024);

// The last run's machine -- what it ended as, what it said, and the high-water
// mark of its control stack.
//
// THE PEAK IS WHAT MAKES "A LOOP COSTS NO DEPTH" CHECKABLE.
// SCRATCH.md/NO_LIMITS.md §2.2 spends a section on it -- "a loop costs zero
// stack depth, the frame is reused every iteration ... a million calls that
// each RETURN is depth 1" -- and that claim is invisible in an answer. A
// million-iteration loop and a ten-iteration loop return the same number; only
// the stack tells them apart.
extern satellite::eval::Ending last_ending;
extern std::vector<satellite::errors::Diagnostic> last_problems;
extern unsigned long long last_peak;

// Whether the compile or the run reported this code.
bool raised(const Run &run, satellite::errors::Code code);
bool ran_into(satellite::errors::Code code);

// A capsule's answer as the text a person would read -- what `satl --call`
// prints, so a fixture asserts on the thing a user sees.
std::string answer_of(Run &run, const std::string &capsule,
                      const std::vector<long long> &arguments);

// Whether `text` holds `needle`.
bool holds(const std::string &text, const std::string &needle);

void section_compile();     // every node kind becomes an op or a named refusal
void section_arithmetic();  // DESIGN §8.1 through the evaluator, and §6.6's table
void section_control();     // if, while, for, and what a condition may be
void section_calls();       // frames, arguments, returns, and DESIGN §7.1's bug
void section_depth();       // the milestone: no depth, and a ceiling in words
void section_dispatch();    // handlers[path_id], the receiver tag, the cache
void section_scalars();     // M11's rows: constants, string and number methods
void section_variant();     // M12's rows: the variant, and what "nothing" is
void section_interrupted(); // M11's fourth Ending, without a signal
void section_clock_and_dice(); // M13's rows: the tiers' shapes, now and sleep

} // namespace eval_test
