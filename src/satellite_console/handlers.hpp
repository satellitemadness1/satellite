#pragma once

// The console's rows in `handlers[path_id]` -- PLAN M10.
//
// THE FIRST REAL ROWS THE TABLE HAS EVER HELD. `evaluator/dispatch.hpp` says of
// itself that it is "EMPTY IN `satl` AT M9, AND THAT IS THE MILESTONE BOUNDARY
// RATHER THAN A GAP ... the first rows are M10's console", and PLAN §8's path
// ledger says the same from the other end. Until now the only consumer was
// `tests/eval_test/dispatch.cpp` installing handlers of its own, which is PLAN
// M2's rule about a registry needing a reader in the milestone that writes it.
//
// THE MODULE THAT OWNS THE WORD INSTALLS THE ROW, which is the seam this file
// exists to draw. `evaluator/` knows what a table is and nothing about what a
// console is; `satellite_console/console.cpp` knows what a console is and
// nothing about a Value. This file is the join, and it is the only file in the
// tree that includes both -- so when M14 adds `input` `1 5 2`-`1 5 4`,
// `typed()` `1 5 5`, `width`, `height`, `clear()` and `home()`, they arrive
// here beside `display` rather than in the evaluator.
//
// AND THE ROWS ARE INSTALLED BY THE ARM THAT RUNS A PROGRAM, not by a static
// initialiser. `Handlers::table()` is a function-local static for PLAN §4.3's
// reason -- `satl --version` must not build a table it never reads -- and a
// namespace-scope object here would put the cost back before main() by another
// road.

namespace satellite::console {

// Install every row this module owns. Idempotent: `Handlers::install()`
// overwrites a row rather than appending one, so a second call is the same
// table.
void install_handlers();

} // namespace satellite::console
