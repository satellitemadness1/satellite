#pragma once

// `satellite.help`'s three rows in `handlers[path_id]` -- PLAN M18.
//
// THIS FILE IS A JOIN AND THAT IS WHY IT IS NOT IN EITHER MODULE IT NEEDS.
// satellite_console/handlers.hpp says the same of itself: the walk that builds
// the answer knows nothing about a console, and the console knows nothing about
// what is worth printing. Here they meet, and the evaluator's table is the third
// party -- so a module that had to know about the other two would be a module
// that could not be tested without them.
//
// HELP IS A HANDLER ROW LIKE ANY OTHER, WHICH IS NOT AN IMPLEMENTATION DETAIL.
// `built()` names a node when it has a handler row, so help installing itself
// into the same table it reads is what makes `satellite.help` appear among the
// topics `satellite.help()` prints. A help that reached the console some other
// way would be the one word in the language its own account left out.
//
// THREE ROWS AND TWO ANSWERS. `satellite.help` `1 19` and `satellite.help()`
// `1 19 0` are the same walk started at the root -- PLAN's "the parentheses are
// optional when you want everything" -- and `satellite.help(x)` `1 19 1` is the
// same walk started at x. There is no third routine.

namespace satellite::help {

void install_handlers();

} // namespace satellite::help
