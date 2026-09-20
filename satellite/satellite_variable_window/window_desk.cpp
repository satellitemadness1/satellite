// satellite/satellite_variable_window/window_desk.cpp -- the one GTK thread.
// window_desk.hpp says why there is one and why it is not started at startup.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4 (make_support/047-window.mk). satl
// itself builds everywhere; bytecode/window_calls.cpp is the one file that says
// so when it was built without a window.

#include "window_desk.hpp"

#include "window_spill.hpp"

#include <gtk/gtk.h>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace satellite004 {
namespace {

std::mutex desk_mutex;                  // guards everything below
std::condition_variable desk_changed;
std::thread desk_thread;
GMainLoop *desk_loop = nullptr;
bool desk_tried = false;                // gtk_init has been attempted
bool desk_running = false;              // ...and it worked
std::string desk_trouble;
std::vector<WindowHandle> open_windows;

// THE JOB, AND THE ONE PLACE IT IS WAITED ON. g_main_context_invoke copies
// nothing and takes a pointer, so the parcel lives on the calling thread's
// stack until `done` says the desk has finished with it.
struct Parcel {
    const std::function<void()> *job;
    std::mutex mutex;
    std::condition_variable finished;
    bool done = false;
};

gboolean run_the_parcel(gpointer as_pointer)
{
    Parcel *parcel = static_cast<Parcel *>(as_pointer);
    (*parcel->job)();
    {
        std::lock_guard<std::mutex> lock(parcel->mutex);
        parcel->done = true;
    }
    parcel->finished.notify_all();
    return G_SOURCE_REMOVE;
}

// THE DESK ITSELF. gtk_init_CHECK and not gtk_init (hello-static.c:155-158):
// it answers false where there is no display instead of dying, and a headless
// machine running `satl batch.satl` must get a refusal it can read, not a
// SIGABRT out of a library it never asked for.
void be_the_desk()
{
    // THE SPILL COMES FIRST, AND "FIRST" IS LITERAL (WIN-1). xkeyboard-config is
    // read inside gtk_init() by a call that null-checks nothing, and glib freezes
    // its GSettings source list on the first lookup -- so a spill one line later
    // is a spill that did nothing, silently. window_spill.hpp has the three.
    //
    // A FAILED SPILL IS NOT FATAL HERE. It is reported through the same path as
    // "no display": a machine whose $XDG_RUNTIME_DIR is full still gets a refusal
    // it can read, rather than the SIGSEGV this whole milestone exists to remove.
    std::string spill_trouble;
    const bool spilled = spill_what_gtk_needs(spill_trouble);

    const bool started = spilled && gtk_init_check() != FALSE;
    GMainLoop *loop = started ? g_main_loop_new(nullptr, FALSE) : nullptr;
    {
        std::lock_guard<std::mutex> lock(desk_mutex);
        desk_tried = true;
        desk_running = started;
        desk_loop = loop;
        if (!spilled)
            desk_trouble = "the window data satl carries could not be written -- " + spill_trouble;
        else if (!started)
            desk_trouble = "there is no display to draw on -- GTK could not open one "
                           "(no Wayland or X11 session in this environment)";
    }
    desk_changed.notify_all();
    if (!started)
        return;
    g_main_loop_run(loop);
}

} // namespace

void close_the_desk_when_the_windows_are();

bool open_the_desk(std::string &why)
{
    std::unique_lock<std::mutex> lock(desk_mutex);
    if (!desk_tried && !desk_thread.joinable())
        desk_thread = std::thread(be_the_desk);
    desk_changed.wait(lock, [] { return desk_tried; });
    if (!desk_running) {
        why = desk_trouble;
        return false;
    }
    return true;
}

void on_the_desk(const std::function<void()> &job)
{
    Parcel parcel;
    parcel.job = &job;
    g_main_context_invoke(nullptr, run_the_parcel, &parcel);
    std::unique_lock<std::mutex> lock(parcel.mutex);
    parcel.finished.wait(lock, [&parcel] { return parcel.done; });
}

void the_desk_holds(const WindowHandle &window)
{
    std::lock_guard<std::mutex> lock(desk_mutex);
    open_windows.push_back(window);
}

// CALLED ON THE DESK'S OWN THREAD, out of GTK's `destroy` signal. It takes the
// same mutex as everything else and must never go through on_the_desk(), which
// would be the desk waiting on itself.
void the_desk_let_go_of(satellite_window *window)
{
    {
        std::lock_guard<std::mutex> lock(desk_mutex);
        for (std::size_t at = 0; at < open_windows.size(); ++at) {
            if (open_windows[at].get() != window)
                continue;
            // AND EVERY PIECE INSIDE IT. GTK frees a window's children with the
            // window, so a button's handle that the program still holds would
            // keep a GtkWidget * that has already been freed -- and it would
            // read as a live button right up until something touched it.
            for (const WindowHandle &piece : open_windows[at]->pieces) {
                piece->on_the_screen = false;
                piece->widget = nullptr;
            }
            open_windows[at]->pieces.clear();
            open_windows[at]->on_the_screen = false;
            open_windows[at]->widget = nullptr;
            open_windows[at]->inside = nullptr;
            open_windows.erase(open_windows.begin() + static_cast<long>(at));
            break;
        }
    }
    desk_changed.notify_all();
}

unsigned long long int windows_open()
{
    std::lock_guard<std::mutex> lock(desk_mutex);
    return open_windows.size();
}

void close_the_desk_now()
{
    {
        std::unique_lock<std::mutex> lock(desk_mutex);
        if (!desk_thread.joinable())
            return;
    }
    // THE WINDOWS GO DOWN ON THE DESK'S THREAD, as every other GTK call does.
    // The list is copied under the lock and walked outside it, because each
    // destroy calls the_desk_let_go_of, which takes that same lock.
    std::vector<WindowHandle> taking_down;
    {
        std::lock_guard<std::mutex> lock(desk_mutex);
        taking_down = open_windows;
    }
    for (const WindowHandle &one : taking_down) {
        GtkWidget *widget = static_cast<GtkWidget *>(one->widget);
        if (widget != nullptr)
            on_the_desk([widget] { gtk_window_destroy(GTK_WINDOW(widget)); });
    }
    close_the_desk_when_the_windows_are();
}

void close_the_desk_when_the_windows_are()
{
    GMainLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(desk_mutex);
        if (!desk_thread.joinable())
            return;                                  // no window was ever opened
        desk_changed.wait(lock, [] { return open_windows.empty(); });
        loop = desk_loop;
    }
    // OUTSIDE THE LOCK: g_main_loop_quit wakes the desk, which may take the
    // mutex on its way out.
    if (loop != nullptr)
        g_main_loop_quit(loop);
    desk_thread.join();
    std::lock_guard<std::mutex> lock(desk_mutex);
    if (desk_loop != nullptr) {
        g_main_loop_unref(desk_loop);
        desk_loop = nullptr;
    }
    desk_running = false;
}

} // namespace satellite004
