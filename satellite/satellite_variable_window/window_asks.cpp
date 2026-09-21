// satellite/satellite_variable_window/window_asks.cpp -- ASKING a piece what it
// says and whether it is on, and telling it otherwise.
// GTK_AND_NO_DEPENDENCIES.md GTK-2 and GTK-3.
//
// SPLIT OUT OF window_pieces.cpp AT GTK-3, at 297 lines against the author's
// "try to build for 300 lines". The line is `make` against `ask`: a piece is
// made once and asked for ever, and this is the half that has to cross to the
// desk and bring an answer back.
//
// WHAT A PIECE HOLDS IS NOT OURS, AND THAT IS THE WHOLE SUBJECT OF THIS FILE. A
// button's label satellite handed it. What a person TYPED, and whether they
// ticked the box, arrive without telling us -- so every read here goes to the
// desk, asks the widget, and answers what is actually on the screen.
//
// `text` HAS EXACTLY ONE WRITER AND IT IS THIS THREAD. Each on_the_desk() lambda
// writes a LOCAL, and the local is copied into the handle after the lambda has
// finished. A std::string written on the desk's thread and read on the
// interpreter's is undefined behaviour, and the only other fields these two
// threads share are a pointer and two bools.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

namespace satellite004 {
namespace {

// THE GtkTextBuffer OF A TEXT AREA, which is where its words actually live --
// the widget only draws them. Two iterators and a copy; there is no shorter way.
std::string what_a_text_area_says(GtkWidget *widget)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
    GtkTextIter from, to;
    gtk_text_buffer_get_bounds(buffer, &from, &to);
    gchar *got = gtk_text_buffer_get_text(buffer, &from, &to, FALSE);
    std::string out = got == nullptr ? std::string() : std::string(got);
    g_free(got);
    return out;
}

// WHAT A WIDGET SAYS, ASKED OF GTK. ON THE DESK'S THREAD ONLY -- every caller
// is inside an on_the_desk() lambda.
//
// A BUTTON AND A LABEL ARE IN HERE TOO even though the handle already knows: it
// costs one call and it means `.text` answers what is ON THE SCREEN rather than
// what satellite last wrote, which are the same thing today and need not stay
// so. NULL IS A REAL ANSWER from gtk_button_get_label -- a button made with an
// icon and no label has none -- so it is checked rather than handed to
// std::string, which would be undefined behaviour and not an empty string.
std::string what_a_piece_says(GtkWidget *widget, satellite_window::Piece piece)
{
    const char *got = nullptr;
    switch (piece) {
    case satellite_window::button:   got = gtk_button_get_label(GTK_BUTTON(widget)); break;
    case satellite_window::label:    got = gtk_label_get_text(GTK_LABEL(widget)); break;
    case satellite_window::text_box: got = gtk_editable_get_text(GTK_EDITABLE(widget)); break;
    case satellite_window::text_area: return what_a_text_area_says(widget);
    // A CHECKBOX'S WORDS ARE ITS LABEL. A SWITCH HAS NONE and answers "", which
    // is the truth rather than a refusal: a switch really does say nothing.
    case satellite_window::checkbox: got = gtk_check_button_get_label(GTK_CHECK_BUTTON(widget)); break;
    case satellite_window::a_switch: break;
    // A NUMBER IS NOT WORDS. A slider has no label and a progress bar's text is
    // GTK's own optional overlay, not a thing satellite gave it -- both answer
    // "" here and `.value` is what a program actually wants of them.
    case satellite_window::slider:
    case satellite_window::number_box:
    case satellite_window::progress: break;
    case satellite_window::window:
    case satellite_window::how_many_pieces: break;
    }
    return got == nullptr ? std::string() : std::string(got);
}

// ---------------------------------------------------------------------------
// ON AND OFF (GTK-3).
// ---------------------------------------------------------------------------
//
// ONLY A CHECKBOX AND A SWITCH, and everything else is refused by name rather
// than answered `false`. A label is not "off"; it has no such question, and
// inventing an answer for it is exactly the kind of quiet wrongness this
// language refuses.

bool is_turned_on_or_off(const satellite_window &which)
{
    return which.piece == satellite_window::checkbox || which.piece == satellite_window::a_switch;
}

// WHY `which` AND NOT A RAW WIDGET: a checkbox is a GtkCheckButton and a switch
// is a GtkSwitch, and their getters are different functions on unrelated types.
// The Piece is the only thing that can tell them apart.
bool is_it_on(GtkWidget *widget, satellite_window::Piece piece)
{
    if (piece == satellite_window::checkbox)
        return gtk_check_button_get_active(GTK_CHECK_BUTTON(widget)) != FALSE;
    return gtk_switch_get_active(GTK_SWITCH(widget)) != FALSE;
}

} // namespace

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
        case satellite_window::text_box: gtk_editable_set_text(GTK_EDITABLE(widget), text.c_str()); break;
        case satellite_window::text_area:
            gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)), text.c_str(), -1);
            break;
        case satellite_window::checkbox: gtk_check_button_set_label(GTK_CHECK_BUTTON(widget), text.c_str()); break;
        default: break;
        }
    });
    // AND THE HANDLE KEEPS ITS OWN COPY, because that is what a bare `.text`
    // reads back -- window_calls.cpp answers it without crossing to the desk.
    which.text = text;
    return true;
}

