#pragma once
// satellite/satellite_variable_window/window_frame.hpp -- A FRAME ON THE
// SCREEN: the half of window_new() that a console shares. GTK-17.
//
// A WINDOW AND A CONSOLE ARE ONE FRAME WITH DIFFERENT INSIDES. Both are a
// GtkWindow holding a vertical column -- the column is where a menu bar goes
// (GTK-12) -- presented on the desk, held by the desk, and let go of when GTK
// says `destroy`. What differs is what fills the column: a GtkFixed to place
// pieces in, or a VteTerminal on the ground its shadow falls on
// (console_shadow.cpp). `frame_new` is that shared half, and `fill` is
// the difference, run ON THE DESK with the window and its column.
//
// `fill` RUNS BEFORE `destroy` IS CONNECTED, and that order is load-bearing:
// a console connects a `destroy` handler of its own in there, to stop its
// reader and let go of its pty, and GTK runs handlers in the order they were
// connected -- so the console's runs while the handle is whole, and the desk's
// bookkeeping (which nulls the handle's widgets) runs after it.
//
// GTK IS IN THIS HEADER, AND THAT IS ALLOWED HERE AND NOWHERE PUBLIC: it is
// included by four files compiled only where pkg-config found gtk4
// (satellite_window.cpp, window_open.cpp, window_answers.cpp and
// window_console.cpp), and by nothing satellite_object.hpp reaches.

#include "satellite_window.hpp"

#include <gtk/gtk.h>

namespace satellite004 {

// `subtitle` IS THE WORDS UNDER A WINDOW'S TITLE (2026-10-09), "" for none -- and always ""
// for a console, whose word takes three things.
WindowHandle frame_new(satellite_window::Piece which, const std::string &title, const std::string &subtitle,
                       unsigned long long int width, unsigned long long int height, std::string &why,
                       void (*fill)(satellite_window &made, GtkWidget *window, GtkWidget *column));

// THE FRAME ITSELF, ON THE DESK: a GtkWindow with the handle's title and the
// size it last asked for, its column filled by `fill`, `destroy` connected,
// presented, and its first frame watched for. frame_new runs it the first time
// a window is opened and window_open (`.open()`) every time after -- ONE place
// that says what a window on a screen is, or the second opening would be the
// one that forgot something.
void put_the_frame_up(satellite_window &which,
                      void (*fill)(satellite_window &made, GtkWidget *window, GtkWidget *column));

// A WINDOW'S SIZE IS ITS INSIDE (SATELLITE_WINDOW_SECTIONS.md rule 3, SEC-1; the author, 2026-10-25: "yes", to "is
// 800x600 the inside?"): ON THE DESK, asks GTK for a window `wide` across and `tall` under its title bar and its menu
// bar, each measured as it is now: from the window's realize, where GTK has made its bar, and from `.resize`. A first
// menu grows the window by its bar on its own (window_menu_bar.cpp), keeping a size a person gave it.
void ask_for_the_inside(satellite_window &which, int wide, int tall);

// WHAT A WINDOW'S COLUMN HOLDS (WIN-3): the GtkFixed that `.append` places
// pieces in. A console fills its column with a terminal instead, on its ground
// (console_shadow.cpp).
void a_fixed_to_place_pieces_in(satellite_window &made, GtkWidget *window, GtkWidget *column);

// AND WHAT THE WINDOW ANSWERS, PUT BACK ON A NEW FRAME (window_answers.cpp).
// ON THE DESK, from put_the_frame_up: a key controller if it had `.key`, a
// click gesture if it had `.clicked`, its clock if it had `.every`. A window
// the program has just made has none of them, so for one of those it does
// nothing. `.closed` needs nothing here: the `destroy` handler reads its name.
void the_window_answers_again(satellite_window &which);

} // namespace satellite004
