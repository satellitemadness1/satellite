// The File menu. See menu.hpp for the split, and for why it takes no keys.

#include "programs/satl-term/menu.hpp"
#include "programs/satl-term/tabs.hpp"

#include <vte/vte.h>

#include <string>

namespace satellite {
namespace {

// The window every dialog is modal for, and the one this menu's actions act in.
// A static for the reason keys.cpp gives about its own: a second satl-term is a
// second process, not a second window in this one -- which is exactly what New
// window below goes and makes.
GtkWidget *the_window = nullptr;

// Something went wrong that the person asked for and cannot see. It goes in a
// dialog rather than onto the terminal, because the terminal is a transcript of
// what a PROGRAM did and this is the window talking about itself; feeding it
// there would put a sentence satl never printed into a screen somebody is about
// to save to a file.
void complain(const std::string &about)
{
    GtkAlertDialog *alert = gtk_alert_dialog_new("%s", about.c_str());
    gtk_alert_dialog_show(alert, GTK_WINDOW(the_window));
    g_object_unref(alert);
}

// A dialog that was dismissed is an ANSWER and not a failure -- somebody
// changed their mind -- and telling them so with a second dialog is the window
// arguing with them. Every other error is real and gets said.
bool was_only_dismissed(GError *error)
{
    return error && g_error_matches(error, GTK_DIALOG_ERROR,
                                    GTK_DIALOG_ERROR_DISMISSED);
}

// ---- Open ----------------------------------------------------------------

void on_file_chosen(GObject *source, GAsyncResult *answer, gpointer)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_open_finish(dialog, answer, &error);

    if (!file) {
        if (!was_only_dismissed(error))
            complain(std::string("that file could not be opened: ") +
                     (error ? error->message : "no reason given"));
        g_clear_error(&error);
        g_object_unref(dialog);
        return;
    }

    // A PATH ON THIS MACHINE OR NOTHING. The chooser can hand back a file on a
    // remote share that the desktop mounts through gvfs and that has no name in
    // the filesystem satl will be spawned into; g_file_get_path answers null
    // for exactly those. Handing satl a URI it cannot open would surface as the
    // interpreter failing to find a file the person had just picked from a list.
    char *path = g_file_get_path(file);
    if (path) {
        tabs_run(path, {});
        g_free(path);
    } else {
        complain("that file is not on this machine -- satl can only run a "
                 "program it can open by path. Copy it here first.");
    }

    g_object_unref(file);
    g_object_unref(dialog);
}

// The chooser starts in the directory `satl-term` was TYPED IN, which is the
// same directory the spawned satl will resolve a relative path against
// (window.cpp's NON_UNIQUE note is why a process still has that directory). A
// chooser that opens somewhere else makes the person navigate back to where
// they already were.
void on_open(GSimpleAction *, GVariant *, gpointer)
{
    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Run a satellite program");

    GtkFileFilter *programs = gtk_file_filter_new();
    gtk_file_filter_set_name(programs, "satellite programs");
    gtk_file_filter_add_pattern(programs, "*.satl");

    GtkFileFilter *everything = gtk_file_filter_new();
    gtk_file_filter_set_name(everything, "every file");
    gtk_file_filter_add_pattern(everything, "*");

    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, programs);
    g_list_store_append(filters, everything);
    gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
    gtk_file_dialog_set_default_filter(dialog, programs);

    GFile *here = g_file_new_for_path(".");
    gtk_file_dialog_set_initial_folder(dialog, here);

    gtk_file_dialog_open(dialog, GTK_WINDOW(the_window), nullptr,
                         on_file_chosen, nullptr);

    g_object_unref(here);
    g_object_unref(filters);
    g_object_unref(programs);
    g_object_unref(everything);
}

// ---- Save output as ------------------------------------------------------

void on_place_chosen(GObject *source, GAsyncResult *answer, gpointer for_terminal)
{
    GtkFileDialog *dialog = GTK_FILE_DIALOG(source);
    VteTerminal *terminal = VTE_TERMINAL(for_terminal);
    GError *error = nullptr;
    GFile *file = gtk_file_dialog_save_finish(dialog, answer, &error);

    if (file) {
        GFileOutputStream *stream =
            g_file_replace(file, nullptr, FALSE, G_FILE_CREATE_NONE, nullptr,
                           &error);
        if (stream) {
            // VTE_WRITE_DEFAULT is the text of the screen AND the scrollback,
            // which is the whole point: the interesting half of a failed run is
            // usually the part that has already scrolled off.
            vte_terminal_write_contents_sync(terminal, G_OUTPUT_STREAM(stream),
                                             VTE_WRITE_DEFAULT, nullptr, &error);
            g_output_stream_close(G_OUTPUT_STREAM(stream), nullptr, nullptr);
            g_object_unref(stream);
        }
        if (error)
            complain(std::string("that could not be written: ") +
                     error->message);
        g_object_unref(file);
    } else if (!was_only_dismissed(error)) {
        complain(std::string("that place could not be used: ") +
                 (error ? error->message : "no reason given"));
    }

    g_clear_error(&error);
    g_object_unref(terminal);
    g_object_unref(dialog);
}

