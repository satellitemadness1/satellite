#pragma once

// `satl --satc <file>` -- PLAN M4.5's consumer. See SATC.md for the format and
// src/satellite_cache/cache.hpp for the module this drives.
//
// WHY IT IS NOT THREE LINES IN main.cpp LIKE THE OTHER FLAGS. --words prints a
// table, --tokens prints a list and --unparse prints a program; each is one
// call and a choice of stream. This one is SATC.md §4's reading order, which is
// three steps that have to happen in that order and a fourth thing to say when
// none of them worked, and the file it exercises is on the user's disk rather
// than in front of them. A milestone whose consumer is a loop needs somewhere
// for the loop to be.

#include <string>

namespace satellite {

// Run the cache over `path`: read the `.satc` if there is a usable one, walk
// the source and write a fresh one if there is not, print the `.satc` on stdout
// either way, and say on stderr which of the two happened.
//
// A COMMAND'S EXIT STATUS IS ABOUT THE PROGRAM AND NEVER ABOUT THE CACHE, which
// is §4's "a missing, stale or unreadable `.satc` is never an error" arriving at
// the one place a script can see it. A cache that could not be read, could not
// be written, or was thrown away costs a walk and nothing else; what decides
// the status is whether the user's program parsed.
int satc_command(const std::string &path);

} // namespace satellite
