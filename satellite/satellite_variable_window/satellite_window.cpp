// satellite/satellite_variable_window/satellite_window.cpp -- what a program can
// do to a WINDOW. SATELLITE_WINDOW.md WIN-3.
//
// THE PIECES THAT GO INSIDE ONE ARE NEXT DOOR, in window_pieces.cpp since
// GTK-1. This file makes a window, closes it, focuses it, titles it, appends
// into it and holds the run open; that file makes a button, a label and
// whatever comes after them.
//
// EVERY GTK CALL IN THIS FILE HAPPENS INSIDE on_the_desk(), which is the rule
// window_desk.hpp exists to keep: GTK4 is not thread-safe and the interpreter
// runs the program on its own thread. A GTK call written outside one of these
// lambdas would work most of the time, which is the worst way for it to be
// wrong.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4. bytecode/window_calls.cpp is the
// one file that says so when this satl was built without a window.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

namespace satellite004 {
namespace {

// A WINDOW WENT AWAY, whoever took it away: a person clicking the close button
// and `my_window.close()` both arrive here, because both are GTK's `destroy`.
// That is the point of connecting it rather than doing the bookkeeping in
// window_close() -- one of those two would otherwise be unaccounted for.
void it_was_closed(GtkWidget *, gpointer user_data)
{
    the_desk_let_go_of(static_cast<satellite_window *>(user_data));
}

// A BUTTON WAS PRESSED (WIN-11). ON THE DESK'S THREAD, so it does the one thing
// that is safe here -- writes the capsule's name on the desk's queue -- and the
// interpreter's thread takes it off and walks it. window_desk.hpp says why the
// walker must not be entered from here.
//
// `when_pressed` IS READ ON THIS THREAD AND WRITTEN ON IT, which is what makes
// reading it without a lock right: window_pressed() sets it inside on_the_desk().
void it_was_pressed(GtkWidget *, gpointer user_data)
{
    satellite_window *button = static_cast<satellite_window *>(user_data);
    if (!button->when_pressed.empty())
        the_desk_saw_a_press(button->when_pressed, button->shared_from_this());
}

GtkWidget *as_widget(const satellite_window &which) { return static_cast<GtkWidget *>(which.widget); }

// A WINDOW THAT IS NO LONGER ON A SCREEN, said the same way everywhere.
bool still_there(const satellite_window &which, std::string &why)
{
    if (which.widget != nullptr && which.on_the_screen)
        return true;
    why = "it is closed";
    return false;
}

} // namespace

WindowHandle window_new(const std::string &title, unsigned long long int width,
                        unsigned long long int height, std::string &why)
{
    // A WINDOW NOBODY CAN SEE IS NOT WHAT WAS ASKED FOR. GTK takes a 0 and draws
    // a window of whatever size it likes, which is an answer that is wrong and
    // does not say so -- the thing SATELLITE_FILE_OPERATIONS refuses to do.
    if (width == 0 || height == 0) {
        why = "a window's width and height must both be more than 0";
        return nullptr;
    }
    // GTK's own sizes are int. A width past that is not a window, it is a typo.
    if (width > 32767 || height > 32767) {
        why = "a window's width and height must each be 32767 or less";
        return nullptr;
    }
    if (!open_the_desk(why))
        return nullptr;

    WindowHandle made = std::make_shared<satellite_window>(satellite_window::window);
    made->title = title;
    // TOLD TO THE DESK BEFORE IT IS PRESENTED, so a window that is closed the
    // instant it appears is still a window the desk knows how to let go of.
    made->on_the_screen = true;
    the_desk_holds(made);

    satellite_window *raw = made.get();
    const int wide = static_cast<int>(width), tall = static_cast<int>(height);
    on_the_desk([raw, &title, wide, tall] {
        // NO GtkApplication, ON PURPOSE (hello-static.c:17-22): GtkApplication is
        // GApplication, which registers on the D-Bus session bus, and a machine
        // that satl is shipped to may have none. gtk_window_new() needs none of it.
        GtkWidget *window = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(window), title.c_str());
        gtk_window_set_default_size(GTK_WINDOW(window), wide, tall);
        // GTK4 HAS NO ABSOLUTE POSITION IN A BOX, and `.append` places by
        // coordinate, so every window holds a GtkFixed to put pieces into (WIN-3).
        GtkWidget *inside = gtk_fixed_new();
        gtk_window_set_child(GTK_WINDOW(window), inside);
        g_signal_connect(window, "destroy", G_CALLBACK(it_was_closed), raw);
        raw->widget = window;
        raw->inside = inside;
        gtk_window_present(GTK_WINDOW(window));
    });
    return made;
}

