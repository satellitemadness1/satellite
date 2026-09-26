#pragma once

// Phase 4's memory — $HOME/.satl_orbit, and the four things done to it.
//
// Split out of orbit.hpp, which had passed the 325-line ceiling once the result
// type gave a Finding what each of the five phases said about it. The seam is
// one the file already had: everything else in orbit.hpp is five phases over
// values in memory, and this is a FORMAT ON DISK — one record per line, written
// by one run and read by the next, with all the compatibility that implies.
//
// §9's terms for $HOME/.satl_history are the terms here: the file is REAL, it is
// READABLE, and there is a way to see it and clear it from inside the language,
// because keeping a record of what a user did is "the kind of thing this
// language does not do quietly". A hidden index would be the language acting on
// the user behind their back, which is the one thing it has never done.
//
//     THE THIRD TERM IS NOT MET YET, AND THIS COMMENT USED TO CLAIM IT WAS.
//
// Nothing under src/ calls orbit_memory_read, orbit_memory_forget or
// orbit_memory_path. The four functions below exist, orbit_test drives them,
// and there is NO SPELLING IN THE LANGUAGE that reaches any of them -- so every
// .orbit() writes to a file in the user's home that no satellite program can
// look at or delete. That is the language keeping a record quietly, which is
// the one thing the paragraph above says it does not do.
//
// It is a DEFECT and not a to-do: the terms were stated as met, here and in
// orbit_index.cpp, from the day phase 4 landed. Found 2026-08-25 by grepping
// for the callers and finding none. What it needs is two module functions --
// one to show the file, one to clear it -- and a naming decision, because
// satellite.system.memory already means RAM. satellite_orbit_search/
// orbit_plan.txt, DECISION 9, carries the whole of it.

#include <string>
#include <vector>

namespace satellite {

struct OrbitRecord {
    std::string pattern;   // the query, rendered
    std::string resolved;  // what it settled on, rendered
    long long count = 0;   // how many times that pairing has been seen
};

// Where the file is. Empty when the machine has no $HOME, which is the same
// answer §9 gives the history file: memory-only, and nothing written.
std::string orbit_memory_path();

// Every record, or an empty vector when there is no file yet. Never fails: a
// missing memory is a memory with nothing in it, not an error.
void orbit_memory_read(std::vector<OrbitRecord> &out);

// Remember that `pattern` settled on `resolved`, incrementing the count if that
// pairing is already there. Silently does nothing when there is no $HOME.
void orbit_memory_record(const std::string &pattern, const std::string &resolved);

// Forget everything. The "and a way to clear it" half of §9's terms.
void orbit_memory_forget();

// Override the file, for tests and for a program that wants its own. Empty
// restores $HOME/.satl_orbit. THREAD-LOCAL, on DECISION 5a's argument for the
// dial: a knob meaning "where should MY memory live".
void set_orbit_memory_path(const std::string &path);

} // namespace satellite
