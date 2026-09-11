#pragma once

// `satellite.system.memory` `1 22 4` and the twenty-three rows under it.
//
// SPLIT FROM handlers.cpp BY SUBJECT, which PLAN M20 asks for by name: v1
// answered the whole of `satellite.system` from one 321-line file, and the
// memory verbs are the half of it that is a quantity of bytes in a unit. What
// is left there -- `home`, `environment`, the dials, `persist` -- is not.

namespace satellite::system {

// Install the memory rows into eval::Handlers. Called from this module's
// install_handlers(), so a caller still installs `satellite.system` once.
void install_memory();

} // namespace satellite::system
