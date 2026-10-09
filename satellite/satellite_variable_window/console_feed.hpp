#pragma once
// satellite/satellite_variable_window/console_feed.hpp -- FAST_PRINTING.md STEP 5: satl's own
// console fed straight from the display thread, not through its pty. console_feed.cpp says how.

namespace satellite004 {

// ON THE DESK, once satl's own console has its terminal and satl's stdio is on the pty: from here
// on the display thread's pieces go into VTE directly. `terminal` is the VteTerminal; `master`
// is its pty's master, which the feed watches so it never overtakes what is still in the pty.
void console_feed_starts(void *terminal, int master);

// ON THE DESK, Ctrl-S (true) or Ctrl-Q (false) while the pty takes them as flow control: nothing is
// fed while it holds, as nothing came through the stopped pty.
void console_feed_holds(bool held);

// ON THE DESK, when satl's own console goes away, whoever took it: nothing more is fed, what was
// waiting is let go, and a flush waiting on it goes on. The display thread writes fd 1 again.
void console_feed_stops();

} // namespace satellite004
