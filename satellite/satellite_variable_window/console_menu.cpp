// satellite/satellite_variable_window/console_menu.cpp -- THE FILE MENU ACROSS
// satl'S OWN CONSOLE: New window, Open…, Save output as…. GTK-17, satl-term's
// menu ported the day satl-term was removed (the author, 2026-09-22: "we are
// getting rid of satl-term and replacing it with something built in to the satl
// exe").
//
// SPLIT FROM console_launch.cpp BY SUBJECT: that file moves satl onto its own
// console and decides what the keyboard means; this one is dialogs, and starting
// another satl. EVERY CALL HERE RUNS ON THE DESK -- the actions fire there, the
// dialogs answer there -- and none of it writes to satl's own pty.
//
// COMPILED WHERE pkg-config FINDS gtk4, and only its VTE half has anything in
// it: a satl without VTE has no console of its own to put a menu on.

#include "satellite_window.hpp"

#if SATELLITE_HAS_CONSOLE

#include "window_console.hpp"
#include "../machine/stack_share.hpp"

#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>

namespace satellite004 {
namespace {

// THE FILE MENU, satl-term'S PORTED (satl-term/menu.cpp, removed 2026-09-22). New window, Open…,
// Save output as…, and NO FIFTH: the close button is the desktop's, and a menu
// item repeating it is a second spelling of something already spelled.
//
// MOUSE ONLY, and that is satl-term's rule too: no Ctrl-O, no Ctrl-S -- a menu
// shortcut is a key taken from every program that will ever run on the other
// side of the pty. AND NOT F10: GTK gives a menu bar F10 by default
// (`handle-menubar-accel`), and that is exactly such a key; it is turned off,
// so F10 reaches the program.
//
// NEW TAB IS NOT PORTED, AND IT CANNOT BE AS IT WAS. satl-term's tabs were one
// window holding several satl PROCESSES; satl here IS the process, and the
// walker runs one program at a time. New window is the same act, one window a
// run -- and it is what satl-term's own New window already did.
//
// OPEN… RUNS THE PROGRAM IN A NEW WINDOW, and that is the one place this departs
// from satl-term, which ran it in the tab in front when that tab's program had
// finished. The window in front here is satl itself, still running what it was
// started with -- usually the prompt, which runs until the person leaves -- so
// "in this window, and never at the cost of a run" can only be kept as "in a
// window of its own".
// ---------------------------------------------------------------------------

// WHAT GOES WRONG IN A DIALOG IS SAID IN A DIALOG, never fed to the terminal:
// the terminal is a transcript of what a PROGRAM did, and a sentence satl never
// printed does not belong in the file somebody is about to save. ON THE DESK.
void say_it_in_a_box(satellite_window *console, const std::string &saying)
{
    GtkAlertDialog *box = gtk_alert_dialog_new("%s", saying.c_str());
    GtkWindow *over = console != nullptr && console->widget != nullptr
                          ? GTK_WINDOW(static_cast<GtkWidget *>(console->widget))
                          : nullptr;
    gtk_alert_dialog_show(box, over);
    g_object_unref(box);
}

// A NEW satl, in a console of its own, running `file` -- or the prompt when
// there is none. ON THE DESK.
//
// /proc/self/exe AND NOT PATH, so the new window is THIS satl with THESE
// libraries beside it. Its stdin is /dev/null and its stdout and stderr are
// where THIS satl was started (the C stderr, which keep_the_c_streams_off_the_pty
// left there) -- never this console's pty, or the new satl's first words would
// land in this window. setsid() in the child, so this console's hangup is never
// the new one's; GLib's double fork reaps it, so no zombie waits on this one.
void start_another_satl(satellite_window *console, const char *file)
{
    char self[4096];
    const ssize_t length = readlink("/proc/self/exe", self, sizeof self - 1);
    if (length <= 0) {
        say_it_in_a_box(console, "satl cannot read its own path (/proc/self/exe), so it cannot start another");
        return;
    }
    self[length] = '\0';
    char console_flag[] = "--console";
    std::vector<char *> words{self, console_flag};
    std::string path = file != nullptr ? std::string(file) : std::string();
    if (file != nullptr)
        words.push_back(path.data());
    words.push_back(nullptr);
    GError *trouble = nullptr;
    const int where_satl_started = fileno(stderr);
    if (!g_spawn_async_with_fds(nullptr, words.data(), nullptr, G_SPAWN_STDIN_FROM_DEV_NULL,
                                [](gpointer) { setsid(); hand_a_child_the_stack_satl_was_given(); }, nullptr, nullptr, -1, where_satl_started,
                                where_satl_started, &trouble)) {
        say_it_in_a_box(console, std::string("could not start another satl -- ") +
                                     (trouble != nullptr ? trouble->message : "no reason given"));
        if (trouble != nullptr)
            g_error_free(trouble);
    }
}

void when_a_new_window(GSimpleAction *, GVariant *, gpointer user_data)
{
    start_another_satl(static_cast<satellite_window *>(user_data), nullptr);
}

// A DISMISSED CHOOSER IS AN ANSWER, NOT A FAILURE, and gets no box.
bool it_was_only_dismissed(const GError *trouble)
{
    return trouble != nullptr && g_error_matches(trouble, GTK_DIALOG_ERROR, GTK_DIALOG_ERROR_DISMISSED);
}

void when_a_program_was_chosen(GObject *source, GAsyncResult *result, gpointer user_data)
{
    satellite_window *console = static_cast<satellite_window *>(user_data);
    GError *trouble = nullptr;
    GFile *chosen = gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source), result, &trouble);
    if (chosen != nullptr) {
        // A FILE WITH NO PATH ON THIS MACHINE -- a gvfs share -- is refused with
        // a sentence: satl handed a URI would fail to find a file the person had
        // just picked from a list.
        char *path = g_file_get_path(chosen);
        if (path != nullptr)
            start_another_satl(console, path);
        else
            say_it_in_a_box(console, "that file has no path on this machine, so satl cannot open it");
        g_free(path);
        g_object_unref(chosen);
    } else if (!it_was_only_dismissed(trouble)) {
        say_it_in_a_box(console, std::string("could not open it -- ") +
                                     (trouble != nullptr ? trouble->message : "no reason given"));
    }
    if (trouble != nullptr)
        g_error_free(trouble);
    g_object_unref(source);
}