bool window_append(satellite_window &into, const WindowHandle &piece, bool by_place,
                   long long int x, long long int y, std::string &why)
{
    const bool a_window = into.piece == satellite_window::window;
    if (!a_window && !into.holds_pieces()) {
        why = std::string(into.piece_name()) + " holds nothing -- a window, a row, a column and a "
              "grid do";
        return false;
    }
    // A WINDOW AND A GRID PLACE BY COORDINATE; A ROW AND A COLUMN DO NOT. Which
    // one is right is the RECEIVER'S, and the checker cannot know it -- a
    // satellite.variable.window name may hold either, and which it holds is not
    // decided until the line that makes it runs. So the checker lets both counts
    // through and this is where the wrong one is named.
    const bool wants_a_place = a_window || into.piece == satellite_window::grid;
    if (wants_a_place != by_place) {
        why = wants_a_place
                  ? std::string(into.piece_name()) + " places what is put in it, so .append takes the "
                    "piece and where it goes: .append(the_piece, across, down)"
                  : std::string(into.piece_name()) + " puts its pieces one after another, so .append "
                    "takes just the piece: .append(the_piece)";
        return false;
    }
    // A WINDOW MUST BE ON A SCREEN; A ROW NEED NOT BE. A row is built and filled
    // BEFORE it goes into a window, which is the ordinary order to write those
    // lines in -- so what must be true of a container is only that its widget is
    // still there.
    if (a_window ? !still_there(into, why) : into.widget == nullptr) {
        if (!a_window)
            why = "it is closed";
        return false;
    }
    if (piece == nullptr || piece->widget == nullptr) {
        why = "there is nothing here to append";
        return false;
    }
    if (piece->widget == into.widget) {
        why = "a piece cannot be put inside itself";
        return false;
    }
    // ALREADY SOMEWHERE. GTK refuses to give a widget a second parent and prints
    // its own critical warning; satellite says it in a sentence first.
    if (piece->inside_of.lock() != nullptr) {
        why = "that piece is already in " + std::string(piece->inside_of.lock()->piece_name());
        return false;
    }
    // AND NOT INTO SOMETHING IT ALREADY HOLDS, which is the only way the
    // `inside_of` chain could be made to loop -- and a loop there is a hang in
    // the_window_holding(), which every press walks.
    for (WindowHandle above = into.weak_from_this().lock(); above != nullptr;
         above = above->inside_of.lock()) {
        if (above.get() == piece.get()) {
            why = "a piece cannot be put inside something it already holds";
            return false;
        }
    }
    satellite_window *raw = piece.get();
    void *inside = a_window ? into.inside : into.widget;
    const satellite_window::Piece holder = into.piece;
    const int at_x = static_cast<int>(x), at_y = static_cast<int>(y);
    on_the_desk([raw, inside, holder, at_x, at_y] {
        GtkWidget *widget = static_cast<GtkWidget *>(raw->widget);
        GtkWidget *into_this = static_cast<GtkWidget *>(inside);
        if (holder == satellite_window::grid) {
            // A CELL, COUNTING FROM 1, which is how satellite counts a file's
            // lines (SATELLITE_FILE_OPERATIONS, the author: "all line numbers
            // start at 1"). GTK counts cells from 0, and the one subtraction is
            // here so that no program ever has to know that.
            gtk_grid_attach(GTK_GRID(into_this), widget, at_x - 1, at_y - 1, 1, 1);
            return;
        }
        if (holder != satellite_window::window) {
            gtk_box_append(GTK_BOX(into_this), widget);
            return;
        }
        // BY ITS CENTRE, NOT ITS CORNER (WIN-3): 400, 300 is the middle of an
        // 800x600 window. GtkFixed places by the top-left, so the piece is
        // measured and half of each side is taken off -- which is the whole
        // difference between the author's spelling and GTK's.
        int least = 0, natural = 0, wide = 0, tall = 0;
        gtk_widget_measure(widget, GTK_ORIENTATION_HORIZONTAL, -1, &least, &natural, nullptr, nullptr);
        wide = natural;
        gtk_widget_measure(widget, GTK_ORIENTATION_VERTICAL, wide, &least, &natural, nullptr, nullptr);
        tall = natural;
        gtk_fixed_put(GTK_FIXED(into_this), widget,
                      static_cast<double>(at_x) - wide / 2.0, static_cast<double>(at_y) - tall / 2.0);
    });
    // ON A SCREEN ONLY IF WHAT IT WENT INTO IS. A button appended into a row
    // that is not in a window yet is not on any screen, and saying it was would
    // let `.press()` pretend a person clicked something nobody could see.
    piece->on_the_screen = a_window || into.on_the_screen;
    // HELD BY THE WINDOW, so that the window going away can null this piece's
    // GtkWidget * before GTK frees it underneath a handle the program still has.
    into.pieces.push_back(piece);
    // AND THE PIECE KNOWS WHAT IT IS IN, which is how a pressed capsule reaches
    // the window: `.append` is the one place a piece ever enters anything, so it
    // is the one place that can say so. IT NAMES THE IMMEDIATE PARENT -- a row,
    // if that is what it went into -- and the_window_holding() walks the rest.
    piece->inside_of = into.weak_from_this();
    // AND A ROW THAT IS ALREADY IN A WINDOW PUTS EVERYTHING IT HOLDS ON THE
    // SCREEN WITH IT. Filling a row first and appending it after is the ordinary
    // order; appending it first and filling it later is just as legal, and the
    // pieces inside were marked not-on-a-screen when they went in.
    if (piece->holds_pieces() && piece->on_the_screen)
        for (const WindowHandle &inside_it : piece->pieces)
            inside_it->on_the_screen = true;
    return true;
}

