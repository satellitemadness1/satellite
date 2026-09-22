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
    // A ROW, A COLUMN AND A GRID TAKE NO WORDS EITHER, and they are here rather
    // than in a factory of their own because a container IS a widget -- what
    // makes them different is what `.append` does with them, not how they are
    // made.
    case satellite_window::row:
        return gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    case satellite_window::column:
        return gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    case satellite_window::scroll: {
        GtkWidget *made = gtk_scrolled_window_new();
        // A SCROLL WITH NO SIZE MEASURES ALMOST NOTHING, the trap the text area
        // and the slider both walked into: `.append` places by the measured
        // size, so a scroll left to itself is a few pixels nobody can see into.
        gtk_widget_set_size_request(made, 300, 200);
        return made;
    }
    // A FRAME IS THE ONE HOLDER WITH WORDS: they are drawn on its edge, which is
    // what a frame is for.
    case satellite_window::frame: return gtk_frame_new(text.empty() ? nullptr : text.c_str());
    case satellite_window::split: {
        GtkWidget *made = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_widget_set_size_request(made, 300, 200);
        return made;
    }
    case satellite_window::grid: {
        GtkWidget *made = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(made), 6);
        gtk_grid_set_column_spacing(GTK_GRID(made), 6);
        return made;
    }
    // A SET OF TABS TAKES NO WORDS: each tab's name is its PIECE's own `.title`
    // (GTK-16), read when the piece is appended. THE BAR IS ALWAYS SHOWN, unlike
    // satl-term's, which hides it with one page: a program that made tabs wants
    // to see tabs, and a set with one page that looked like a plain piece would
    // be a widget saying nothing about what it is.
    case satellite_window::tabs: return gtk_notebook_new();
    // MADE FROM NUMBERS AND NOT FROM WORDS, so they are not this function's --
    // window_piece_of_numbers below is where they are made, and reaching here
    // with one is window_calls.cpp having read the table wrongly.
    case satellite_window::slider:
    case satellite_window::number_box:
    case satellite_window::progress:
    case satellite_window::choice:
    case satellite_window::one_of:
    case satellite_window::picture:
    // AND A MENU IS NOT A WIDGET AT ALL (GTK-12): window_menu.cpp makes it, out
    // of a GMenu and an action group, and window_piece_of_text routes there
    // before it ever asks this function.
    case satellite_window::menu:
    // AND A CANVAS IS MADE FROM ITS SIZE (GTK-15), in window_canvas.cpp.
    case satellite_window::canvas:
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
    // A MENU IS MADE FROM A LINE OF TEXT -- its heading -- AND IS NOT A WIDGET,
    // so it has a factory of its own (window_menu.cpp) and goes there.
    if (which == satellite_window::menu)
        return window_piece_of_a_menu(text, why);
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

namespace {

// A RADIO GROUP, AS GTK4 MAKES ONE (GTK-3's radio, built 2026-09-22): a check
// button a word, in a column, and every button after the first given the
// FIRST as its group -- which is what turns a check button into a radio. ON
// THE DESK.
//
// THE FIRST IS TICKED FROM THE START. GTK4 would leave none ticked, and a
// choice shows its first item from the start; a one-of that showed nothing
// picked would have `.chosen` answer "" for a control that looks like it has
// an answer, and a person cannot un-pick a radio once one is picked anyway.
GtkWidget *a_radio_group_of(const char *const *words)
{
    GtkWidget *column = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkCheckButton *first = nullptr;
    for (; *words != nullptr; ++words) {
        GtkWidget *button = gtk_check_button_new_with_label(*words);
        if (first == nullptr) {
            first = GTK_CHECK_BUTTON(button);
            gtk_check_button_set_active(first, TRUE);
        } else {
            gtk_check_button_set_group(GTK_CHECK_BUTTON(button), first);
        }
        gtk_box_append(GTK_BOX(column), button);
    }
    return column;
}

} // namespace

WindowHandle window_piece_of_items(satellite_window::Piece which,
                                   const std::vector<std::string> &items, std::string &why)
{
    const bool a_radio = which == satellite_window::one_of;
    if (which != satellite_window::choice && !a_radio) {
        why = "that is not a piece made from a list";
        return nullptr;
    }
    if (items.empty()) {
        why = std::string(a_radio ? "a one-of" : "a choice") +
              " needs something to choose from, and it was given an empty list";
        return nullptr;
    }
    if (!open_the_desk(why))
        return nullptr;
    WindowHandle made = std::make_shared<satellite_window>(which);
    satellite_window *raw = made.get();
    // NULL-TERMINATED, WHICH IS GTK'S SHAPE AND NOT OURS. The pointers point
    // into `items`, which is the CALLER's vector and outlives this call --
    // on_the_desk() waits, so the lambda has finished before the caller's frame
    // can end. Copying the strings again would be copying them a third time.
    std::vector<const char *> as_gtk_wants;
    as_gtk_wants.reserve(items.size() + 1);
    for (const std::string &item : items)
        as_gtk_wants.push_back(item.c_str());
    as_gtk_wants.push_back(nullptr);
    const char *const *strings = as_gtk_wants.data();
    on_the_desk([raw, strings, a_radio] {
        raw->widget = a_radio ? a_radio_group_of(strings) : gtk_drop_down_new_from_strings(strings);
    });
    return made;
}

