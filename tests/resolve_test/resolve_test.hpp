#pragma once

// The harness, and the five sections. Three functions and a counter, no
// framework -- the shape words_test, lexer_test, parser_test, satc_test,
// reporter_test and limits_test already use.
//
// WHAT IS BEING PROVED. DESIGN §7's claim is not that a program compiles; it is
// that every name in it has somewhere to LIVE that is private to one call. So
// the sections below check the four things that claim decomposes into: that a
// name reaches the right slot and the slots are per-capsule, that a
// redeclaration takes a FRESH one (§7.4), that every path becomes the number
// WORD_NUMBERS §2.2 says it is, and that every row of `errors.def`'s S05xx
// block is reachable from a real program.
//
// ONE ASSERTION PER ROW OF errors.def, THROUGH THE REAL ENTRY POINT, which is
// the rule tests/reporter_test/parsing.cpp established and SESSION.md §5.20 is
// the reason for: a site that raises a NEIGHBOURING code renders perfectly and
// no assert can see it. Every check below therefore names the code it expects
// and runs the source through resolve() rather than calling the function that
// reports it.
//
// AND THE COUNTS ARE ASSERTED, which is the clause MILESTONES/M4.5.md §5 asks
// for and the one that cannot be tested any other way. A cache that stopped
// being read still produces the right numbers -- the walk is the fallback, and
// it is correct -- so the only thing that can catch the skip disappearing is
// the count of walks it saved. section_cache is that.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "name_resolver/resolve.hpp"
#include "parser/parser.hpp"
#include "satellite_words/words.hpp"

#include <string>
#include <vector>

namespace resolve_test {

extern int failures;

void check(bool ok, const std::string &what);

// Where example/ is. Taken from argv[1] so the suite runs from anywhere, the
// same argument words_test takes for WORD_NUMBERS.md and for the same reason.
extern std::string example_directory;

// A parse and its resolve, kept together because every assertion needs both --
// a slot means nothing without the tree the node is in.
//
// THE NUMBERING IS HERE TOO AND IT HAS TO BE. A user's PathId is valid inside
// ONE RUN (words_runtime.hpp's table), so a test that let the Words object die
// would be asking about numbers that no longer mean anything.
struct Run {
    satellite::words::Words words;
    satellite::Parse parsed;
    satellite::resolve::Resolved resolved;

    // Whether the source parsed at all. A resolve assertion over a tree that
    // did not parse is an assertion about a fixture typo.
    bool parsed_clean() const { return parsed.ok(); }
};

// Resolve a source, from text, with no cache behind it.
void resolve_source(const std::string &source, Run &into);

// Whether `text` holds `needle` -- so a check names the thing it is looking for
// rather than a character offset.
bool holds(const std::string &text, const std::string &needle);

// Whether the run reported exactly this code, and nothing that is not a note.
bool only_problem(const Run &run, satellite::errors::Code code);

// Whether the run reported this code at all.
bool raised(const Run &run, satellite::errors::Code code);

// The frame of the capsule spelled `name`, or null.
const satellite::resolve::Frame *frame_of(const Run &run, const std::string &name);

// Every path the run resolved to, as WORD_NUMBERS §2.2 spells the number --
// "1 5 1". Used to assert that a program's numbers are the numbering's.
std::vector<std::string> numbers_of(const Run &run);

// Whether the run resolved something to this number.
bool resolved_to(const Run &run, const std::string &number);

// How the run arrived at this number -- walked, taken from the `.satc`, or
// already in the tree. Parsed when nothing resolved to it.
//
// PER PATH AND NOT ONLY PER RUN, which is what mutation testing asked for. The
// first version of section_cache asserted the TOTALS, and disabling half the
// skip -- the chain half, leaving the type half alone -- walked straight
// through every one of those assertions: the counts still moved in the right
// direction, and they still added up. A count over a whole run cannot see which
// of two mechanisms produced it.
satellite::resolve::Origin origin_of(const Run &run, const std::string &number);

void section_frames();     // §7.2 and §7.4 -- slots, and the fresh one
void section_names();      // what a name reaches, and what it does not
void section_paths();      // DESIGN §6.3's walk, and the numbers it arrives at
void section_arguments();  // §7.7, its six spellings, and the seventh
void section_cache();      // MILESTONES/M4.5.md §5's clause, counted
void section_examples();   // the acceptance programs, through the real command

} // namespace resolve_test
