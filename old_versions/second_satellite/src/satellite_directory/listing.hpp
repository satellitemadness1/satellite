#pragma once

// The TABLE a person sees when they type `satellite.directory.list()` or
// `list(d)` on a line of its own at the prompt -- one row per entry:
//
//   name        type | permissions   | owner   | created          | modified
//   evaluator   dir  | rwxr-xr-x     | madness | 2026-08-23 10:02 | 2026-09-12 19:31
//   Makefile    text | rw-r--r--     | madness | 2026-08-23 10:02 | 2026-09-10 08:14
//   satl        - -  | rwxr-xr-x exe | madness | 2026-09-12 19:31 | 2026-09-12 19:31
//
// THE LIST ITSELF IS UNCHANGED, and that is the author's decision of
// 2026-09-12: a PROGRAM still gets M16's list of plain sorted names back, so
// `satellite.file.open` and a loop over it keep working, and
// `satellite.console.display(list)` still prints `[a, b]`. The table is what
// the PROMPT shows for the one line whose value would otherwise be thrown away.
// v1 drew it from a list method (`.lines()`, which has no number here) by
// stat()ing every element relative to the working directory -- wrong for
// `list(d)`, whose elements are leaves of d. Asking from inside the handler is
// what fixes that: it is the one place that knows which directory was read.
//
// A REQUEST AND NOT A MODE. The prompt asks for ONE table before running a line
// it recognised (satellite_prompt/session.cpp) and withdraws the request after,
// and the handler consumes it on first use, so nothing else a line does can
// print a second one. thread_local, because a thread that line started is not
// the line.

#include <string>
#include <vector>

namespace satellite::directory {

void ask_for_listing(bool wanted);

// True once per request: answers whether a table was asked for and clears it.
bool take_listing_request();

// The table for `leaves`, in the order given, each a name inside `directory`.
std::string listing(const std::string &directory,
                    const std::vector<std::string> &leaves);

} // namespace satellite::directory
