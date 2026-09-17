#pragma once
// satellite 004 (PLAN M0.5, 2026-09-17): ported unchanged from 003 revision 07's
// src/programs/satl-term/ but for its include paths. The milestones, sections and
// src/ paths named below are 003's.
//

// The File menu.
//
// SEPARATE FROM window.cpp because they are two jobs: that file is a command
// line and a GtkApplication, and this one is four things a person can ask for
// with a mouse. It is also the only file in this folder that opens a dialog,
// which is the seam it would have to be split on if a second menu ever wants
// one.
//
// FOUR ITEMS AND NO FIFTH. New tab and New window make somewhere to work, Open
// puts a program in this window, and Save output as writes down what a terminal
// is holding -- which matters most in the state this binary was built around,
// where a failed run is held on screen and the reason is on it. There is no
// Quit and no Close: the window's own close button is the fifth item, it is
// already there, and a menu entry duplicating it would be a second spelling of
// something the desktop has spelled for thirty years.
//
// NO ACCELERATORS, DELIBERATELY. keys.cpp's rule is that a key is only taken
// when there is nobody to give it to, and a menu shortcut is a key taken from
// every program that will ever run on the other side of the pty, forever, to
// save a person one mouse click. That trade is refused; the file's own header
// paragraph carries the argument.

#include <gtk/gtk.h>

namespace satellite {

// Build the menu bar and put its actions on the window.
//
// The actions are the WINDOW's -- the "win." prefix a GtkApplicationWindow
// gives anything added to it -- and not the application's, because every one of
// them acts on the window it was chosen in: the tab in front, the terminal
// whose screen is being saved. An application action would be the same four
// verbs with no answer to "which window".
GtkWidget *menu_bar_new(GtkWidget *window);

} // namespace satellite
