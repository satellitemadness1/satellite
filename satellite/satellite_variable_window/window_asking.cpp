// satellite/satellite_variable_window/window_asking.cpp -- SAYING SOMETHING TO
// A PERSON, AND ASKING THEM. GTK_AND_NO_DEPENDENCIES.md GTK-11.
//
// A QUESTION IS A CAPSULE AND NOT A WAIT. GtkAlertDialog is asynchronous: the
// answer arrives in a GAsyncReadyCallback on the desk's thread, which is the
// press queue again with a different producer. GTK-11 asked whether a satellite
// line may instead STOP until a person answers -- `satellite.variable.string a =
// my_window.ask("...")` -- and that stays the author's. Nothing here forecloses
// it; this is the shape a press already taught.
//
// THE FILE DIALOG IS HERE SINCE 2026-09-22, AND IT WAS NOT BEFORE, ON PURPOSE.
// GtkFileDialog goes out to xdg-desktop-portal whenever one is on the bus, and
// a wedged portal was Q-WIN-11a -- the synchronous D-Bus call that hangs satl
// for ever with nothing printed. The author ruled on 2026-09-22, in one word:
// "defend". window_desk.cpp turns portals off before the display is opened, so
// what opens here is GTK's own GtkFileChooserDialog in satl's own process, and
// the path comes back the way a question's answer does.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <gtk/gtk.h>

#include <string>

namespace satellite004 {
namespace {

// A PERSON ANSWERED. ON THE DESK'S THREAD, in GTK's own async callback.
//
// WHAT THEY SAID TRAVELS ON THE EVENT, never written onto the piece here --
// window_desk.hpp says why that is the difference between one writer and a race.
void they_answered(GObject *source, GAsyncResult *result, gpointer user_data)
{
    satellite_window *window = static_cast<satellite_window *>(user_data);
    GError *went_wrong = nullptr;
    const int which = gtk_alert_dialog_choose_finish(GTK_ALERT_DIALOG(source), result, &went_wrong);
    // DISMISSED IS NOT A FAILURE. Closing a question without choosing is a
    // thing a person is entitled to do, and GTK reports it as an error -- so it
    // becomes "" rather than a refusal of the program, which did nothing wrong.
    std::string said;
    if (went_wrong != nullptr) {
        g_error_free(went_wrong);
    } else {
        said = which == 0 ? "yes" : "no";
    }
    if (!window->when_answered.empty())
        the_desk_saw_something(window->when_answered, window->shared_from_this(), false, said,
                               AnEvent::an_answer);
    g_object_unref(source);
}

// A PERSON CHOSE A FILE, OR CLOSED THE DIALOG. ON THE DESK'S THREAD, in GTK's
// own async callback, and the path travels on the event as a question's answer
// does -- it IS an answer, and `.answer` is how a program reads it.
//
// DISMISSED IS "" AND NOT A REFUSAL, for they_answered's reason. A file with
// no path -- a GFile that is not on this machine's disk -- is "" too, and
// cannot happen through GTK's own chooser; it is said here so that a portal
// chooser one day does not hand a program a URI it cannot open.
void they_chose(GObject *source, GAsyncResult *result, gpointer user_data)
{
    satellite_window *window = static_cast<satellite_window *>(user_data);
    GError *went_wrong = nullptr;
    GFile *file = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &went_wrong);
    std::string said;
    if (file == nullptr) {
        if (went_wrong != nullptr)
            g_error_free(went_wrong);
    } else {
        char *path = g_file_get_path(file);
        if (path != nullptr) {
            said = path;
            g_free(path);
        }
        g_object_unref(file);
    }
    if (!window->when_a_file_is_chosen.empty())
        the_desk_saw_something(window->when_a_file_is_chosen, window->shared_from_this(), false, said,
                               AnEvent::an_answer);
    g_object_unref(source);
}

bool a_window_that_can_be_asked(satellite_window &which, std::string &why)
{
    if (!which.is_a_window()) {
        why = "only a window says things to a person -- a question needs a window to sit over";
        return false;
    }
    if (which.widget == nullptr || !which.on_the_screen) {
        why = "it is closed";
        return false;
    }
    return true;
}

} // namespace

bool window_message(satellite_window &which, const std::string &saying, std::string &why)
{
    if (!a_window_that_can_be_asked(which, why))
        return false;
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    // "%s" AND NOT THE TEXT ITSELF. gtk_alert_dialog_new takes a PRINTF FORMAT,
    // so a person's own text containing a % would be read as a conversion and
    // GTK would walk off the end of an argument list that has nothing in it.
    // That is a crash a program could cause by displaying a percentage.
    on_the_desk([widget, &saying] {
        GtkAlertDialog *says = gtk_alert_dialog_new("%s", saying.c_str());
        gtk_alert_dialog_show(says, GTK_WINDOW(widget));
        g_object_unref(says);
    });
    return true;
}

bool window_ask(satellite_window &which, const std::string &question, const std::string &capsule,
                std::string &why)
{
    if (!a_window_that_can_be_asked(which, why))
        return false;
    satellite_window *raw = &which;
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    on_the_desk([raw, widget, &question, &capsule] {
        raw->when_answered = capsule;
        GtkAlertDialog *asks = gtk_alert_dialog_new("%s", question.c_str());
        // YES FIRST, so that button 0 is yes -- which is what they_answered
        // reads. A list that ended NULL-terminated the other way round would
        // answer "no" to every yes and nothing would say so.
        const char *buttons[] = {"Yes", "No", nullptr};
        gtk_alert_dialog_set_buttons(asks, buttons);
        gtk_alert_dialog_set_cancel_button(asks, 1);
        gtk_alert_dialog_set_default_button(asks, 0);
        // THE DIALOG IS NOT UNREF'ED HERE. gtk_alert_dialog_choose is
        // asynchronous and they_answered is handed the dialog as its source;
        // dropping the last reference now would free it before the person has
        // answered. The callback is what unrefs it, once.
        gtk_alert_dialog_choose(asks, GTK_WINDOW(widget), nullptr, they_answered, raw);
    });
    return true;
}

bool window_choose_a_file(satellite_window &which, const std::string &capsule, std::string &why)
{
    if (!a_window_that_can_be_asked(which, why))
        return false;
    satellite_window *raw = &which;
    GtkWidget *widget = static_cast<GtkWidget *>(which.widget);
    on_the_desk([raw, widget, &capsule] {
        raw->when_a_file_is_chosen = capsule;
        GtkFileDialog *chooses = gtk_file_dialog_new();
        // MODAL OVER ITS WINDOW, as a question is, and NOT UNREF'ED HERE for
        // window_ask's reason: gtk_file_dialog_open is asynchronous and
        // they_chose is handed the dialog as its source, which is what unrefs
        // it, once.
        gtk_file_dialog_set_modal(chooses, TRUE);
        gtk_file_dialog_open(chooses, GTK_WINDOW(widget), nullptr, they_chose, raw);
    });
    return true;
}

} // namespace satellite004
