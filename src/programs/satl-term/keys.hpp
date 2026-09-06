#pragma once

// What the keyboard means in a satl-term window.
//
// SEPARATE FROM terminal.cpp because they are two jobs: that file is a terminal
// with a child in it, and this one is a policy about three keystrokes. The split
// is by subject and not by line count -- the same seam terminal.hpp was split
// from window.cpp on -- though the count agrees: terminal.cpp is the largest
// file M1.5 left behind and MILESTONES/M1.5.md §3 named it "the number to
// watch".
//
// ONE CONTROLLER AND NOT THREE. Every key this window cares about is decided in
// one place, which is what makes the decisions orderable at all: two capture
// phase controllers on the same widget answer the same keystroke in whatever
// order GTK happens to hold them in, and "Ctrl-C copies" and "any key closes
// this window" are two answers to the same press. The window being held open
// after its child is gone is exactly when Ctrl-C means copy, so those two rules
// had to meet somewhere; this is where.
//
// Nothing here knows what a satellite program is either. It knows whether there
// is a child to give a keystroke to, and what to do when there is not.

#include <gtk/gtk.h>

namespace satellite {

// Install the window's key bindings, in the capture phase, once.
//
// `interpreter_is_running` is asked AT THE MOMENT A KEY ARRIVES rather than
// read once here, because the answer changes underneath: the whole of Ctrl-C's
// meaning turns on whether there is still a child on the other side of the pty.
// A bare function pointer rather than a std::function, matching the seam
// system_facts/interrupt.hpp draws for the same reason -- the caller owns the
// state and this side only knows there is a question to ask.
void install_key_bindings(GtkWidget *window,
                          GtkWidget *terminal,
                          bool (*interpreter_is_running)());

// From now on, an ordinary key closes the window.
//
// Called by whoever decided to HOLD the window open with its child gone --
// terminal.cpp's exit policy -- so that the "press any key" it prints is true.
// Ctrl-C and Ctrl-V are not ordinary keys and keys.cpp says why they still are
// not, which is the whole reason this lives beside them rather than in a
// controller of its own.
void close_on_any_key();

} // namespace satellite
