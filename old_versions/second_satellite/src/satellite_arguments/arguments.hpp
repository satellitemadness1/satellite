#pragma once

// `satellite.library.main.arguments` -- DESIGN §7.7, PLAN M20. The object the
// language hands `satellite.main`, and the thirty-six paths under it.
//
// A MODULE OVER TWO SUBJECTS AND THEY ARE DIFFERENT ONES, which is why the
// directory has two files. arguments.cpp is the OBJECT: the command line, the
// directory it was started in, and the one `Value` a run ever builds of it.
// handlers.cpp is the LANGUAGE OVER IT: which registry number answers which
// fact, the six groups that display their children, and the two rows that say
// they are M25's. `system_facts/arguments_facts.hpp` is the layer below both
// and knows nothing about satellite at all.
//
// THE OBJECT IS PROCESS-WIDE AND THE FACTS ARE READ WITHOUT IT. A fact under
// `arguments` compiles to the same `op_dispatch` a module constant does --
// evaluator/compile_expressions.cpp's "a language path read without being
// called" -- so the handlers below take no receiver and answer from here.
// That is not a shortcut around the value: there is exactly one command line
// and one machine per run, so a per-receiver read would be reading the same
// thing through a longer path. The VALUE exists for what needs a value:
// `display(arguments)`, `arguments[i]`, and handing the object to a capsule.

#include "satellite_value/value.hpp"

#include <string>
#include <vector>

namespace satellite::arguments {

// Build the one object, from the words `satl` was started with. argv[0] is the
// program and the rest are the user's, in order.
//
// CALLED BEFORE ANYTHING RUNS, AND IT COSTS NO SYSCALL BUT ONE. The command
// line is already in hand and `getcwd()` is the one fact that cannot wait --
// value_arguments.hpp carries why. Everything else the object answers is
// deferred to the first ask, in system_facts/arguments_facts.cpp.
//
// ONCE PER RUN, WHICH IS ONCE PER PROCESS EXCEPT AT THE PROMPT: each
// `run <file> a b` there is a command line of its own and calls this again,
// between runs, when no thread of the last one is left to be reading it.
void start(const std::vector<std::string> &words);

// The object as a value. `nothing` before start() -- which is what a test
// binary that never calls it sees, and is honest: a process with no command
// line recorded has no arguments object rather than an empty one.
const Value &object();

// The rows. Installed by whoever runs a program, beside every other module's.
// The ten selectors go in with them -- selectors.cpp is a separate file for a
// SUBJECT reason and not a linking one: those rows bind a receiver and these
// do not, which is the whole division the directory draws.
void install_handlers();

} // namespace satellite::arguments
