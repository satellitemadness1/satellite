#pragma once
// satellite/satellite_variable_window/window_console.hpp -- WHAT THE TWO
// CONSOLE FILES SHARE (GTK-17): the VteTerminal behind a console's `void *`,
// the one question every console method asks first, and the terminal's look.
//
//   window_console.cpp    a PROGRAM's console: satellite.console.new, and what a
//                         program does to one -- .display, .typed, .clear,
//                         .home, .columns, .rows, its colours and its font
//   console_launch.cpp    satl's OWN console: `satl --console`, or satl with no
//                         console of its own, its stdio moved onto the pty, the
//                         keyboard, the hold at the end and the key that closes it
//   console_menu.cpp      the File menu across satl's own console
//   console_settings.cpp  File > Settings…, and the font size config.ini keeps
//   console_status.cpp    the bar along the bottom: idle or running, memory,
//                         threads active, and the size in characters
//
// VTE IS IN THIS HEADER, AND THAT IS ALLOWED HERE AND NOWHERE PUBLIC: it is
// included by two files, and by them only where VTE was found
// (SATELLITE_HAS_CONSOLE, make_support/047-window.mk), and by nothing
// satellite_object.hpp reaches. satellite_window.hpp keeps the `void *`.

#include "satellite_window.hpp"

#include <gtk/gtk.h>
#include <vte/vte.h>

namespace satellite004 {

// THE TERMINAL INSIDE A CONSOLE, as what it is. A CAST AND NOT A CALL: its
// callers take it on the interpreter's thread before their on_the_desk()
// lambda, as as_widget() is taken next door, and every VTE call on the result
// is the lambda's.
inline VteTerminal *terminal_of(const satellite_window &which)
{
    return VTE_TERMINAL(static_cast<GtkWidget *>(which.terminal));
}

// A CONSOLE THAT IS STILL ON A SCREEN, said the same way everywhere. A piece
// that is not a console is refused by name; one whose window has gone is
// "it is closed".
bool a_console_that_is_open(const satellite_window &which, std::string &why);

// satl-term's LOOK, ON A TERMINAL: black on the light blue field, IBM Plex Mono
// at the size config.ini keeps. Shared so that satl's own console and a
// program's are dressed exactly alike. ON THE DESK. (Q-VTE-1's stopgap, which
// drops VTE's own warning line, needs a pty, so it is a_terminal_to_type_in's.)
void dress_the_terminal(VteTerminal *terminal);

// THE FILE MENU ACROSS satl'S OWN CONSOLE -- New window, Open…, Save output as…,
// Settings… -- and F10 left to the program. ON THE DESK. console_menu.cpp.
void give_it_a_file_menu(satellite_window &console);

// THE CONSOLE'S FONT SIZE, IN POINTS: `console.font_size` in ~/.satl/config.ini,
// and 11 when the row is absent or not a whole number from 1 up. The second puts
// a size on a terminal (ON THE DESK); the third is File > Settings… (ON THE
// DESK). console_settings.cpp.
long long int console_font_points();
void set_console_font_points(VteTerminal *terminal, long long int points);
void open_the_console_settings(satellite_window &console);

// THE BAR ALONG THE BOTTOM OF satl'S OWN CONSOLE -- "idle   19 MB",
// "0/1024 threads active", "120x48" -- refreshed twice a second. ON THE DESK.
// console_status.cpp.
void give_it_a_status_bar(satellite_window &console);

} // namespace satellite004
