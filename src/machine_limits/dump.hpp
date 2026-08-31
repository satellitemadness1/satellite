#pragma once

// What `satl --limits` prints, and the line `satl --words` ends with.
//
// THE MILESTONE'S CONSUMER, IN THE MILESTONE THAT WROTE IT -- the rule PLAN M2
// set because the first satellite shipped three commits where its word registry
// had no reader at all and four defects accumulated behind a guarantee nothing
// was checking. `--words` is M2's, `--tokens` M3's, `--unparse` M4's, `--satc`
// M4.5's, `--check` and `--errors` M5's, and this is M6's.
//
// AND IT IS THE ONLY WAY TO SEE ANY OF THIS. Nothing in the language reads a
// limit until M8, nothing runs until M10, and a watchdog that has not fired
// looks exactly like no watchdog. A dump is not a nicety here; it is the whole
// visible surface of the milestone.

#include <cstddef>
#include <string>

namespace satellite::limits {

// Every value satl is holding to, and where each one came from.
std::string limits_text();

// What `satl --words` says about the pool after it has printed the table.
//
// M6's DONE-WHEN ASKS `satl --words` TO SHOW TWO THINGS -- "the pool parked
// before the walk begins and the walk itself single-threaded" -- and this is
// both, said where the walk just happened. The `units` it is given is what was
// walked, so the sentence is about the run that produced the table above it
// rather than about the pool in general.
std::string walk_note_text(size_t units);

} // namespace satellite::limits
