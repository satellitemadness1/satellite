#pragma once
// satellite/satellite_variable_window/window_desk.hpp -- THE ONE GTK THREAD.
// SATELLITE_WINDOW.md WIN-2.
//
// The author, 2026-09-19: *"the main gtk+ window starts automatically in another
// thread"*. This is that thread, and it is ONE thread and not one per window.
//
// WHY NOT A THREAD PER WINDOW, which is what "a thread per gtk object" would
// read as: GTK4 is not thread-safe, and every call must happen on the thread
// that called gtk_init (gtk's own question_index.md). A second thread calling
// into GTK does not run slowly, it corrupts. So the desk is one thread that owns
// gtk_init, the GMainContext and every widget, and the interpreter hands work
// over to it -- which is the shape GTK itself is built for.
//
// WHY IT IS NOT STARTED AT STARTUP. `satl batch.satl` on a headless server draws
// nothing, and warming would cost it a Wayland connection, a fontconfig scan and
// an EGL context on every run -- a failed connect, at that. So the desk opens the
// first time a window word actually runs, and a program with no window never
// pays. WIN-2 wants better than this eventually: satl already tokenises the whole
// file before running a line, so it CAN know a window word is coming and warm
// while the program starts. That is a speed-up on top of this, not a different
// design.
//
// MEASURED BEFORE IT WAS WRITTEN (2026-09-20): gtk_init_check on a second thread,
// g_main_loop_run there, and g_main_context_invoke from the first thread putting
// an 800x600 window with a button on the screen -- clean exit 0.

#include "satellite_window.hpp"

#include <functional>
#include <string>

namespace satellite004 {

// Starts the desk if it is not running, and returns once gtk_init has been tried.
// False with `why` filled in when there is no display to draw on -- which is not
// a crash and must not be: satl runs on machines with no screen every day.
bool open_the_desk(std::string &why);

// RUNS `job` ON THE DESK AND WAITS FOR IT. The waiting is the point: a window
// word answers a handle, and the handle is not made until the desk has made the
// widget. Never call it from the desk itself -- nothing in satl does, because
// only the interpreter thread runs a program.
void on_the_desk(const std::function<void()> &job);

// A WINDOW THE DESK HOLDS OPEN, by the same handle the program holds. Told to
// the desk when a window is presented; forgotten when GTK says `destroy`,
// whoever caused it -- the program, or a person clicking the close button.
//
// THE DESK'S OWN STRONG REFERENCE IS WHAT MAKES THE SIGNAL SAFE. `destroy`
// arrives on the desk's thread and is handed the satellite_window by raw
// pointer; a program that has already dropped its last name for the window
// would have freed it, and the handler would read freed memory. Holding one
// more reference for exactly as long as the window is on the screen costs a
// pointer and removes the whole question.
void the_desk_holds(const WindowHandle &window);

// THE DESK LETTING GO, called on the DESK'S OWN THREAD out of GTK's `destroy`
// signal -- never from the interpreter's. It is what makes a person clicking
// the close button and a program calling `.close()` the same event.
void the_desk_let_go_of(satellite_window *window);

// How many windows are on the screen right now.
unsigned long long int windows_open();

// ---------------------------------------------------------------------------
// A PRESS, AND WHICH THREAD RUNS THE CAPSULE FOR IT (WIN-11).
// ---------------------------------------------------------------------------
//
// THE INTERPRETER'S THREAD RUNS IT, ALWAYS, AND THE DESK NEVER DOES. A capsule
// is walked by run_statements, which reads the one BytecodeRegistry and writes
// the one MachineState, and the statement ring is a global beside them. Two
// threads in there at once is not a slow program, it is a corrupt one -- the
// same sentence this file already says about GTK, pointing the other way. So
// the desk's `clicked` handler does the one thing it safely can: it writes the
// capsule's NAME down and wakes whoever is waiting.
//
// SO A PRESS IS A QUEUE AND NOT A CALL, and the consequences are worth saying
// out loud rather than discovering:
//
//   * A press that arrives while the program is still running its own lines
//     WAITS. It is not lost and it is not run underneath the program.
//   * Presses run ONE AT A TIME, in the order they were made. A press made
//     while a capsule is running waits for that capsule to finish.
//   * A press that arrives after the last window closed is still drained, so a
//     button pressed at the moment the window went away is not silently dropped.
//
// REVERSIBLE, AND THIS IS WHERE TO REVERSE IT: running the capsule on the desk
// would mean giving the walker its own state per thread, which is a language
// decision and not a window one.
// ONE PRESS, WAITING TO BE RUN: which capsule answers it, WHAT was pressed, and
// the window that was pressed in. All three are settled ON THE DESK at the
// moment of the press and carried, rather than worked out later on the
// interpreter's thread -- because later the window may be gone. A press that
// closes the last window and a press queued behind it are both ordinary, and
// the second one is still owed the window it happened in, closed or not: a
// closed window is a thing satellite can hold and ask (`w.ok` is false).
//
// HANDLES AND NOT POINTERS, so nothing can be freed between the press and the
// run; the queue's own reference is what guarantees it.
struct APress {
    std::string capsule;
    WindowHandle piece;    // the button
    WindowHandle window;   // the window it is in, null only if it was in none
};

void the_desk_saw_a_press(const std::string &capsule, const WindowHandle &piece);

// WAITS FOR THE NEXT PRESS, ON THE INTERPRETER'S THREAD. True with `capsule`
// filled in when there is one to run; false when every window is closed and no
// press is left -- which is when the run is over. False at once when the desk
// was never opened.
bool the_desk_waits_for_a_press(APress &press);

// BLOCKS UNTIL EVERY WINDOW IS CLOSED, then stops the desk and joins it. Returns
// at once when the desk was never opened.
void close_the_desk_when_the_windows_are();

// TAKES EVERY WINDOW DOWN AND STOPS THE DESK, without waiting for anybody. This
// is what a REFUSED run does: the report has already been printed, and leaving a
// window on the screen after it would hold the program open for a person who has
// been told it stopped -- a hang that reads exactly like the interpreter locking
// up. A run that finished uses the one above.
void close_the_desk_now();

} // namespace satellite004
