#pragma once

// The containers' rows for `handlers[path_id]` -- PLAN M16, and the door onto
// this module for everything that installs handlers.
//
// WHAT LIVES BEHIND IT: the two constructors (`satellite.container.list()`
// `1 4 2 0` and `satellite.container.map()` `1 4 1 0` -- the author's
// 2026-09-05 decision, MILESTONES/M16.md §2: DESIGN §8.7's "a declared
// variable of ANY type holds nothing" stands as written, so the bare call
// shapes are how a program constructs one), the list's twenty-five methods
// and the map's nine, the minted `search(pattern)` rows, and the search
// power's two dials `satellite.system.threshold()` `1 22 5` / `(n)` `1 22 6`.
//
// THE DIALS ARE SPELLED UNDER `system` AND INSTALLED FROM HERE, which is v1's
// own arrangement kept across the port: "it is not a system FACT: uname and
// getpwuid answer what the machine is, and this sets how the search behaves."
// satellite_system/ is the machine's facts and the `1 14 2` retunes; the
// search's knob belongs to the milestone that built the search -- and
// installing it here is also what lets tests/eval_test exercise it, since
// that binary deliberately links no machine_limits and so no satellite_system.

#include "satellite_value/value.hpp"

namespace satellite::containers {

// Every row this module owns, installed into eval::Handlers::table(). Sums
// the halves in methods_internal.hpp and adds the constructors and the dials.
void install_handlers();

} // namespace satellite::containers
