// satellite/satellite_variable_window/satellite_window.cpp -- what a program can
// do to a window. SATELLITE_WINDOW.md WIN-3.
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

WindowHandle window_button(const std::string &text, std::string &why)
{
    if (!open_the_desk(why))
        return nullptr;
    WindowHandle made = std::make_shared<satellite_window>(satellite_window::button);
    made->text = text;
    satellite_window *raw = made.get();
    on_the_desk([raw, &text] { raw->widget = gtk_button_new_with_label(text.c_str()); });
    // NOT on_the_screen: a button is nothing until it is appended, and `.append`
    // is what puts it on one.
    return made;
}

bool window_append(satellite_window &into, const WindowHandle &piece, long long int x,
                   long long int y, std::string &why)
{
    if (into.piece != satellite_window::window) {
        why = "only a window can have something appended into it";
        return false;
    }
    if (!still_there(into, why))
        return false;
    if (piece == nullptr || piece->widget == nullptr) {
        why = "there is nothing here to append";
        return false;
    }
    if (piece->on_the_screen) {
        why = "that piece is already in a window";
        return false;
    }
    satellite_window *raw = piece.get();
    void *inside = into.inside;
    const int at_x = static_cast<int>(x), at_y = static_cast<int>(y);
    on_the_desk([raw, inside, at_x, at_y] {
        // BY ITS CENTRE, NOT ITS CORNER (WIN-3): 400, 300 is the middle of an
        // 800x600 window. GtkFixed places by the top-left, so the piece is
        // measured and half of each side is taken off -- which is the whole
        // difference between the author's spelling and GTK's.
        GtkWidget *widget = static_cast<GtkWidget *>(raw->widget);
        int least = 0, natural = 0, wide = 0, tall = 0;
        gtk_widget_measure(widget, GTK_ORIENTATION_HORIZONTAL, -1, &least, &natural, nullptr, nullptr);
        wide = natural;
        gtk_widget_measure(widget, GTK_ORIENTATION_VERTICAL, wide, &least, &natural, nullptr, nullptr);
        tall = natural;
        gtk_fixed_put(GTK_FIXED(static_cast<GtkWidget *>(inside)), widget,
                      static_cast<double>(at_x) - wide / 2.0, static_cast<double>(at_y) - tall / 2.0);
    });
    piece->on_the_screen = true;
    // HELD BY THE WINDOW, so that the window going away can null this piece's
    // GtkWidget * before GTK frees it underneath a handle the program still has.
    into.pieces.push_back(piece);
    // AND THE PIECE KNOWS WHICH WINDOW IT IS IN, which is how a pressed capsule
    // reaches the window: `.append` is the one place a piece ever enters one, so
    // it is the one place that can say so.
    piece->inside_of = into.weak_from_this();
    return true;
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
