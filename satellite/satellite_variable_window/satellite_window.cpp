// satellite/satellite_variable_window/satellite_window.cpp -- what a program can
// do to a WINDOW. SATELLITE_WINDOW.md WIN-3.
//
// FIVE FILES NOW, and this one is the WINDOW. It makes one, closes it, focuses
// it, titles it, appends into it and holds the run open:
//
//   window_pieces.cpp    MAKING a piece -- four factories, one an argument shape
//   window_asks.cpp      a piece's WORDS: .text and .path
//   window_state.cpp     what a piece is SET TO: .on, .value, .chosen
//   window_answers.cpp   what a piece SAYS BACK: .pressed, .press, .changed, .closed
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
    satellite_window *window = static_cast<satellite_window *>(user_data);
    // THE CAPSULE IS QUEUED BEFORE THE DESK LETS GO, and the order matters: the
    // queue carries a HANDLE to the window, and the_desk_let_go_of drops the
    // desk's own reference. Queueing second would still work -- the program's
    // handle keeps it alive -- but only when the program still holds one, and a
    // window opened and forgotten is an ordinary program.
    if (!window->when_closed.empty())
        the_desk_saw_something(window->when_closed, window->shared_from_this());
    the_desk_let_go_of(window);
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
    made->asked_wide = static_cast<int>(width);
    made->asked_tall = static_cast<int>(height);
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
    // AND SOME OF THEM HOLD A FIXED NUMBER (GTK-16). Refused rather than
    // ignored: gtk_scrolled_window_set_child on a scroll that already has one
    // silently DROPS the first -- the piece is still a piece, the program still
    // holds it, and it is simply not on the screen any more and nothing said so.
    const unsigned int room = into.holds_how_many();
    if (room != 0 && into.pieces.size() >= room) {
        why = std::string(into.piece_name()) + " holds " + (room == 1 ? "one piece" : "two pieces") +
              ", and it already has " + (into.pieces.size() == 1 ? "one" : "two");
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
        if (holder == satellite_window::scroll) {
            gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(into_this), widget);
            return;
        }
        if (holder == satellite_window::frame) {
            gtk_frame_set_child(GTK_FRAME(into_this), widget);
            return;
        }
        if (holder == satellite_window::split) {
            // FIRST ONE ON THE LEFT, SECOND ON THE RIGHT, which is what "one
            // after another" already means everywhere else here -- a split is a
            // row of exactly two with a handle between them.
            if (gtk_paned_get_start_child(GTK_PANED(into_this)) == nullptr)
                gtk_paned_set_start_child(GTK_PANED(into_this), widget);
            else
                gtk_paned_set_end_child(GTK_PANED(into_this), widget);
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

bool window_resize(satellite_window &which, long long int wide, long long int tall, std::string &why)
{
    if (which.piece != satellite_window::window) {
        why = "only a window is given a size -- a piece inside one is sized by what holds it";
        return false;
    }
    if (wide <= 0 || tall <= 0) {
        why = "a window's width and height must both be more than 0";
        return false;
    }
    if (wide > 32767 || tall > 32767) {
        why = "a window's width and height must each be 32767 or less";
        return false;
    }
    if (!still_there(which, why))
        return false;
    GtkWidget *widget = as_widget(which);
    const int to_wide = static_cast<int>(wide), to_tall = static_cast<int>(tall);
    on_the_desk([widget, to_wide, to_tall] {
        gtk_window_set_default_size(GTK_WINDOW(widget), to_wide, to_tall);
    });
    which.asked_wide = to_wide;
    which.asked_tall = to_tall;
    return true;
}

bool window_size_of(satellite_window &which, bool the_height, long long int &out, std::string &why)
{
    if (which.widget == nullptr) {
        why = "it is closed -- ask its size while the window is still open";
        return false;
    }
    GtkWidget *widget = as_widget(which);
    int got = 0, natural = 0;
    const bool a_window = which.piece == satellite_window::window;
    on_the_desk([widget, the_height, &got, &natural] {
        got = the_height ? gtk_widget_get_height(widget) : gtk_widget_get_width(widget);
        if (got > 0)
            return;
        // NOTHING ON A SCREEN YET, so ask what it WOULD measure. This is the
        // same question `.append` asks to centre a piece, which is why a button
        // can answer it before it is in a window at all.
        int least = 0;
        gtk_widget_measure(widget, the_height ? GTK_ORIENTATION_VERTICAL : GTK_ORIENTATION_HORIZONTAL,
                           -1, &least, &natural, nullptr, nullptr);
    });
    if (got > 0)
        out = got;
    else if (a_window)
        out = the_height ? which.asked_tall : which.asked_wide;
    else
        out = natural;
    return true;
}

bool window_fullscreen_of(satellite_window &which, bool &out, std::string &why)
{
    if (which.piece != satellite_window::window) {
        why = "only a window fills the screen";
        return false;
    }
    if (!still_there(which, why))
        return false;
    GtkWidget *widget = as_widget(which);
    bool got = false;
    on_the_desk([widget, &got] { got = gtk_window_is_fullscreen(GTK_WINDOW(widget)) != FALSE; });
    out = got;
    return true;
}

bool window_set_fullscreen(satellite_window &which, bool on, std::string &why)
{
    if (which.piece != satellite_window::window) {
        why = "only a window fills the screen";
        return false;
    }
    if (!still_there(which, why))
        return false;
    GtkWidget *widget = as_widget(which);
    // ASKED, NEVER TAKEN. A compositor decides, and it may refuse -- which is a
    // normal answer and not a failure, so this reports true either way. A
    // program that needs to know asks `.fullscreen` when it matters.
    on_the_desk([widget, on] {
        if (on)
            gtk_window_fullscreen(GTK_WINDOW(widget));
        else
            gtk_window_unfullscreen(GTK_WINDOW(widget));
    });
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
