#pragma once

// `satellite.library.system`'s four dials, behind the tables -- the reads
// `1 14 2 1` through `1 14 2 4` as module constants, and their writes as
// M15's retune rows (dispatch.hpp's Assigners).
//
// A MODULE OF ITS OWN, AND THE SEAM IS WHY. The dials' VALUES live in the
// running machine's Policy, which the evaluator owns; the one dial the
// evaluator never reads -- min_free_mb, the watchdog's -- lives in
// machine_limits, which the evaluator deliberately does not link
// (tests/eval_test's link rule carries the receipt, M8.5 §4.1). So the rows
// that must see both sides live here, beside neither, the way
// satellite_console owns `1 5` and satellite_time owns `1 9`: a leaf module
// linked by `satl` and installed with the others at run time.
// tests/eval_test proves the RETUNE MECHANISM with rows of its own instead of
// linking this module, which is PLAN M2's registry rule pointing the other
// way for once -- the mechanism has a consumer in the milestone that built
// it, and the real rows have example/floats.satl and the milestone note's
// transcript.

namespace satellite::system {

// Install the four reads and the four write rows into eval::Handlers and
// eval::Assigners. Called beside the console's, the scalars' and the clock's
// installs -- programs/run_command.cpp and evaluate_commands.cpp.
void install_handlers();

} // namespace satellite::system
