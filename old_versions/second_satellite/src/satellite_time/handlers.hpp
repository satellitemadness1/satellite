#pragma once

// `satellite.time`'s rows in `handlers[path_id]` -- PLAN M13.
//
// TWO ROWS AND NOT FOUR, AND THE GAP IS DELIBERATE AND DATED. `now` `1 9 1`
// and `sleep(n)` `1 9 3` are M13's; `new` `1 9 2` moved to M29 on 2026-09-04
// -- designed beside `satellite.variable.date`, because an instant
// constructor cannot be designed apart from the date it constructs from --
// and an uninstalled row is S0721's sentence rather than a silence, which is
// how a reserved number reads until its milestone lands.
//
// THE SEAM IS satellite_console/handlers.hpp's: the module that owns the word
// installs the row, and this file is the only one in the tree that knows both
// what a clock is and what a Value is.

namespace satellite::time {

// Install both rows. Idempotent: install() overwrites rather than appends.
void install_handlers();

} // namespace satellite::time