WindowHandle the_window_holding(const satellite_window &piece)
{
    // CAPPED, AND THE CAP IS NOT THE DESIGN. `.append` refuses to make a loop,
    // so this walks a tree; the count is here so that a defect in that refusal
    // is a refusal here rather than a hang inside a press, which is the one
    // place a hang would look exactly like satl locking up.
    WindowHandle above = piece.inside_of.lock();
    for (int steps = 0; above != nullptr && steps < 4096; ++steps) {
        if (above->piece == satellite_window::window)
            return above;
        above = above->inside_of.lock();
    }
    return nullptr;
}

bool window_pressed(satellite_window &which, const std::string &capsule, std::string &why)
{
    if (which.piece != satellite_window::button) {
        why = "only a button is pressed";
        return false;
    }
    // NOT still_there(): a button is not on a screen until it is appended, and
    // saying what a press does BEFORE putting it in a window is the ordinary
    // order to write it in. What must be true is that the widget still exists --
    // a button whose window has been closed has had its GtkWidget * nulled.
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    satellite_window *raw = &which;
    on_the_desk([raw, &capsule] {
        raw->when_pressed = capsule;
        // CONNECTED ONCE, AND NOT ONCE A CALL. `b.pressed(a).pressed(b)` is a
        // button that runs b, not one that runs both -- a second connection
        // would leave the first handler in place and fire twice for one press.
        if (!raw->press_is_connected) {
            g_signal_connect(static_cast<GtkWidget *>(raw->widget), "clicked",
                             G_CALLBACK(it_was_pressed), raw);
            raw->press_is_connected = true;
        }
    });
    return true;
}

bool window_press(satellite_window &which, std::string &why)
{
    if (which.piece != satellite_window::button) {
        why = "only a button is pressed";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    // AND IT MUST BE SOMETHING A PERSON COULD HAVE CLICKED. A button that has
    // not been appended is on no screen, so pressing it is the program
    // pretending a person did something they could not have done. The two are
    // told apart because they are different mistakes: one forgot `.append`, the
    // other is holding a button whose window has gone.
    if (!which.on_the_screen) {
        why = "it is not in a window yet -- append it into one first";
        return false;
    }
    GtkWidget *widget = as_widget(which);
    // THE SAME SIGNAL A MOUSE RELEASE EMITS (gtkbutton.c:802), so a pressed
    // capsule cannot tell this apart from a person -- which is the whole point.
    // It arrives at it_was_pressed on the desk's thread, exactly as a click
    // does, and is QUEUED there rather than run: a program that presses its own
    // button does not recurse into the walker, it adds a press to the line.
    on_the_desk([widget] { g_signal_emit_by_name(widget, "clicked"); });
    return true;
}

bool window_close(satellite_window &which, std::string &why)
{
    if (which.piece != satellite_window::window) {
        why = "only a window can be closed";
        return false;
    }
    if (!still_there(which, why))
        return false;
    GtkWidget *widget = as_widget(which);
    // THE BOOKKEEPING IS THE `destroy` HANDLER'S, not this function's: a person
    // clicking the close button never reaches here, and one of the two ways a
    // window can go away must not be the one that is accounted for.
    on_the_desk([widget] { gtk_window_destroy(GTK_WINDOW(widget)); });
    return true;
}

bool window_focus(satellite_window &which, std::string &why)
{
    if (which.piece != satellite_window::window) {
        why = "only a window can be brought to the front";
        return false;
    }
    if (!still_there(which, why))
        return false;
    GtkWidget *widget = as_widget(which);
    // ASKED, NEVER TAKEN. A Wayland compositor decides whether a window comes to
    // the front, and refusing is a normal answer -- stealing focus is a thing a
    // desktop protects a person from on purpose.
    on_the_desk([widget] { gtk_window_present(GTK_WINDOW(widget)); });
    return true;
}

bool window_set_title(satellite_window &which, const std::string &title, std::string &why)
{
    if (which.piece != satellite_window::window) {
        why = "only a window has a title";
        return false;
    }
    if (!still_there(which, why))
        return false;
    GtkWidget *widget = as_widget(which);
    on_the_desk([widget, &title] { gtk_window_set_title(GTK_WINDOW(widget), title.c_str()); });
    which.title = title;
    return true;
}

void windows_stay_open_until_closed(bool the_program_finished)
{
    if (the_program_finished)
        close_the_desk_when_the_windows_are();
    else
        close_the_desk_now();
}

} // namespace satellite004
