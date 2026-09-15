#pragma once

// Starting the machine limits from the command line -- M6's half of what
// main.cpp does before any arm runs.
//
// THE SAME SEAM cache_command.hpp AND check_command.hpp ARE ON. main.cpp
// answers the command line and decides WHAT TO SAY; the work is a module's.
// What is here rather than in machine_limits/ is the part that knows about
// ARGUMENTS -- which flags may name a config file, and what a failure to read
// one is worth in this program's exit statuses. machine_limits/limits.hpp takes
// a path and knows nothing about argv, which is what lets tests/limits_test
// drive it without one.

#include <string>
#include <vector>

namespace satellite {

// Read the limits, start the pool, start the watchdog. EXIT_FINE, or a status
// to return from main().
//
// CALLED BEFORE ANY ARM AND AFTER THE ARGUMENTS ARE ASSEMBLED. PLAN §4.5.1.2 is
// the reason it is not deferred into the arms that would use it: "satl almost
// must start the THREAD_COUNT in satellite_config.ini because we almost have to
// assume the user will call parallel_for, so that requires a warm pool no matter
// what." That makes the pool a property of the PROCESS rather than of what the
// process was asked to do. A program that never threads pays 1-3% of startup
// for it -- §4.5.1.2's table, measured -- and one that does avoids ~590 us of
// blocked execution at its first parallel call.
//
// A MALFORMED CONFIG STOPS EVERYTHING, INCLUDING `satl --version`, and that is
// the intended shape rather than an oversight. satl cannot know how much of
// this machine it may take, and carrying on with a guess is DESIGN §1.1's
// "never anything behind their back" with the machine's memory as the stake.
// The block on stderr names the file, the line and the character; `satl
// --limits <another file>` is the way back in, and the paragraph below is why
// that works.
//
// THE CONFIG FILE IS AN OPERAND OF TWO FLAGS AND NOT A --config OPTION,
// deliberately. A `--config` every arm accepted would be a second way to say
// what `satellite_config.ini` beside the binary already says, reachable from
// commands that have nothing to do with limits. `--limits` and `--watchdog` are
// the commands ABOUT the limits, and an operand is the shape `satl --words
// <path>` and `satl --errors <code>` already use. It is also the escape hatch
// for the one failure a fatal config read creates: with a malformed file
// installed, `satl --limits good.ini` still answers, because the operand
// REPLACES the found file rather than adding to it.
int start_limits(const std::vector<std::string> &args);

} // namespace satellite
