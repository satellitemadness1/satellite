#pragma once

// `satellite.random`'s rows in `handlers[path_id]` -- PLAN M13.
//
// THE MODULE THAT OWNS THE WORD INSTALLS THE ROW, the seam
// satellite_console/handlers.hpp drew first: `evaluator/` knows what a table
// is and nothing about a generator; `satellite_random/` knows what a draw is
// and nothing about a Value until this join. Twelve rows -- `1 7 1` through
// `1 7 12`, every numbered child of `satellite.random` -- and the three
// `.range` spellings arrive free, because M2 built them as aliases of
// `1 7 5`/`1 7 8`/`1 7 11` and an alias is a second spelling of a number this
// table already has a row for.
//
// THREE OF THE TWELVE ARE REFUSALS BY DESIGN (the author, 2026-09-04): the
// zero-argument shapes `1 7 1`-`1 7 3` hold S0901's sentence, one text for
// all three tiers and both spellings, so no number is left owning nothing.
//
// WHAT v1 DID INSTEAD IS THE DISPATCH PLAN §7 THROWS AWAY: modules_random.cpp
// string-compared the tier and "range" per call. What survives of it here is
// the argument checking and its failure texts, as errors.def S09xx rows.

namespace satellite::random {

// Install every row this module owns. Idempotent the way every installer is:
// `Handlers::install()` overwrites a row rather than appending one.
void install_handlers();

} // namespace satellite::random
