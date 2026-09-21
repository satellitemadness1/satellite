// satellite/satellite_variable_window/window_pieces.cpp -- MAKING the things
// that go IN a window. GTK_AND_NO_DEPENDENCIES.md GTK-1.
//
// SPLIT OUT OF satellite_window.cpp ON 2026-09-21, and the reason is the line
// rule rather than tidiness: that file was 274 lines against the author's "try
// to build for 300 lines" and GTK-1 alone would have passed it. Eighteen widget
// milestones are written down; the first of them is where the split is cheap.
//
// SPLIT AGAIN AT GTK-3, for the same rule and at 297 lines. THREE FILES NOW:
//
//   satellite_window.cpp   the WINDOW -- making one, closing, focusing, titling,
//                          appending into it, and holding the run open
//   window_pieces.cpp      MAKING a piece: one factory for all of them
//   window_asks.cpp        ASKING a piece: .text and .on, read and written
//
// The line between the last two is `make` against `ask`, and it is not arbitrary:
// making happens once and asking happens for ever, and the asking half is the
// half that has to cross to the desk and bring a value back.
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
    case satellite_window::text_box: {
        // GtkEntry TAKES NO TEXT AT CONSTRUCTION, which is the first place the
        // one-call shape above stops fitting -- and the reason a_widget_for is a
        // switch rather than a table of function pointers.
        GtkWidget *made = gtk_entry_new();
        gtk_editable_set_text(GTK_EDITABLE(made), text.c_str());
        return made;
    }
    case satellite_window::text_area: {
        GtkWidget *made = gtk_text_view_new();
        gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(made)), text.c_str(), -1);
        // A SIZE, AND IT IS NOT AN ARBITRARY ONE. A GtkTextView holding nothing
        // measures almost nothing, and `.append` places a piece BY ITS MEASURED
        // SIZE -- so a text area put in a window would be a few pixels of nothing
        // that a person cannot find, let alone click into. That is the same
        // failure window_new() refuses for a window of size 0: an answer that is
        // wrong and does not say so. GTK-8's `.resize` is how a program says
        // otherwise.
        gtk_widget_set_size_request(made, 300, 150);
        return made;
    }
    case satellite_window::checkbox: return gtk_check_button_new_with_label(text.c_str());
    // A SWITCH TAKES NO WORDS, and `text` is empty here because its word takes
    // nothing -- satellite.window.switch(). A switch with a label beside it is a
    // switch and a label, which is two pieces and GTK-7's business.
    case satellite_window::a_switch: return gtk_switch_new();
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

} // namespace satellite004
