#pragma once

// The VTE terminal widget and the interpreter it hosts.
//
// SEPARATE FROM window.cpp because they are two jobs: that file is a command
// line and a GtkApplication, and this one is a terminal with a child in it.
// The split is by subject and not by line count -- PLAN.md §3's ceiling applies
// from the first commit precisely so that a file is never reshaped to fit it
// afterwards.
//
// ONE TERMINAL, AND A WINDOW MAY HOLD SEVERAL. Everything this file knows about
// a child is state carried BY THE WIDGET rather than by the file, and that is
// what File > New tab cost: until 2026-09-06 the two flags below were file
// statics, which is a limit of one interpreter per process written where nobody
// would look for it. tabs.cpp is now the file that knows how many there are,
// and this one has stopped counting.
//
// Nothing here knows what a satellite program is. It knows how to find `satl`,
// how to hand it a PTY, and what to do when it is gone.

#include <gtk/gtk.h>

#include <string>
#include <vector>

namespace satellite {

// What a terminal says when it is FINISHED WITH -- its child is gone and the
// exit policy did not hold the screen open for it. tabs.cpp answers by closing
// the page, and the last page closing takes the window with it, which is how
// today's single-tab window keeps behaving exactly as it did.
//
// A bare function pointer rather than a std::function, matching the seam
// keys.hpp draws for the same reason: the caller owns what happens next, and
// this side only knows there is somebody to tell.
using TerminalFinished = void (*)(GtkWidget *terminal);

// Build a terminal, paint it, and spawn the sibling `satl` into its PTY.
//
// `file` empty means the prompt -- `satl --repl`; otherwise `satl --run file`
// followed by `args`. `hold_always` keeps the terminal up even when the child
// exits cleanly; a child that FAILS holds it either way, which is the whole of
// the policy and is argued where it is implemented.
GtkWidget *terminal_new(const std::string &file,
                        const std::vector<std::string> &args,
                        bool hold_always,
                        TerminalFinished finished);

// Run a program in a terminal WHOSE CHILD IS ALREADY GONE -- File > Open in a
// tab that has finished. It refuses while a child is alive rather than killing
// it, because no part of this binary may take a run away from the person who
// started it; tabs.cpp is what turns that refusal into a new tab instead.
//
// The scrollback is KEPT. A terminal that has failed is holding the only copy
// of the reason (see the exit policy in the .cpp), and clearing it to make room
// for the next program throws that away at the exact moment somebody has chosen
// to keep working in this window.
void terminal_run(GtkWidget *terminal,
                  const std::string &file,
                  const std::vector<std::string> &args);

// Whether there is still an interpreter on the other side of the pty. Asked AT
// THE MOMENT A KEY ARRIVES rather than read once, because the answer changes
// underneath and it is the whole of what Ctrl-C means.
bool terminal_is_running(GtkWidget *terminal);

// Whether this terminal is being held open with its child gone, which is the
// state its own message calls "press any key to close". keys.cpp asks, so that
// the promise the screen makes and the key that keeps it are one decision.
bool terminal_is_held(GtkWidget *terminal);

// "I am done with you" from the outside -- Ctrl-C in a held terminal with
// nothing highlighted, and the any-key close. It goes through the same
// TerminalFinished the exit policy uses, so a tab closes by one road however
// the decision was reached.
void terminal_finish(GtkWidget *terminal);

} // namespace satellite
