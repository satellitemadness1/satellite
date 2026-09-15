#pragma once

// What the keyboard means in a satl-term window.
//
// SEPARATE FROM terminal.cpp because they are two jobs: that file is a terminal
// with a child in it, and this one is a policy about three keystrokes. The split
// is by subject and not by line count -- the same seam terminal.hpp was split
// from window.cpp on.
//
// ONE CONTROLLER AND NOT THREE, and now not one per tab either. Every key this
// window cares about is decided in one place, which is what makes the decisions
// orderable at all: two capture phase controllers on the same widget answer the
// same keystroke in whatever order GTK happens to hold them in, and "Ctrl-C
// copies" and "any key closes this" are two answers to the same press. The
// terminal being held open after its child is gone is exactly when Ctrl-C means
// copy, so those two rules had to meet somewhere; this is where.
//
// Nothing here knows what a satellite program is either. It knows whether there
// is a child to give a keystroke to, and what to do when there is not.

#include <gtk/gtk.h>

namespace satellite {

// Install the window's key bindings, in the capture phase, once.
//
// `terminal_in_front` is asked AT THE MOMENT A KEY ARRIVES rather than read
// once here, and since File > New tab that is two questions and not one: WHICH
// terminal the key is for, and then -- through terminal.hpp -- whether it still
// has a child. Both change underneath, and the whole of Ctrl-C's meaning turns
// on the answers. A bare function pointer rather than a std::function, matching
// the seam system_facts/interrupt.hpp draws for the same reason: the caller owns
// the state and this side only knows there is a question to ask.
//
// It is tabs.cpp that calls this, because tabs.cpp is what can answer the
// question. Until 2026-09-06 terminal.cpp did, which was right while a window
// held one terminal and a per-window controller could be installed by the thing
// it watched.
void install_key_bindings(GtkWidget *window,
                          GtkWidget *(*terminal_in_front)());

} // namespace satellite