// ---------------------------------------------------------------------------
// A PICTURE (GTK-6), AND THE FOUR PROJECTS IT SWITCHES ON.
// ---------------------------------------------------------------------------
namespace {

// A FILE INTO SOMETHING THAT CAN BE DRAWN, WITH ITS ERROR. ON THE DESK'S THREAD.
//
// gdk_texture_new_from_filename IS THE ONE WITH A GError, which is the whole
// reason it is used instead of gtk_picture_new_for_filename: that one takes a
// path that is not there, hands back a widget that draws nothing, and says
// nothing at all. A person would see an empty space where their logo should be
// and have no way to find out why.
//
// THE MESSAGE IS GLib'S OWN and it is better than anything we would write: it
// names the file, and it tells a missing file apart from an unreadable one and
// from one that is there and is not a picture.
GdkTexture *a_texture_from(const std::string &path, std::string &why)
{
    GError *went_wrong = nullptr;
    GdkTexture *made = gdk_texture_new_from_filename(path.c_str(), &went_wrong);
    if (made == nullptr) {
        why = went_wrong != nullptr && went_wrong->message != nullptr
                  ? std::string(went_wrong->message)
                  : "it could not be read as a picture";
        if (went_wrong != nullptr)
            g_error_free(went_wrong);
        return nullptr;
    }
    return made;
}

} // namespace

WindowHandle window_piece_of_a_file(satellite_window::Piece which, const std::string &path,
                                    std::string &why)
{
    if (which != satellite_window::picture) {
        why = "that is not a piece made from a file";
        return nullptr;
    }
    if (path.empty()) {
        why = "a picture needs the name of a file, and it was given nothing";
        return nullptr;
    }
    // THE DESK IS OPENED BEFORE THE FILE IS READ, and that order is not free
    // choice: gdk_texture_new_from_filename wants GDK started. A machine with no
    // screen therefore says NO_DISPLAY rather than complaining about the file,
    // which is right -- the file is not the reason that run cannot draw.
    if (!open_the_desk(why))
        return nullptr;
    WindowHandle made = std::make_shared<satellite_window>(which);
    satellite_window *raw = made.get();
    std::string went_wrong;
    on_the_desk([raw, &path, &went_wrong] {
        GdkTexture *drawn = a_texture_from(path, went_wrong);
        if (drawn == nullptr)
            return;
        raw->widget = gtk_picture_new_for_paintable(GDK_PAINTABLE(drawn));
        // THE PICTURE TOOK ITS OWN REFERENCE. Ours is spent.
        g_object_unref(drawn);
    });
    if (raw->widget == nullptr) {
        why = went_wrong;
        return nullptr;
    }
    // WHERE IT GOT WHAT IT DRAWS, kept so `.path` can answer after the window
    // has closed -- satellite opened that file, so it is ours to remember.
    made->text = path;
    return made;
}

bool window_path_of(satellite_window &which, std::string &out, std::string &why)
{
    if (which.piece != satellite_window::picture) {
        why = std::string(which.piece_name()) + " shows no file -- a picture does";
        return false;
    }
    out = which.text;
    return true;
}

bool window_set_path(satellite_window &which, const std::string &to, std::string &why)
{
    if (which.piece != satellite_window::picture) {
        why = std::string(which.piece_name()) + " shows no file -- a picture does";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    std::string went_wrong;
    bool swapped = false;
    on_the_desk([widget, &to, &went_wrong, &swapped] {
        GdkTexture *drawn = a_texture_from(to, went_wrong);
        if (drawn == nullptr)
            return;
        gtk_picture_set_paintable(GTK_PICTURE(widget), GDK_PAINTABLE(drawn));
        g_object_unref(drawn);
        swapped = true;
    });
    if (!swapped) {
        // THE OLD PICTURE IS STILL THERE, and `.path` still answers it. A
        // failed change must not leave a piece saying it shows a file it does
        // not -- which is what writing `which.text` before checking would have
        // done.
        why = went_wrong;
        return false;
    }
    which.text = to;
    return true;
}

} // namespace satellite004
