#pragma once

// The window's tabs -- how many terminals there are, and which one is in front.
//
// SEPARATE FROM window.cpp because they are two jobs: that file is a command
// line and a GtkApplication, and this one is a notebook with terminals in it.
// SEPARATE FROM terminal.cpp for the older reason -- that file is one terminal
// and the child inside it, and it deliberately cannot count past one.
//
// THIS IS THE FILE THE ONE-CHILD ASSUMPTION MOVED INTO, and it is the whole
// cost of File > New tab. Everything that used to be a file static somewhere
// else -- is a child alive, is the screen being held, which window to destroy --
// is now either per-terminal state that terminal.cpp carries, or a question
// asked of the notebook here.
//
// A SINGLE TAB SHOWS NO TAB BAR, so a window that nobody has asked for a second
// tab in is pixel for pixel the window M1.5 built. The chrome arrives with the
// second terminal and leaves with it.
//
// Nothing here knows what a satellite program is either. It knows how many
// terminals a window is holding and what a page label says.

#include <gtk/gtk.h>

#include <string>
#include <vector>

namespace satellite {

// Build the notebook and put the window's keyboard on. No pages yet: the first
// one is opened by whoever parsed the command line, with the file it named.
//
// `hold_always` is --hold, and it is remembered here because it is a property
// of this INVOCATION rather than of any one terminal -- every tab this window
// opens gets the same answer, including tabs opened from the File menu an hour
// later.
GtkWidget *tabs_new(GtkWidget *window, bool hold_always);

// A new tab, always -- File > New tab, and the first tab at startup. `file`
// empty is the prompt.
void tabs_open_tab(const std::string &file,
                   const std::vector<std::string> &args);

// Run a program IN THIS WINDOW -- File > Open.
//
// The tab in front if its interpreter has finished, and a new tab if it has
// not. ONE ITEM AND TWO PLACES IT CAN LAND, which is a decision and not an
// oversight: "run it in this window" is the promise, and the alternative to
// taking the free tab is either killing somebody's running program or greying
// the menu's main item out -- and after M22 the prompt runs until the person
// leaves, so greying it out means a File menu that can never open a file again.
void tabs_run(const std::string &file,
              const std::vector<std::string> &args);

// The terminal in the tab in front, or null when there are none left. keys.cpp
// asks it of every keystroke and menu.cpp of anything that acts on what is on
// the screen.
GtkWidget *tabs_terminal_in_front();

} // namespace satellite
