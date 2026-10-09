#pragma once
// satellite/satellite_variable_window/window_spill.hpp -- the carried data, written
// to disk before gtk_init() runs. SATELLITE_WINDOW.md WIN-1.
//
// CODE LINKS; DATA DOES NOT. satl carries GTK, glib, cairo, pango, freetype and
// fontconfig as machine code, and every one of them then looks on disk for files
// that are not on a machine with no GTK installed. Three of those lookups do not
// fail politely -- they take the process down:
//
//   xkeyboard-config   gdkkeymap-wayland.c calls xkb_keymap_new_from_names() at
//                      seat creation and null-checks NOTHING. SIGSEGV inside
//                      gtk_init(), before a window exists and before a line prints.
//   fontconfig's config  measured 2026-09-20: "Cannot load default config file",
//                      then SIGSEGV. Our fontconfig does not read /etc/fonts.
//   GSettings schemas  g_settings_new() on a missing schema calls g_error(),
//                      which is fatal and cannot be caught.
//
// So this runs FIRST, on the desk's thread, before gtk_init -- see window_desk.cpp.
//
// ORDERING IS LOAD-BEARING AND NOT A STYLE CHOICE. glib's
// initialise_schema_sources() is wrapped in g_once_init_enter
// (gio/gsettingsschema.c): the FIRST call to g_settings_schema_source_get_default()
// freezes the source list for the life of the process. GSETTINGS_SCHEMA_DIR set
// one call later has no effect at all, and nothing reports that it did nothing.

#include <string>

namespace satellite004 {

// Writes what GTK needs into a writable directory and points GTK at it with
// XKB_CONFIG_ROOT, FONTCONFIG_FILE and GSETTINGS_SCHEMA_DIR. Answers false with
// `why` filled in; never throws, and never calls into GTK.
//
// IT REUSES WHAT IS ALREADY THERE. The spill goes to $XDG_RUNTIME_DIR, which the
// system clears at logout, under a name carrying the data's own size -- so a
// second satl in the same session finds it complete and writes nothing, and a
// satl built from DIFFERENT data cannot mistake an old spill for its own. A
// half-written spill is the one thing that must never be reused: a partial xkb
// tree fails exactly the way no xkb tree does.
bool spill_what_gtk_needs(std::string &why);

} // namespace satellite004