void when_open(GSimpleAction *, GVariant *, gpointer user_data)
{
    satellite_window *console = static_cast<satellite_window *>(user_data);
    if (console->widget == nullptr)
        return;
    GtkFileDialog *chooses = gtk_file_dialog_new();
    gtk_file_dialog_set_title(chooses, "Run a satellite program");
    GtkFileFilter *programs = gtk_file_filter_new();
    gtk_file_filter_set_name(programs, "satellite programs");
    gtk_file_filter_add_pattern(programs, "*.satl");
    GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);
    g_list_store_append(filters, programs);
    gtk_file_dialog_set_filters(chooses, G_LIST_MODEL(filters));
    g_object_unref(filters);
    g_object_unref(programs);
    // WHERE satl WAS STARTED, satl-term's own choice: the folder a person was in.
    char *here = g_get_current_dir();
    GFile *folder = g_file_new_for_path(here);
    gtk_file_dialog_set_initial_folder(chooses, folder);
    g_object_unref(folder);
    g_free(here);
    gtk_file_dialog_open(chooses, GTK_WINDOW(static_cast<GtkWidget *>(console->widget)), nullptr,
                         when_a_program_was_chosen, console);
}

void when_a_place_to_save_was_chosen(GObject *source, GAsyncResult *result, gpointer user_data)
{
    satellite_window *console = static_cast<satellite_window *>(user_data);
    GError *trouble = nullptr;
    GFile *chosen = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(source), result, &trouble);
    if (chosen != nullptr && console->terminal != nullptr) {
        // THE SCREEN AND THE SCROLLBACK, satl-term's reason: the interesting half
        // of a failed run is usually the part that has already scrolled off.
        GFileOutputStream *out = g_file_replace(chosen, nullptr, FALSE, G_FILE_CREATE_NONE, nullptr, &trouble);
        if (out != nullptr) {
            if (vte_terminal_write_contents_sync(terminal_of(*console), G_OUTPUT_STREAM(out), VTE_WRITE_DEFAULT,
                                                 nullptr, &trouble) != FALSE)
                g_output_stream_close(G_OUTPUT_STREAM(out), nullptr, trouble == nullptr ? &trouble : nullptr);
            g_object_unref(out);
        }
        if (trouble != nullptr)
            say_it_in_a_box(console, std::string("could not save the output -- ") + trouble->message);
    } else if (!it_was_only_dismissed(trouble)) {
        say_it_in_a_box(console, std::string("could not save the output -- ") +
                                     (trouble != nullptr ? trouble->message : "the console has closed"));
    }
    if (chosen != nullptr)
        g_object_unref(chosen);
    if (trouble != nullptr)
        g_error_free(trouble);
    g_object_unref(source);
}

