// The GTK4 console, behind a seam so that main.cpp does not include gtk.h.
//
// `satellite` with no arguments opens the console; `satellite check foo.satl`
// stays a command-line tool.  One binary, both behaviours -- which is also what
// keeps .ship/001/verify.sh and every README example working.
//
// A build without GTK4 still links: gui_available() returns false and main
// falls back to printing usage.
#pragma once

namespace satellite {

// Was this binary built with the GTK4 console in it?
bool gui_available();

// Run the console.  Returns the process exit status.  Only call when
// gui_available() is true.
int gui_main(int argc, char** argv);

}  // namespace satellite
