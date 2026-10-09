// satellite/satellite_variable_window/console_status.cpp -- THE BAR ALONG THE
// BOTTOM OF satl'S OWN CONSOLE: idle or running and the memory satl holds on the
// left, how many of the start-up threads are busy in the middle, and the
// terminal's size in characters on the right.
//
// (the author, 2026-09-22) "can you add a bottom bar that says "idle/running" "0
// mb"(the entire satl interpreter's ram being used) and the chars on the screen?
// 80x24 in the other corner, and then give a thread count too, as "0/1024
// threads active" thread count in the middle".
//
// IT COSTS THE INTERPRETER NOTHING IT WOULD NOTICE. The desk reads everything
// here twice a second, on its own thread: one flag the interpreter stores at
// the edges of a run (machine/run_state.hpp), two counters the pool moves once
// a job (threads/startup_threads.hpp), /proc for the memory, and VTE for the
// size. A label is set only when its text changed.
//
// THE MEMORY IS RESIDENT MEMORY, satl's own and that of the programs it is
// running with `interpret` -- a child satl on this console, the same
// interpreter as far as the person is concerned.
//
// THE THREADS ARE THE START-UP POOL'S (arguments.threads_startup). A program's
// lines run on satl's main thread, which is not one of them, so a running
// program with nothing handed to the pool reads 0 of them active -- which is
// true.

#include "satellite_window.hpp"

#if SATELLITE_HAS_CONSOLE

#include "window_console.hpp"
#include "../machine/run_state.hpp"
#include "../threads/startup_threads.hpp"

#include <fstream>
#include <string>
#include <unistd.h>

namespace satellite004 {
namespace {

struct StatusBar {
    GtkWidget *left;
    GtkWidget *middle;
    GtkWidget *right;
    VteTerminal *terminal;
};

// Resident bytes of one process, from /proc/<pid>/statm's second number.
unsigned long long int resident_bytes(const std::string &pid)
{
    std::ifstream statm("/proc/" + pid + "/statm");
    unsigned long long int size = 0, resident = 0;
    if (!(statm >> size >> resident))
        return 0;
    const long page = sysconf(_SC_PAGESIZE);
    return resident * static_cast<unsigned long long int>(page > 0 ? page : 4096);
}

// satl, and every process it started that is still running: `interpret`'s child.
unsigned long long int satls_resident_bytes()
{
    const std::string self = std::to_string(getpid());
    unsigned long long int bytes = resident_bytes(self);
    std::ifstream children("/proc/" + self + "/task/" + self + "/children");
    std::string child;
    while (children >> child)
        bytes += resident_bytes(child);
    return bytes;
}

void say(GtkWidget *label, const std::string &text)
{
    if (text != gtk_label_get_text(GTK_LABEL(label)))
        gtk_label_set_text(GTK_LABEL(label), text.c_str());
}

// `data` IS THE BAR, held by the timer. THE CONSOLE HAS GONE when the bar is no
// longer in a window: asking is safe because the timer's reference keeps the bar,
// and the answer ends the timer, which lets the bar -- and its StatusBar -- go.
gboolean refresh(gpointer data)
{
    GtkWidget *bar = GTK_WIDGET(data);
    if (gtk_widget_get_root(bar) == nullptr)
        return G_SOURCE_REMOVE;
    StatusBar *status = static_cast<StatusBar *>(g_object_get_data(G_OBJECT(bar), "satl-status"));
    const unsigned long long int megabytes = (satls_resident_bytes() + 512 * 1024) / (1024 * 1024);
    say(status->left, std::string(the_interpreter_is_running().load(std::memory_order_relaxed) ? "running" : "idle") +
                          "   " + std::to_string(megabytes) + " MB");
    say(status->middle, std::to_string(pool_threads_busy()) + "/" + std::to_string(pool_threads_up()) +
                            " threads active");
    say(status->right, std::to_string(vte_terminal_get_column_count(status->terminal)) + "x" +
                           std::to_string(vte_terminal_get_row_count(status->terminal)));
    return G_SOURCE_CONTINUE;
}

GtkWidget *a_label()
{
    GtkWidget *label = gtk_label_new("");
    gtk_widget_set_margin_start(label, 8);
    gtk_widget_set_margin_end(label, 8);
    return label;
}

} // namespace

void give_it_a_status_bar(satellite_window &console)
{
    if (console.widget == nullptr || console.terminal == nullptr)
        return;
    GtkWidget *window = static_cast<GtkWidget *>(console.widget);
    GtkWidget *bar = gtk_center_box_new();
    gtk_widget_set_margin_top(bar, 2);
    gtk_widget_set_margin_bottom(bar, 2);
    StatusBar *status = new StatusBar{a_label(), a_label(), a_label(), terminal_of(console)};
    gtk_center_box_set_start_widget(GTK_CENTER_BOX(bar), status->left);
    gtk_center_box_set_center_widget(GTK_CENTER_BOX(bar), status->middle);
    gtk_center_box_set_end_widget(GTK_CENTER_BOX(bar), status->right);
    // THE BAR OWNS ITS StatusBar AND THE TIMER OWNS A REFERENCE TO THE BAR, so
    // the last of the two to go -- the window or the timer -- frees both.
    g_object_set_data_full(G_OBJECT(bar), "satl-status", status,
                           [](gpointer data) { delete static_cast<StatusBar *>(data); });
    gtk_box_append(GTK_BOX(gtk_window_get_child(GTK_WINDOW(window))), bar);
    refresh(bar);
    g_timeout_add_full(G_PRIORITY_LOW, 500, refresh, g_object_ref(bar), g_object_unref);
}

} // namespace satellite004

#endif