void when_save(GSimpleAction *, GVariant *, gpointer user_data)
{
    satellite_window *console = static_cast<satellite_window *>(user_data);
    if (console->widget == nullptr)
        return;
    GtkFileDialog *chooses = gtk_file_dialog_new();
    gtk_file_dialog_set_title(chooses, "Save what the console holds");
    gtk_file_dialog_set_initial_name(chooses, "satellite-output.txt");
    gtk_file_dialog_save(chooses, GTK_WINDOW(static_cast<GtkWidget *>(console->widget)), nullptr,
                         when_a_place_to_save_was_chosen, console);
}

} // namespace

// THE BAR, ACROSS THE TOP OF satl'S OWN CONSOLE. ON THE DESK. Its actions are
// "satl.new-window" and the rest, on the window: no GtkApplication, for the
// reason satellite_window.cpp gives -- it would register on a session bus.
void give_it_a_file_menu(satellite_window &console)
{
    GtkWidget *window = static_cast<GtkWidget *>(console.widget);
    gtk_window_set_handle_menubar_accel(GTK_WINDOW(window), FALSE);
    GSimpleActionGroup *actions = g_simple_action_group_new();
    const struct {
        const char *name;
        void (*does)(GSimpleAction *, GVariant *, gpointer);
    } items[] = {{"new-window", when_a_new_window}, {"open", when_open}, {"save", when_save}};
    for (const auto &item : items) {
        GSimpleAction *action = g_simple_action_new(item.name, nullptr);
        g_signal_connect(action, "activate", G_CALLBACK(item.does), &console);
        g_action_map_add_action(G_ACTION_MAP(actions), G_ACTION(action));
        g_object_unref(action);
    }
    gtk_widget_insert_action_group(window, "satl", G_ACTION_GROUP(actions));
    g_object_unref(actions);

    GMenu *file = g_menu_new();
    const struct {
        const char *label;
        const char *action;
    } sections[] = {{"New window", "satl.new-window"}, {"Open…", "satl.open"}, {"Save output as…", "satl.save"}};
    for (const auto &section : sections) {
        GMenu *one = g_menu_new();
        g_menu_append(one, section.label, section.action);
        g_menu_append_section(file, nullptr, G_MENU_MODEL(one));
        g_object_unref(one);
    }
    GMenu *bar = g_menu_new();
    g_menu_append_submenu(bar, "File", G_MENU_MODEL(file));
    g_object_unref(file);
    GtkWidget *menu_bar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(bar));
    g_object_unref(bar);
    gtk_box_prepend(GTK_BOX(gtk_window_get_child(GTK_WINDOW(window))), menu_bar);
}


} // namespace satellite004

#endif
