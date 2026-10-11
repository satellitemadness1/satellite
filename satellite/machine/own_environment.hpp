#pragma once
// satellite/machine/own_environment.hpp -- WHAT satl SETS IN ITS OWN ENVIRONMENT FOR ITSELF, and the
// environment a program it starts is handed instead (the review of steps 2-5, 2026-10-01).
//
// satl's console sets names for the GTK it carries: XKB_CONFIG_ROOT, FONTCONFIG_FILE, FONTCONFIG_PATH
// and GSETTINGS_SCHEMA_DIR (satellite_variable_window/window_spill.cpp), and GDK_DEBUG on a system GTK
// older than 4.18 (window_desk.cpp). They are satl's alone: its fonts.conf names ONE directory and no
// system paths, so a program handed it -- a font tool, a GUI program a script opens -- would see only
// the fonts satl carries. So each is kept here as the machine gave it, before satl sets it, and a
// program started by satellite.variable.program or .bash is handed the machine's.

#include <string>
#include <vector>

namespace satellite004 {

// CALLED BEFORE satl SETS `name` FOR ITSELF: what the machine gave -- a value, or none -- is kept, the
// first time only.
void keep_the_machines(const char *name);

// THE ENVIRONMENT A PROGRAM IS HANDED: satl's own, with every name kept above as the machine gave it.
// The strings are held in `storage` and the answer points into `pointers`, ending in nullptr -- or it
// is environ itself, untouched, when satl has set nothing for itself. Called before a child exists:
// it allocates.
char *const *environment_for_a_program(std::vector<std::string> &storage, std::vector<char *> &pointers);

} // namespace satellite004
