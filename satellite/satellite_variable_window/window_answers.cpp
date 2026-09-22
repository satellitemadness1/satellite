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

#include <cmath>

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

// ONE OF A one_of WAS TICKED (GTK-3's radio). A pick fires `toggled` TWICE --
// once on the button going off and once on the one going on -- and only the
// one going ON is a change. The first draft queued both and leaned on the
// queue collapsing the pair; the interpreter can take the first off before
// the second is on, and then a capsule runs twice for one pick. Asking the
// button is one call and no race.
void one_was_ticked(GtkCheckButton *button, gpointer user_data)
{
    if (gtk_check_button_get_active(button))
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
    // A PERSON CLICKING A TAB CHANGES WHICH ONE IS IN FRONT (GTK-16). `page`
    // and not `switch-page`: the latter hands a handler the page and its
    // number, a shape neither function above has, and `notify::page` says the
    // same thing in the shape `notify::selected` already uses.
    case satellite_window::tabs:
        g_signal_connect(widget, "notify::page", G_CALLBACK(it_was_noticed), &which);
        return true;
    // A PERSON TICKING ONE OF A one_of CHANGES IT (GTK-3's radio). The box
    // itself has no signal; each check button in it has `toggled`, and
    // one_was_ticked above queues a change only for the button that went ON.
    case satellite_window::one_of:
        for (GtkWidget *button = gtk_widget_get_first_child(widget); button != nullptr;
             button = gtk_widget_get_next_sibling(button))
            g_signal_connect(button, "toggled", G_CALLBACK(one_was_ticked), &which);
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
           piece == satellite_window::choice || piece == satellite_window::tabs ||
           piece == satellite_window::one_of;
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

// THE CLOCK STRUCK (GTK-13). ON THE DESK'S THREAD, doing the one safe thing --
// writing the capsule's name on the queue -- exactly as a press does.
//
// COLLAPSED, and that is what "ticks do not queue up" means in practice: a tick
// arriving while the previous one is still waiting its turn is dropped rather
// than stacked, because the alternative is a program falling further behind for
// ever and looking like a leak rather than a loop.
gboolean it_ticked(gpointer user_data)
{
    satellite_window *window = static_cast<satellite_window *>(user_data);
    if (window->when_it_ticks.empty())
        return G_SOURCE_CONTINUE;
    the_desk_saw_something(window->when_it_ticks, window->shared_from_this(), true);
    return G_SOURCE_CONTINUE;
}

// WHAT A KEY IS CALLED, AND A PROGRAM NEVER SEES A KEYVAL (GTK-14).
//
// A PRINTABLE KEY IS ITS CHARACTER and everything else is a NAME in lower case.
// gdk_keyval_to_unicode() answers 0 for a key that draws nothing, which is
// exactly the test wanted; gdk_keyval_name() gives GDK's own spelling --
// "Escape", "Up", "Return", "F1" -- and lower case is satellite's.
//
// THE CHARACTER IS ENCODED AS UTF-8, because that is what a satellite string is.
// g_unichar_to_utf8 into six bytes is glib's own documented maximum.
std::string what_key_that_was(guint keyval)
{
    const gunichar drawn = gdk_keyval_to_unicode(keyval);
    if (drawn != 0 && g_unichar_isprint(drawn)) {
        char bytes[8] = {0};
        const gint many = g_unichar_to_utf8(drawn, bytes);
        return std::string(bytes, static_cast<std::size_t>(many));
    }
    const char *named = gdk_keyval_name(keyval);
    if (named == nullptr)
        return std::string();
    std::string out(named);
    for (char &c : out)
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    return out;
}

// A KEY WAS PRESSED. ON THE DESK'S THREAD, and what it SAID travels on the
// event rather than being written onto the piece here -- window_desk.hpp says
// why that is the difference between one writer and a race.
//
// NOT COLLAPSED. Two presses of the same key are two presses, the same way three
// clicks are three clicks; it is a slider's DRAG that is one change.
gboolean a_key_went_down(GtkEventControllerKey *, guint keyval, guint, GdkModifierType,
                         gpointer user_data)
{
    satellite_window *window = static_cast<satellite_window *>(user_data);
    if (!window->when_a_key.empty())
        the_desk_saw_something(window->when_a_key, window->shared_from_this(), false,
                               what_key_that_was(keyval), AnEvent::a_key);
    // FALSE, SO THE KEY GOES ON TO THE WIDGET THAT WANTED IT. Answering TRUE
    // would mean a program that watches for Escape has silently made every text
    // box in the window unusable.
    return FALSE;
}

// AND A CLICK ON SOMETHING THAT IS NOT A BUTTON. WHERE IT LANDED TRAVELS ON THE
// EVENT (GTK-15's leftover), as a key's name does: GTK hands the point in the
// widget's own pixels, which on a canvas are the pixels `.line` draws in, and
// the interpreter copies it onto the piece for `.across` and `.down` to read.
// Rounded down to a whole pixel, because a satellite number is whole.
void it_was_clicked(GtkGestureClick *, gint, gdouble x, gdouble y, gpointer user_data)
{
    satellite_window *piece = static_cast<satellite_window *>(user_data);
    if (!piece->when_clicked.empty())
        the_desk_saw_something(piece->when_clicked, piece->shared_from_this(), false, std::string(),
                               AnEvent::a_place, static_cast<long long int>(std::floor(x)),
                               static_cast<long long int>(std::floor(y)));
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
    if (!which.is_a_window()) {
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

bool window_every(satellite_window &which, const std::string &capsule, long long int milliseconds,
                  std::string &why)
{
    if (!which.is_a_window()) {
        why = "only a window keeps time -- a piece inside one lives as long as the window does";
        return false;
    }
    // A TICK OF 0 IS NOT A RHYTHM, it is a busy loop with a capsule in it: glib
    // would run it as fast as the main loop turns, the queue would fill faster
    // than the interpreter could drain it, and the program would look like a
    // leak. Refused where it is written.
    if (milliseconds <= 0) {
        why = "how often must be more than 0 milliseconds";
        return false;
    }
    if (milliseconds > 86400000) {
        why = "how often must be a day (86400000 milliseconds) or less";
        return false;
    }
    if (which.widget == nullptr || !which.on_the_screen) {
        why = "it is closed";
        return false;
    }
    satellite_window *raw = &which;
    const unsigned int how_often = static_cast<unsigned int>(milliseconds);
    on_the_desk([raw, &capsule, how_often] {
        raw->when_it_ticks = capsule;
        // A SECOND `.every` REPLACES THE FIRST. Leaving the old source running
        // would give one window two clocks with no way to tell them apart and no
        // way to stop either.
        if (raw->tick != 0)
            g_source_remove(raw->tick);
        // ON THE DESK'S OWN CONTEXT, which is what g_timeout_add attaches to
        // when it is called from the thread that owns it -- the desk parks in
        // g_main_loop_run there, so the source fires on the desk and nowhere
        // else.
        raw->tick = g_timeout_add(how_often, it_ticked, raw);
    });
    return true;
}

bool window_key(satellite_window &which, const std::string &capsule, std::string &why)
{
    if (!which.is_a_window()) {
        why = "only a window hears the keyboard -- a key goes to whatever has the focus, and the "
              "window is what sees them all";
        return false;
    }
    if (which.widget == nullptr || !which.on_the_screen) {
        why = "it is closed";
        return false;
    }
    satellite_window *raw = &which;
    on_the_desk([raw, &capsule] {
        raw->when_a_key = capsule;
        // THE CONTROLLER IS ADDED ONCE. GTK4 has no key signal on a widget --
        // everything is a controller you add -- and adding a second would run
        // the capsule twice for one key.
        if (raw->key_is_connected)
            return;
        GtkEventController *hears = gtk_event_controller_key_new();
        g_signal_connect(hears, "key-pressed", G_CALLBACK(a_key_went_down), raw);
        gtk_widget_add_controller(static_cast<GtkWidget *>(raw->widget), hears);
        raw->key_is_connected = true;
    });
    return true;
}

bool window_clicked(satellite_window &which, const std::string &capsule, std::string &why)
{
    if (which.piece == satellite_window::button) {
        why = "a button already has a word for being clicked -- write .pressed(" + capsule +
              ") instead";
        return false;
    }
    // A MENU IS NOT CLICKED, ITS ITEMS ARE PICKED (GTK-12) -- and it is not a
    // widget a gesture could be added to.
    if (!which.is_drawn()) {
        why = "a menu is not clicked, its items are picked -- write .item(" + capsule +
              ", \"Open\") to say what runs";
        return false;
    }
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    satellite_window *raw = &which;
    on_the_desk([raw, &capsule] {
        raw->when_clicked = capsule;
        if (raw->click_is_connected)
            return;
        GtkGesture *notices = gtk_gesture_click_new();
        g_signal_connect(notices, "released", G_CALLBACK(it_was_clicked), raw);
        gtk_widget_add_controller(static_cast<GtkWidget *>(raw->widget),
                                  GTK_EVENT_CONTROLLER(notices));
        raw->click_is_connected = true;
    });
    return true;
}

} // namespace satellite004
