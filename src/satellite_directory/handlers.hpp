#pragma once

// `satellite.directory`'s five rows -- PLAN M19.
//
// NOT `satellite.terminal`, however much "cd" feels like a terminal thing.
// v1's note, kept because the reason is still true: `satl --run script.satl`
// has no terminal at all and still has a working directory, so naming it after
// one would be a lie in the headless case, which is the common case. It is also
// not something the terminal could do -- DESIGN §10.4 puts the window in a
// SEPARATE PROCESS that spawns this one, so a `change` made there would change
// the wrong process's directory.
//
// FIVE ROWS AND NO CREATE VERB, WHICH IS A REAL HOLE AND IS NAMED RATHER THAN
// DISCOVERED. WORD_NUMBERS §2.2 gives this node change / current / exists /
// list() / list(d) and nothing that MAKES a directory. PLAN M19 says so in its
// own words -- "whether one is minted is the numbering's, and it is named here
// so it is not discovered inside a demonstration" -- and the author left it
// unminted on 2026-09-08 because M19's done-when needs none and a verb minted to
// round out a namespace is a verb designed by symmetry. `1 18 6` is free.

namespace satellite::directory {

// Install all five. Idempotent: install() overwrites rather than appends.
void install_handlers();

} // namespace satellite::directory
