#pragma once

// How display's style options become the bytes around its text -- M30, "the
// screen". PLAN.md's M30 entry holds the decisions; this file holds the four
// that are built: foreground=, background=, bold= and italic=.
//
// STYLE IS A PER-CALL OPTION AND NEVER A SWITCH. The printer is a thread and a
// line is atomic because the unit queued is a whole string (console.hpp), so a
// global "colour is now orange" would leak into whichever thread's line came
// next. So the escapes open and close INSIDE the one string display queues.

#include <string>

namespace satellite::eval {
class Machine;
}

namespace satellite::console {

// The two ends of a styled line: `open` goes before the text and `close` after
// it, and both are empty when the call gave no style or the style is not to be
// written. False means the call was refused -- a colour that is not xRRGGBB
// (S1004) or a bold= that is not a bool (S0713) -- and nothing is printed.
//
// A STYLE IS CHECKED EVEN WHEN IT WILL NOT BE WRITTEN, so a program that is
// wrong in a terminal is wrong in a pipe too. What is not written:
//   - anything, when stdout is not a terminal -- a pipe or a file gets the
//     plain text (the author, 2026-09-12);
//   - colour, when NO_COLOR is set and not empty (no-color.org, the author,
//     2026-09-12). bold= and italic= still apply, as the convention says.
bool style_of(eval::Machine &m, std::string *open, std::string *close);

} // namespace satellite::console
