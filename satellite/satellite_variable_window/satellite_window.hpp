#pragma once
// satellite/satellite_variable_window/satellite_window.hpp -- satellite.variable.window,
// the thirteenth arm. SATELLITE_WINDOW.md WIN-2 and WIN-3.
//
// The author, 2026-09-19: *"we are wiring in GTK+ so that satl and satl-term
// become a single application"*, and the shape he wrote:
//
//     satellite.variable.window my_window = satellite.window.new("window_title", 800, 600)
//     my_window.append(satellite.window.button("text"), 400, 300)
//
// THERE IS NO GTK IN THIS HEADER, ON PURPOSE. satellite_object.hpp includes it,
// so every file that holds a value would otherwise include gtk/gtk.h -- 004's
// own object model would stop compiling on a machine with no GTK, and satl is
// the one thing 047-window.mk promises builds everywhere. The widget is a
// `void *` here and a GtkWidget * in exactly two files, both of which are
// compiled only when pkg-config finds gtk4.
//
// A WINDOW AND A BUTTON ARE ONE TYPE, not two arms. `satellite.window.button()`
// answers something that is only ever handed straight to `.append`, and the
// author's own line never declares a name for it. Two arms would have meant two
// entries in every switch over Kind to say the same thing twice; a `piece` that
// says which it is costs one byte and no switch at all.
//
// THE HANDLE IS SHARED, as a file's and an infinity's are: two names for one
// window are the same window, because a window is a thing on a screen and not a
// value to be copied. It is also shared with the desk (window_desk.hpp), which
// holds its own strong reference for exactly as long as the window is open --
// that is what makes the `destroy` signal safe to handle after the program has
// dropped its last name for it.

#include <memory>
#include <string>
#include <vector>

namespace satellite004 {

class satellite_window;
using WindowHandle = std::shared_ptr<satellite_window>;

class satellite_window {
public:
    // WHICH PIECE OF A WINDOW THIS IS. `window` is the thing with a frame;
    // `button` is a thing put inside one.
    enum Piece { window, button };

    Piece piece = window;

    // THE GtkWidget *, AS A void *. Only window_desk.cpp and satellite_window.cpp
    // ever cast it back, and both are compiled only where GTK is.
    void *widget = nullptr;

    // THE GtkFixed INSIDE A WINDOW, which is what `.append` puts a piece into.
    // GTK4 has no absolute position in a box, so a window that a program places
    // things in BY COORDINATE must hold a GtkFixed (WIN-3).
    void *inside = nullptr;

    // WHAT WAS APPENDED INTO IT, held so that a window going away can say so to
    // every piece inside it. GTK destroys a window's children with the window,
    // and a button's handle that a program still holds would otherwise keep a
    // GtkWidget * that GTK has already freed.
    std::vector<WindowHandle> pieces;

    std::string title;            // what it was made with, and what .title reads back
    std::string text;             // a button's label
    bool on_the_screen = false;   // false once it is closed, whoever closed it

    satellite_window() = default;
    explicit satellite_window(Piece which) : piece(which) {}

    const char *piece_name() const { return piece == window ? "a window" : "a button"; }
};

// ---------------------------------------------------------------------------
// WHAT A PROGRAM CAN DO TO ONE.
// ---------------------------------------------------------------------------
//
// EVERY ONE OF THESE IS CALLED ON THE INTERPRETER'S THREAD and does its work on
// the desk's (window_desk.hpp): GTK4 is not thread-safe and every call must
// happen on the thread that called gtk_init. None of them throws; a failure is
// `why` filled in and a null handle or false.
//
// THEY EXIST IN BOTH BUILDS. When pkg-config found no gtk4 the bodies are not
// compiled at all -- bytecode/window_calls.cpp refuses before it reaches one,
// which is the single place this satl says it was built without a window.

// `satellite.window.new(title, width, height)`. A width or a height of 0 is
// refused, because a window nobody can see is not what was asked for.
WindowHandle window_new(const std::string &title, unsigned long long int width,
                        unsigned long long int height, std::string &why);

// `satellite.window.button(text)`. Not on any screen until it is appended.
WindowHandle window_button(const std::string &text, std::string &why);

// `my_window.append(piece, x, y)` -- BY ITS CENTRE (WIN-3): 400, 300 is the
// middle of an 800x600 window, not a corner. The piece's own measured size is
// halved and taken off at placement, which is the whole difference.
bool window_append(satellite_window &into, const WindowHandle &piece, long long int x,
                   long long int y, std::string &why);

bool window_close(satellite_window &which, std::string &why);
bool window_focus(satellite_window &which, std::string &why);
bool window_set_title(satellite_window &which, const std::string &title, std::string &why);

// THE RUN DOES NOT END WHILE A WINDOW IS OPEN. Called once, from main(), after
// the program has returned: a program that opens a window and stops would
// otherwise take the window down with it before anybody saw it. Returns at once
// when nothing was ever opened, so a program with no window pays nothing.
//
// `the_program_finished` FALSE TAKES THE WINDOWS DOWN INSTEAD OF WAITING, and
// that difference was measured rather than designed: a program refused at a line
// AFTER it opened a window printed its report and then hung, because the window
// was still up and nothing was ever going to close it. A person who has just been
// told their program stopped is owed the prompt back, not a wait with no end.
void windows_stay_open_until_closed(bool the_program_finished);

} // namespace satellite004
