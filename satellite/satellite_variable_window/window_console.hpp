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
// 11 -- and Q-VTE-1's stopgap, the screen reset once so VTE's own warning line
// is not the first thing on it. Shared so that satl's own console and a
// program's are dressed exactly alike. ON THE DESK.
void dress_the_terminal(VteTerminal *terminal);

// THE FILE MENU ACROSS satl'S OWN CONSOLE -- New window, Open…, Save output as…
// -- and F10 left to the program. ON THE DESK. console_menu.cpp.
void give_it_a_file_menu(satellite_window &console);

} // namespace satellite004
