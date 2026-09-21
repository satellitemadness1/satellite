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
    // MADE FROM NUMBERS AND NOT FROM WORDS, so they are not this function's --
    // window_piece_of_numbers below is where they are made, and reaching here
    // with one is window_calls.cpp having read the table wrongly.
    case satellite_window::slider:
    case satellite_window::number_box:
    case satellite_window::progress:
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

WindowHandle window_piece_of_numbers(satellite_window::Piece which, long long int least,
                                     long long int most, std::string &why)
{
    const bool a_range = which == satellite_window::slider || which == satellite_window::number_box;
    if (!a_range && which != satellite_window::progress) {
        why = "that is not a piece made from numbers";
        return nullptr;
    }
    // A RANGE OF NOTHING IS NOT A RANGE. GTK takes least == most and draws a
    // slider that cannot be moved -- a thing on the screen that looks like a
    // control and is not one, which is the shape of wrongness this project
    // refuses everywhere else. Refused where it is written.
    if (a_range && least >= most) {
        why = "a slider runs from its least to its most, so the least must be the smaller of the two";
        return nullptr;
    }
    if (!open_the_desk(why))
        return nullptr;
    WindowHandle made = std::make_shared<satellite_window>(which);
    satellite_window *raw = made.get();
    const double from = static_cast<double>(least), to = static_cast<double>(most);
    on_the_desk([raw, which, from, to] {
        GtkWidget *made_here = nullptr;
        switch (which) {
        case satellite_window::slider:
            made_here = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, from, to, 1.0);
            // A SLIDER IN A GtkFixed MEASURES ALMOST NOTHING, the same trap the
            // text area walked into: `.append` places by the measured size, so a
            // slider left to itself is a few pixels nobody can drag. GTK-8's
            // .resize is how a program says otherwise.
            gtk_widget_set_size_request(made_here, 300, -1);
            // THE NUMBER IS NOT DRAWN. GTK shows it by default and satellite has
            // a label for that -- and a slider that prints "50.000000" under
            // itself is GTK's double leaking into a language that has none.
            gtk_scale_set_draw_value(GTK_SCALE(made_here), FALSE);
            break;
        case satellite_window::number_box:
            made_here = gtk_spin_button_new_with_range(from, to, 1.0);
            break;
        default:
            made_here = gtk_progress_bar_new();
            gtk_widget_set_size_request(made_here, 300, -1);
            break;
        }
        raw->widget = made_here;
    });
    return made;
}

} // namespace satellite004
