// satellite/satellite_variable_window/window_pieces.cpp -- the things that go IN
// a window, and what their words are. GTK_AND_NO_DEPENDENCIES.md GTK-1.
//
// SPLIT OUT OF satellite_window.cpp ON 2026-09-21, and the reason is the line
// rule rather than tidiness: that file was 274 lines against the author's "try
// to build for 300 lines" and GTK-1 alone would have passed it. Eighteen widget
// milestones are written down; the first of them is where the split is cheap.
//
// WHAT IS HERE AND WHAT IS NEXT DOOR: satellite_window.cpp keeps the WINDOW --
// making one, closing it, its title, what is appended into it, and holding the
// run open. This file keeps every piece that is put inside one.
//
// EVERY GTK CALL HAPPENS INSIDE on_the_desk(), which is window_desk.hpp's whole
// reason to exist: GTK4 is not thread-safe and the program runs on another
// thread. A GTK call written outside one of these lambdas would work most of the
// time, which is the worst way for it to be wrong.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbour.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

namespace satellite004 {
namespace {

// WHICH GtkWidget EACH PIECE IS, AND NOTHING ELSE. A switch and not a table of
// function pointers, because the shapes are already diverging: a button and a
// label both take their words at construction and GTK-2's text box takes none
// at all.
//
// EVERY ENUMERATOR IS LISTED AND THERE IS NO `default`, on purpose -- that is
// what makes a `Piece` added to the enum and not given a widget here a COMPILER
// WARNING rather than a null the caller has to explain. kPieceNames does the
// same thing with a static_assert; between them a new widget cannot be half
// added.
GtkWidget *a_widget_for(satellite_window::Piece which, const std::string &text)
{
    switch (which) {
    case satellite_window::button: return gtk_button_new_with_label(text.c_str());
    case satellite_window::label:  return gtk_label_new(text.c_str());
    case satellite_window::window:
    case satellite_window::how_many_pieces: break;
    }
    return nullptr;
}

} // namespace

// EVERY PIECE THAT IS MADE FROM A LINE OF TEXT, MADE ONE WAY. They differ in
// exactly one thing -- which GtkWidget is asked for -- so the handle, the desk,
// the words and the not-on-a-screen-yet rule are written here once and not once
// a widget.
//
// NOT on_the_screen: a piece is nothing until it is appended, and `.append` is
// what puts it on one. A piece that thought it was on a screen before then would
// be pressable before anybody could have pressed it, which is exactly what
// window_press() refuses (WIN-11).
WindowHandle window_piece_of_text(satellite_window::Piece which, const std::string &text,
                                  std::string &why)
{
    // REFUSED BEFORE THE DESK IS OPENED, because opening the desk connects to a
    // compositor and a program that asked for something that is not a piece
    // should not pay for one. A window is the case that gets here in practice --
    // window_new() is what makes one of those.
    if (which == satellite_window::window || which >= satellite_window::how_many_pieces) {
        why = "that is not a piece made from a line of text";
        return nullptr;
    }
    if (!open_the_desk(why))
        return nullptr;
    WindowHandle made = std::make_shared<satellite_window>(which);
    made->text = text;
    satellite_window *raw = made.get();
    on_the_desk([raw, &text, which] { raw->widget = a_widget_for(which, text); });
    return made;
}

bool window_set_text(satellite_window &which, const std::string &text, std::string &why)
{
    // A WINDOW IS TOLD THE RIGHT WORD RATHER THAN GIVEN A SECOND ONE. Answering
    // the title to `.text` would be two names for one thing, and a person who
    // wrote `.text` on a window meant `.title` -- so say that.
    if (which.piece == satellite_window::window) {
        why = "a window's words are its title -- write .title(\"text\") instead";
        return false;
    }
    // NOT still_there(): a piece is on no screen until it is appended, and
    // saying what it says BEFORE putting it in a window is the ordinary order to
    // write it in. What must be true is that the widget still exists -- a piece
    // whose window has been closed has had its GtkWidget * nulled.
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const satellite_window::Piece piece = which.piece;
    on_the_desk([widget, piece, &text] {
        // ONE SETTER A PIECE, AND NO DEFAULT. A piece that reaches here without a
        // case has no words to set, and window_calls.cpp refused it long before:
        // `.text` is offered by the pieces below and by nothing else.
        switch (piece) {
        case satellite_window::button: gtk_button_set_label(GTK_BUTTON(widget), text.c_str()); break;
        case satellite_window::label:  gtk_label_set_text(GTK_LABEL(widget), text.c_str()); break;
        default: break;
        }
    });
    // AND THE HANDLE KEEPS ITS OWN COPY, because that is what a bare `.text`
    // reads back -- window_calls.cpp answers it without crossing to the desk.
    which.text = text;
    return true;
}

} // namespace satellite004
