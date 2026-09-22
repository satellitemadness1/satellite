// satellite/satellite_variable_window/window_answers.cpp -- WHAT A PIECE SAYS
// BACK. SATELLITE_WINDOW.md WIN-11 and GTK_AND_NO_DEPENDENCIES.md GTK-9.
//
// SPLIT OUT OF satellite_window.cpp AT GTK-9, at 463 lines against the author's
// "try to build for 300 lines". GTK-0 named this file before it existed.
//
// EVERY HANDLER IN HERE RUNS ON THE DESK'S THREAD AND WALKS NOTHING. It writes
// the capsule's NAME on the desk's queue and wakes whoever is waiting; the
// INTERPRETER's thread takes it off and runs it. window_desk.hpp carries the
// ruling and why it is the only safe thread for a capsule -- the walker is no
// more thread-safe than GTK is.
//
// THE `destroy` HANDLER IS NOT HERE, on purpose. It is next door beside
// window_new(), which is what connects it, and it is the desk's own bookkeeping
// before it is anybody's capsule -- `.closed` only writes the name it reads.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

namespace satellite004 {
namespace {

// A PERSON CHANGED SOMETHING (GTK-9). ON THE DESK'S THREAD, doing the one thing
// that is safe here -- writing the capsule's name on the desk's queue.
//
// TWO SHAPES BECAUSE GTK HAS TWO. `changed`, `toggled` and `value-changed` hand
// a handler the widget; `notify::active` and `notify::selected` hand it the
// object and the GParamSpec that changed. Both end in the same line.
void it_was_changed(GtkWidget *, gpointer user_data)
{
    satellite_window *piece = static_cast<satellite_window *>(user_data);
    // COLLAPSED, unlike a press -- window_desk.hpp says why a drag is one change
    // and three clicks are three presses.
    if (!piece->when_changed.empty())
        the_desk_saw_something(piece->when_changed, piece->shared_from_this(), true);
}

void it_was_noticed(GObject *, GParamSpec *, gpointer user_data)
{
    it_was_changed(nullptr, user_data);
}

// WHICH SIGNAL MEANS "A PERSON CHANGED THIS", A PIECE AT A TIME. False for a
// piece nothing a person does can change -- a label, a picture, a row, a
// progress bar -- and for a BUTTON, which is pressed rather than changed.
//
// ON THE DESK'S THREAD: a text area's signal is on its BUFFER and not on the
// widget, which is the one case that needs a GTK call to find the object at all.
bool connect_what_changing_means(satellite_window &which, GtkWidget *widget)
{
    switch (which.piece) {
    case satellite_window::text_box:
        g_signal_connect(widget, "changed", G_CALLBACK(it_was_changed), &which);
        return true;
    case satellite_window::text_area:
        g_signal_connect(gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget)), "changed",
                         G_CALLBACK(it_was_changed), &which);
        return true;
    case satellite_window::checkbox:
        g_signal_connect(widget, "toggled", G_CALLBACK(it_was_changed), &which);
        return true;
    case satellite_window::a_switch:
        g_signal_connect(widget, "notify::active", G_CALLBACK(it_was_noticed), &which);
        return true;
    case satellite_window::slider:
    case satellite_window::number_box:
        g_signal_connect(widget, "value-changed", G_CALLBACK(it_was_changed), &which);
        return true;
    case satellite_window::choice:
        g_signal_connect(widget, "notify::selected", G_CALLBACK(it_was_noticed), &which);
        return true;
    default:
        return false;
    }
}

// AND WHETHER THERE IS SUCH A SIGNAL AT ALL, asked WITHOUT connecting one --
// because the refusal happens on the interpreter's thread and connecting does
// not. Kept beside the switch above so the two cannot answer differently.
bool a_person_can_change(satellite_window::Piece piece)
{
    return piece == satellite_window::text_box || piece == satellite_window::text_area ||
           piece == satellite_window::checkbox || piece == satellite_window::a_switch ||
           piece == satellite_window::slider || piece == satellite_window::number_box ||
           piece == satellite_window::choice;
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
        the_desk_saw_something(button->when_pressed, button->shared_from_this());
}

} // namespace

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

bool window_changed(satellite_window &which, const std::string &capsule, std::string &why)
{
    if (which.piece == satellite_window::button) {
        why = "a button is not changed, it is pressed -- write .pressed(" + capsule + ") instead";
        return false;
    }
    if (!a_person_can_change(which.piece)) {
        why = "nothing a person does changes " + std::string(which.piece_name()) +
              ", so a capsule here would never run";
        return false;
    }
    // NOT still_there(): a piece is on no screen until it is appended, and
    // saying what changing it does BEFORE putting it in a window is the ordinary
    // order to write it in -- the same rule window_pressed() follows.
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    satellite_window *raw = &which;
    on_the_desk([raw, &capsule] {
        raw->when_changed = capsule;
        // CONNECTED ONCE, AND NOT ONCE A CALL. `b.changed(a).changed(b)` is a
        // piece that runs b, not one that runs both -- a second connection would
        // leave the first handler in place and fire twice for one change.
        if (!raw->changed_is_connected)
            raw->changed_is_connected = connect_what_changing_means(*raw, static_cast<GtkWidget *>(raw->widget));
    });
    return true;
}

bool window_closed(satellite_window &which, const std::string &capsule, std::string &why)
{
    if (which.piece != satellite_window::window) {
        why = "only a window is closed";
        return false;
    }
    // A WINDOW THAT IS ALREADY GONE, said the way the rest of this module says
    // it. Unlike `.pressed` and `.changed`, this one DOES want the window on a
    // screen: a capsule for the closing of a window that has closed would never
    // run, and saying so is the whole point of this refusal.
    if (which.widget == nullptr || !which.on_the_screen) {
        why = "it is closed";
        return false;
    }
    // NO SIGNAL IS CONNECTED HERE. `destroy` was connected when the window was
    // made -- it is what the desk's own bookkeeping hangs on -- so this writes
    // the name that handler already reads. ON THE DESK, because that handler
    // reads it there and this is the only writer.
    satellite_window *raw = &which;
    on_the_desk([raw, &capsule] { raw->when_closed = capsule; });
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
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    // THE SAME SIGNAL A MOUSE RELEASE EMITS (gtkbutton.c:802), so a pressed
    // capsule cannot tell this apart from a person -- which is the whole point.
    // It arrives at it_was_pressed on the desk's thread, exactly as a click
    // does, and is QUEUED there rather than run: a program that presses its own
    // button does not recurse into the walker, it adds a press to the line.
    on_the_desk([widget] { g_signal_emit_by_name(widget, "clicked"); });
    return true;
}

} // namespace satellite004
