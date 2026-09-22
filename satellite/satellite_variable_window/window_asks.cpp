// satellite/satellite_variable_window/window_asks.cpp -- A PIECE'S WORDS, read
// and written. GTK_AND_NO_DEPENDENCIES.md GTK-2.
//
// SPLIT OUT OF window_pieces.cpp AT GTK-3 and split again at GTK-5, both times
// against the author's "try to build for 300 lines". Four files now:
//
//   satellite_window.cpp   the WINDOW itself
//   window_pieces.cpp      MAKING a piece -- three factories, one a shape
//   window_asks.cpp        a piece's WORDS: .text, read and written
//   window_state.cpp       what a piece is SET TO: .on, .value, .chosen
//
// The line between the last two is words against state. They are different
// questions -- a label's words are satellite's own and a checkbox's tick never
// was -- and they were always going to be asked by different pieces.
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
    // A CHOICE'S WORDS ARE WHICH ITEM IS PICKED, and `.chosen` is the word for
    // that. `.text` answering the same thing would be two names for one thing.
    case satellite_window::choice: break;
    // A CONTAINER SAYS NOTHING OF ITS OWN. What is IN it says things, and a row
    // answering the words of its first piece would be a guess nobody asked for.
    case satellite_window::row:
    case satellite_window::column:
    case satellite_window::grid:
    case satellite_window::scroll:
    case satellite_window::split: break;
    // A FRAME'S WORDS ARE THE ONES ON ITS EDGE, which is the one holder that has
    // any -- so `.text` answers them and `.text("...")` writes them.
    case satellite_window::frame: {
        const char *edge = gtk_frame_get_label(GTK_FRAME(widget));
        return edge == nullptr ? std::string() : std::string(edge);
    }
    // A PICTURE'S WORDS ARE ITS FILE, AND `.path` IS THE WORD FOR THAT. It is
    // refused here rather than answered, the same way a window is sent to
    // `.title` -- a path is not words on a piece, it is where a piece got what
    // it draws.
    case satellite_window::picture: break;
    // A MENU'S WORDS ARE ITS HEADING AND THE HANDLE HOLDS THEM (GTK-12); it
    // never reaches here, because a menu is not a widget to be asked.
    case satellite_window::menu: break;
    case satellite_window::window:
    case satellite_window::how_many_pieces: break;
    }
    return got == nullptr ? std::string() : std::string(got);
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
    if (which.piece == satellite_window::picture) {
        why = "a picture's words are the file it shows -- write .path(\"other.png\") instead";
        return false;
    }
    // A MENU'S WORDS ARE ITS HEADING, and changing one on a bar is a remove and
    // an insert on the bar's model rather than a setter on a widget (GTK-12).
    if (which.piece == satellite_window::menu)
        return window_menu_heading(which, text, why);
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
        case satellite_window::frame:
            gtk_frame_set_label(GTK_FRAME(widget), text.empty() ? nullptr : text.c_str());
            break;
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
    if (which.piece == satellite_window::picture) {
        why = "a picture's words are the file it shows -- write .path instead";
        return false;
    }
    // A MENU'S HEADING IS OURS, LIKE A LABEL'S WORDS: satellite put it there and
    // nothing else writes it, so the handle answers, open or closed, and the
    // desk is never asked (GTK-12).
    if (which.piece == satellite_window::menu) {
        out = which.text;
        return true;
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

} // namespace satellite004