// THE TERMINAL IS HELD FOR THE LIFE OF THE DIALOG. A file chooser is a
// conversation and the window keeps running underneath it -- a child can exit
// while it is open, and closing that tab would otherwise leave this callback
// writing out a terminal that had been finalised. The ref also makes the answer
// the right one: what gets saved is the screen that was in front when the
// person asked, not whichever tab happens to be there when they click Save.
void on_save_output(GSimpleAction *, GVariant *, gpointer)
{
    GtkWidget *terminal = tabs_terminal_in_front();
    if (!terminal)
        return;

    GtkFileDialog *dialog = gtk_file_dialog_new();
    gtk_file_dialog_set_title(dialog, "Save this terminal's output");
    gtk_file_dialog_set_initial_name(dialog, "satl-term.txt");

    GFile *here = g_file_new_for_path(".");
    gtk_file_dialog_set_initial_folder(dialog, here);

    gtk_file_dialog_save(dialog, GTK_WINDOW(the_window), nullptr,
                         on_place_chosen, g_object_ref(terminal));

    g_object_unref(here);
}

// ---- New tab, and New window ---------------------------------------------

void on_new_tab(GSimpleAction *, GVariant *, gpointer)
{
    tabs_open_tab("", {});
}

// A SECOND PROCESS AND NOT A SECOND WINDOW IN THIS ONE, which is the shape
// window.cpp already argues for where it registers the application
// G_APPLICATION_NON_UNIQUE, and which keys.cpp's statics are written against.
// It also means a window that crashes takes only its own work with it.
//
// /proc/self/exe rather than PATH, for child.cpp's reason one level up: the
// second window must be THIS satl-term, so that it spawns the same satl beside
// it -- not whichever build happens to be installed. The working directory is
// inherited, so the new window's chooser opens where this one's does.
void on_new_window(GSimpleAction *, GVariant *, gpointer)
{
    gchar *self = g_file_read_link("/proc/self/exe", nullptr);
    if (!self) {
        complain("satl-term cannot find itself on disk, so it cannot start a "
                 "second window.");
        return;
    }

    char *argv[] = { self, nullptr };
    GError *error = nullptr;
    if (!g_spawn_async(nullptr, argv, nullptr, G_SPAWN_DEFAULT,
                       nullptr, nullptr, nullptr, &error)) {
        complain(std::string("a second window could not be started: ") +
                 error->message);
        g_clear_error(&error);
    }

    g_free(self);
}

const GActionEntry ACTIONS[] = {
    { "new-tab",     on_new_tab,     nullptr, nullptr, nullptr, { 0, 0, 0 } },
    { "new-window",  on_new_window,  nullptr, nullptr, nullptr, { 0, 0, 0 } },
    { "open",        on_open,        nullptr, nullptr, nullptr, { 0, 0, 0 } },
    { "save-output", on_save_output, nullptr, nullptr, nullptr, { 0, 0, 0 } },
};

// THREE SECTIONS AND NOT ONE LIST, so the separators say what the grouping is:
// two ways to get somewhere to work, one way to put a program in front of you,
// one way to take what a program said out of the window. The ellipsis on two of
// them is the desktop's oldest promise -- this item asks before it acts.
GMenuModel *the_model()
{
    GMenu *file = g_menu_new();

    GMenu *somewhere_to_work = g_menu_new();
    g_menu_append(somewhere_to_work, "New tab", "win.new-tab");
    g_menu_append(somewhere_to_work, "New window", "win.new-window");
    g_menu_append_section(file, nullptr, G_MENU_MODEL(somewhere_to_work));
    g_object_unref(somewhere_to_work);

    GMenu *a_program = g_menu_new();
    g_menu_append(a_program, "Open…", "win.open");
    g_menu_append_section(file, nullptr, G_MENU_MODEL(a_program));
    g_object_unref(a_program);

    GMenu *what_it_said = g_menu_new();
    g_menu_append(what_it_said, "Save output as…", "win.save-output");
    g_menu_append_section(file, nullptr, G_MENU_MODEL(what_it_said));
    g_object_unref(what_it_said);

    GMenu *bar = g_menu_new();
    g_menu_append_submenu(bar, "File", G_MENU_MODEL(file));
    g_object_unref(file);

    return G_MENU_MODEL(bar);
}

} // namespace

GtkWidget *menu_bar_new(GtkWidget *window)
{
    the_window = window;

    g_action_map_add_action_entries(G_ACTION_MAP(window), ACTIONS,
                                    G_N_ELEMENTS(ACTIONS), nullptr);

    // A BAR IN THE WINDOW rather than gtk_application_window_set_show_menubar,
    // which hands the model to the desktop shell and lets it decide -- and on a
    // shell that draws no menubar at all, which is the usual one here, the menu
    // exists and is nowhere. The word File is on the screen because this line
    // put it there.
    GMenuModel *model = the_model();
    GtkWidget *bar = gtk_popover_menu_bar_new_from_model(model);
    g_object_unref(model);

    return bar;
}

} // namespace satellite
