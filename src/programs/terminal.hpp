#pragma once

// The VTE terminal widget and the interpreter it hosts.
//
// SEPARATE FROM window.cpp because they are two jobs: that file is a command
// line and a GtkApplication, and this one is a terminal with a child in it.
// The split is by subject and not by line count -- PLAN.md §3's ceiling applies
// from the first commit precisely so that a file is never reshaped to fit it
// afterwards.
//
// Nothing here knows what a satellite program is. It knows how to find `satl`,
// how to hand it a PTY, and what to do when it is gone.

#include <gtk/gtk.h>

#include <string>
#include <vector>

namespace satellite {

// Build the terminal, paint it, and spawn the sibling `satl` into its PTY.
//
// `file` empty means the prompt -- `satl --repl`; otherwise `satl --run file`
// followed by `args`. `hold_always` keeps the window up even when the child
// exits cleanly; a child that FAILS holds it either way, which is the whole of
// the policy and is argued where it is implemented.
//
// `window` is what gets destroyed when the terminal is finished with, so the
// caller does not connect anything itself.
GtkWidget *terminal_new(GtkWidget *window,
                        const std::string &file,
                        const std::vector<std::string> &args,
                        bool hold_always);

} // namespace satellite