bool window_text_of(satellite_window &which, std::string &out, std::string &why)
{
    if (which.piece == satellite_window::window) {
        why = "a window's words are its title -- write .title instead";
        return false;
    }
    // A BUTTON'S AND A LABEL'S WORDS ARE OURS. Nothing but satellite ever writes
    // them, so the handle is the truth -- and it stays the truth after the window
    // has gone, which is why these two answer a closed piece and the next two do
    // not.
    if (which.widget == nullptr &&
        (which.piece == satellite_window::button || which.piece == satellite_window::label)) {
        out = which.text;
        return true;
    }
    // WHAT A PERSON TYPED IS GTK'S, AND WHEN GTK HAS FREED THE WIDGET IT IS GONE.
    // There is no moment in a teardown at which it can be rescued -- window_desk.cpp
    // says why, at the place it was tried. Answering the last thing satellite
    // happened to write instead would be an answer that is wrong and does not say
    // so, so this refuses and names when to read it.
    if (which.widget == nullptr) {
        why = "it is closed, and what was typed in it went with the window -- "
              "read .text while the window is still open";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const satellite_window::Piece piece = which.piece;
    std::string got;
    on_the_desk([widget, piece, &got] { got = what_a_piece_says(widget, piece); });
    // WRITTEN BACK ON THE INTERPRETER'S THREAD, which is the only thread that
    // ever writes `text`. on_the_desk() has already waited, so `got` is finished
    // being written before this line reads it, and the desk never touches this
    // string at all.
    which.text = got;
    out = got;
    return true;
}

bool window_on_of(satellite_window &which, bool &out, std::string &why)
{
    if (!is_turned_on_or_off(which)) {
        why = std::string(which.piece_name()) + " is not something that is turned on or off -- "
              "a checkbox and a switch are";
        return false;
    }
    // NOTHING TO FALL BACK ON, unlike `.text`. A checkbox's state was never
    // satellite's, so a closed one has no last-known answer that is worth
    // anything -- it is refused, and the sentence is the same one `.text` uses.
    if (which.widget == nullptr) {
        why = "it is closed -- read .on while the window is still open";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const satellite_window::Piece piece = which.piece;
    bool got = false;
    on_the_desk([widget, piece, &got] { got = is_it_on(widget, piece); });
    out = got;
    return true;
}

bool window_set_on(satellite_window &which, bool on, std::string &why)
{
    if (!is_turned_on_or_off(which)) {
        why = std::string(which.piece_name()) + " is not something that is turned on or off -- "
              "a checkbox and a switch are";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const satellite_window::Piece piece = which.piece;
    on_the_desk([widget, piece, on] {
        if (piece == satellite_window::checkbox)
            gtk_check_button_set_active(GTK_CHECK_BUTTON(widget), on ? TRUE : FALSE);
        else
            gtk_switch_set_active(GTK_SWITCH(widget), on ? TRUE : FALSE);
    });
    return true;
}

// ---------------------------------------------------------------------------
// THE NUMBER A PIECE IS AT (GTK-4).
// ---------------------------------------------------------------------------
//
// THE UNITS ARE THE PIECE'S OWN and satellite_window.hpp says which: a slider
// and a number box are whole numbers, and a progress bar is MILLIONTHS. The
// millionths exist because GTK holds a double and satellite's percentage is
// exact to 32 digits; a whole number of millionths is the largest unit both can
// say without either of them rounding. window_calls.cpp does the rest.
namespace {

constexpr long long int kMillion = 1000000;

bool has_a_value(const satellite_window &which)
{
    return which.piece == satellite_window::slider || which.piece == satellite_window::number_box ||
           which.piece == satellite_window::progress;
}

} // namespace

bool window_value_of(satellite_window &which, long long int &out, std::string &why)
{
    if (!has_a_value(which)) {
        why = std::string(which.piece_name()) + " has no number -- a slider, a number box and a "
              "progress bar do";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed -- read .value while the window is still open";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const satellite_window::Piece piece = which.piece;
    double got = 0.0;
    on_the_desk([widget, piece, &got] {
        if (piece == satellite_window::progress)
            got = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(widget)) * static_cast<double>(kMillion);
        else if (piece == satellite_window::number_box)
            got = gtk_spin_button_get_value(GTK_SPIN_BUTTON(widget));
        else
            got = gtk_range_get_value(GTK_RANGE(widget));
    });
    // ROUNDED, NOT TRUNCATED. GTK's double for a slider left exactly on 50 can
    // come back as 49.999999999999996, and a language whose numbers are whole
    // must not answer 49 for a slider a person put on 50.
    out = static_cast<long long int>(got < 0.0 ? got - 0.5 : got + 0.5);
    return true;
}

bool window_set_value(satellite_window &which, long long int to, std::string &why)
{
    if (!has_a_value(which)) {
        why = std::string(which.piece_name()) + " has no number -- a slider, a number box and a "
              "progress bar do";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    const satellite_window::Piece piece = which.piece;
    on_the_desk([widget, piece, to] {
        if (piece == satellite_window::progress) {
            // CLAMPED HERE AND NOT REFUSED. A progress bar past its end is a
            // program counting slightly wrong, not a program that has gone
            // wrong -- and GTK draws a fraction above 1.0 as a bar longer than
            // its own frame. A slider and a number box need no clamp: GTK holds
            // them inside the range they were made with.
            const double fraction = static_cast<double>(to) / static_cast<double>(kMillion);
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(widget),
                                          fraction < 0.0 ? 0.0 : (fraction > 1.0 ? 1.0 : fraction));
        } else if (piece == satellite_window::number_box) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), static_cast<double>(to));
        } else {
            gtk_range_set_value(GTK_RANGE(widget), static_cast<double>(to));
        }
    });
    return true;
}

} // namespace satellite004
