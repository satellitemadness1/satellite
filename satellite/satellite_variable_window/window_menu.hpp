#pragma once
// satellite/satellite_variable_window/window_menu.hpp -- WHAT A MENU HOLDS, as
// GTK sees it, shared by the two files that touch one. GTK-12.
//
// SPLIT OUT OF window_menu.cpp ON 2026-09-22, at 384 lines against the author's
// "try to build for 300 lines". The seam is the MODEL against the BAR:
//
//   window_menu.cpp       a menu's own model -- made, its items, its sections,
//                         its heading; gio, but for the one gtk call a heading
//                         changed on a bar makes to reach that bar's model
//   window_menu_bar.cpp   a menu meeting a window, or a menu: the bar that draws
//                         it and the actions put on the window
//
// GTK IS IN THIS HEADER, AND THAT IS ALLOWED HERE AND NOWHERE PUBLIC: it is
// included by two files that are compiled only where pkg-config found gtk4,
// and by nothing satellite_object.hpp reaches. satellite_window.hpp keeps the
// `void *`; these three are the casts back.

#include "satellite_window.hpp"

#include <gtk/gtk.h>

namespace satellite004 {

// A MENU'S MODEL, THE SECTION ITS ITEMS GO INTO NOW, AND ITS ACTIONS -- the
// three `void *` on a menu piece, as what they are. ON THE DESK, like every
// caller.
inline GMenu *model_of(const satellite_window &which) { return static_cast<GMenu *>(which.widget); }
inline GMenu *section_of(const satellite_window &which) { return static_cast<GMenu *>(which.section); }
inline GSimpleActionGroup *actions_of(const satellite_window &which)
{
    return static_cast<GSimpleActionGroup *>(which.actions);
}

} // namespace satellite004
