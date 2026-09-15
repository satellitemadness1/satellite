#pragma once

// `satellite.system.home` `1 22 3`, `satellite.system.environment` `1 22 2`
// and `environment(name)` `1 22 9` -- the facts about the person and the
// session rather than about memory.
//
// SPLIT FROM memory_methods.cpp BY SUBJECT, which PLAN M20 asks for: v1
// answered all of `satellite.system` from one 321-line file.

namespace satellite::system {

// Install `home` and both shapes of `environment` into eval::Handlers.
void install_host();

} // namespace satellite::system
