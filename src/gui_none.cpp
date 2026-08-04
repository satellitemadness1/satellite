// The stand-in for the GTK4 console on a machine without GTK4.
//
// main.cpp is written once against this seam, so a build with no GUI still
// links and simply reports that there is no console to open.
#include "satellite/gui.hpp"

namespace satellite {

bool gui_available() { return false; }
int  gui_main(int, char**) { return 1; }

}  // namespace satellite
