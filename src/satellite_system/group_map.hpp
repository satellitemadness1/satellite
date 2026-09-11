#pragma once

// A GROUP WORD, DISPLAYING ITS CHILDREN -- the shape the author chose for the
// arguments object's groups on 2026-09-11 and chose again for
// `satellite.system.memory()` the same day.
//
// A FILE OF ITS OWN BECAUSE IT IS A SUBJECT AND NOT A ROW. `satellite.system`
// has four group words whose bare shapes were numbered at the 2026-08-28
// transcription -- `system()` `1 22 0`, `memory()` `1 22 4 0`, `swap()`
// `1 22 4 4 0` and `this()` `1 22 4 5 0` -- and each one that gets an answer
// gets it the same way. `memory_methods.cpp` was at 352 lines with this inside
// it, which is PLAN §3's target saying the same thing.

#include "evaluator/dispatch.hpp"
#include "satellite_value/value.hpp"
#include "satellite_words/words.hpp"

namespace satellite::eval { class Machine; }

namespace satellite::system {

// Every child of `group` that answers with no arguments, as a map from the
// child's word to its answer. False when a child's handler refused, which it
// has already reported.
bool children_of(eval::Machine &machine, words::NodeId group, Value *answer);

// One handler per group, resolved at compile time -- a handler is a plain
// function pointer and is handed no path to look itself up by, which is the
// same arrangement satellite_arguments/handlers.cpp makes for its six.
template <words::NodeId Group>
bool display_children(eval::Machine &machine, const Value *, uint32_t,
                      Value *answer)
{
    return children_of(machine, Group, answer);
}

} // namespace satellite::system
