#pragma once

// WHICH REGISTRY NUMBER IS WHICH FACT -- the module's one table, in the one
// place both of its consumers can reach.
//
// INTERNAL TO satellite_arguments/, and it is split out of handlers.cpp for a
// link reason rather than a size one. `satellite_value/render.cpp` renders the
// object -- DESIGN §7.7's "displaying it bare prints all of it" -- so it needs
// the answers; if they lived beside the handlers, every binary that links the
// value renderer would have to link the evaluator's dispatch table with them.
// Nothing in this file knows what a Machine is, and that is what keeps
// tests/number_test off evaluator/.

#include "satellite_value/value.hpp"
#include "satellite_value/value_arguments.hpp"
#include "satellite_words/words.hpp"
#include "system_facts/arguments_facts.hpp"

#include <cstddef>
#include <string>

namespace satellite::arguments {

// A fact, the number that names it, and where in the answers it is.
//
// EXACTLY ONE OF `text` AND `count` IS SET, and which it is, is a property of
// the FACT and not of the value -- DESIGN §7.7 writes `cores` and `threads` as
// "a satellite number", and a count kept as text and parsed back would be the
// conversion M8 spent a milestone not having.
struct Row {
    words::NodeId path;
    std::string facts::MachineAnswers::*text = nullptr;
    unsigned long long facts::MachineAnswers::*count = nullptr;
};

// THIRTY-TWO, AND THE NUMBER IS CHECKED RATHER THAN REPEATED. It is written
// here because handlers.cpp needs it at compile time -- one handler per row,
// expanded from an index sequence -- and rows.cpp static_asserts the table
// against it, so a row appended without this line moving is a build error and
// not a fact that quietly stops being dispatched.
inline constexpr std::size_t kRowCount = 32;
extern const Row kRows[kRowCount];

// One row's answer. THE FIRST CALL IS WHAT BUILDS THE THIRTY-TWO -- every
// reader goes through facts::machine_answers(), so the deferral lives in one
// place and no path can read a fact without paying for the assembly once.
Value answer_for(const Row &row);

// Any child of the object that has an answer, by number: the thirty-two the
// MACHINE knows, plus the two the OBJECT knows -- `session.directory`, which
// is where the program was started, and `count`, which is how many words were
// on the command line. False for the two rows that are M25's, which is what
// keeps them out of `session`'s map as well as off the dispatch table.
bool answer_of(words::PathId path, Value *out);

// The whole object as the text `satellite.console.display(arguments)` prints
// -- DESIGN §7.7. satellite_value/render.cpp's arm is one line calling this.
std::string object_text(const Arguments &body);

} // namespace satellite::arguments
