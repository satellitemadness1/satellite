#pragma once
// satellite/satellite_variable_window/window_frame.hpp -- A FRAME ON THE
// SCREEN: the half of window_new() that a console shares. GTK-17.
//
// A WINDOW AND A CONSOLE ARE ONE FRAME WITH DIFFERENT INSIDES. Both are a
// GtkWindow holding a vertical column -- the column is where a menu bar goes
// (GTK-12) -- presented on the desk, held by the desk, and let go of when GTK
// says `destroy`. What differs is what fills the column: a GtkFixed to place
// pieces in, or a VteTerminal. `frame_new` is that shared half, and `fill` is
// the difference, run ON THE DESK with the window and its column.
//
// `fill` RUNS BEFORE `destroy` IS CONNECTED, and that order is load-bearing:
// a console connects a `destroy` handler of its own in there, to stop its
// reader and let go of its pty, and GTK runs handlers in the order they were
// connected -- so the console's runs while the handle is whole, and the desk's
// bookkeeping (which nulls the handle's widgets) runs after it.
//
// GTK IS IN THIS HEADER, AND THAT IS ALLOWED HERE AND NOWHERE PUBLIC: it is
// included by two files compiled only where pkg-config found gtk4
// (satellite_window.cpp and window_console.cpp), and by nothing
// satellite_object.hpp reaches.

#include "satellite_window.hpp"

#include <gtk/gtk.h>

namespace satellite004 {

WindowHandle frame_new(satellite_window::Piece which, const std::string &title,
                       unsigned long long int width, unsigned long long int height, std::string &why,
                       void (*fill)(satellite_window &made, GtkWidget *window, GtkWidget *column));

} // namespace satellite004
