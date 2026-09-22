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
#include <deque>
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

// PRESSES WAITING TO BE RUN, oldest first (WIN-11). Written by the desk out of
// GTK's `clicked` and read by the interpreter's thread, under the one mutex
// everything else here is under. THE NAME IS COPIED AT PRESS TIME and nothing
// else is kept: no handle, no widget, nothing that a window going away could
// leave dangling between the press and the run.
//
// NOT BOUNDED. A person cannot press a button faster than a capsule runs often
// enough to matter, and a bound here would be a limit the language does not
// have -- a press silently dropped is exactly the answer that is wrong and does
// not say so.
std::deque<AnEvent> presses;

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
namespace {

// A PIECE AND EVERYTHING UNDER IT. Depth-first, and it does not clear the
// `pieces` vectors on the way down: a program still holding a row is entitled to
// ask that row what it held, and those handles are what keep the answer alive.
// Only the WINDOW's own vector is cleared, by the caller, because the window is
// what the desk was holding open.
void let_go_of_every_piece(satellite_window &holder)
{
    for (const WindowHandle &piece : holder.pieces) {
        if (piece->holds_pieces())
            let_go_of_every_piece(*piece);
        piece->on_the_screen = false;
        piece->widget = nullptr;
    }
}

} // namespace

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
            // WHAT A PERSON TYPED IS **NOT** RESCUED HERE, and that was tried
            // first (GTK-2, 2026-09-21). By the time a window's `destroy` reaches
            // us every piece inside it has already been disposed:
            // gtk_window_dispose unparents the child BEFORE chaining to
            // gtk_widget_dispose, which is what emits this signal. Reading them
            // here printed five Gtk-CRITICAL assertion failures and answered
            // nothing.
            //
            // AND THE PIECE'S OWN `destroy` IS NO BETTER. gtk_entry_dispose clears
            // priv->text and gtk_text_view_dispose calls
            // gtk_text_view_set_buffer(view, NULL) -- both before they chain up to
            // the dispose that emits `destroy`. There is no moment in a teardown
            // at which a widget's words can still be asked for.
            //
            // SO `.text` REFUSES ON A CLOSED TEXT BOX and says to read it while
            // the window is open (window_text_of). That is also what keeps this
            // whole module free of a std::string written on the desk's thread and
            // read on the interpreter's: `text` has exactly one writer, and it is
            // not this thread.
            // EVERY PIECE, HOWEVER DEEP (GTK-7). A row inside a window holds
            // pieces of its own and GTK frees the whole tree with the window, so
            // walking only the window's direct children would leave a button in
            // a row holding a GtkWidget * that has been freed -- and it would
            // read as a live button right up until something touched it, which
            // is the exact failure this loop was written to prevent.
            let_go_of_every_piece(*open_windows[at]);
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

// ON THE DESK'S OWN THREAD, out of GTK's `clicked` -- the same rule as
// the_desk_let_go_of above, and for the same reason: the desk must never go
// through on_the_desk(), which would be the desk waiting on itself.
void the_desk_saw_something(const std::string &capsule, const WindowHandle &piece, bool may_collapse,
                            const std::string &said)
{
    {
        std::lock_guard<std::mutex> lock(desk_mutex);
        // THE SAME CHANGE, AGAIN, WITH NOTHING RUN IN BETWEEN. A slider dragged
        // across the screen emits `value-changed` dozens of times and the
        // capsule would answer the same number dozens of times over. Only the
        // BACK of the queue is looked at: anything further in was separated by
        // something else happening, which makes it a different moment.
        if (may_collapse && !presses.empty() && presses.back().capsule == capsule &&
            presses.back().piece == piece)
            return;
        // THE WINDOW IS LOOKED UP HERE, ON THE DESK, while it is certainly
        // alive -- `.append` wrote the link and the desk holds the window open.
        // THE WINDOW AND NOT THE IMMEDIATE PARENT (GTK-7). A button in a row
        // points at the ROW, and handing a capsule a row where it declared a
        // window would fail at `its_window.close()` -- "only a window can be
        // closed" -- which is a refusal about a line that is right.
        presses.push_back(AnEvent{capsule, piece,
                                  piece == nullptr ? WindowHandle() : the_window_holding(*piece),
                                  said});
    }
    desk_changed.notify_all();
}

bool the_desk_waits_for_something(AnEvent &happened)
{
    std::unique_lock<std::mutex> lock(desk_mutex);
    if (!desk_thread.joinable())
        return false;                                // no window was ever opened
    desk_changed.wait(lock, [] { return !presses.empty() || open_windows.empty(); });
    // THE QUEUE FIRST, AND THAT ORDER IS THE POINT. A button pressed in the
    // same instant its window was closed has both conditions true at once, and
    // testing the windows first would throw that press away -- the one case
    // where a person pressed something and nothing happened. Since GTK-9 it also
    // guarantees a window's own `.closed` capsule runs: that one is queued by
    // the `destroy` handler, which is the very thing that empties open_windows.
    if (presses.empty())
        return false;
    happened = presses.front();
    presses.pop_front();
    return true;
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
    // NOTHING WAITING IS RUN AFTER A REFUSAL. The report is printed and the run
    // has stopped; a capsule walked now would be a program running on after it
    // was told it could not.
    {
        std::lock_guard<std::mutex> lock(desk_mutex);
        presses.clear();
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
