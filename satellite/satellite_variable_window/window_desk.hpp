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
// SOMETHING HAPPENED, AND WHICH THREAD RUNS THE CAPSULE FOR IT (WIN-11, GTK-9).
// ---------------------------------------------------------------------------
//
// IT WAS CALLED A PRESS UNTIL GTK-9 and it was renamed rather than redefined. A
// button being pressed was the only thing that could reach satellite code, so
// `APress` was the truth; a text box typed in, a slider moved, a checkbox
// ticked and a window closed all arrive on this same queue now, and a name that
// said "press" for all five would have been a comment that lies.
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
//   * Something that happens while the program is still running its own lines
//     WAITS. It is not lost and it is not run underneath the program.
//   * They run ONE AT A TIME, in the order they happened. A press made while a
//     capsule is running waits for that capsule to finish.
//   * Something that happens after the last window closed is still drained, so
//     a button pressed at the moment the window went away is not silently
//     dropped -- and a window's own `.closed` capsule is only ever drained.
//
// REVERSIBLE, AND THIS IS WHERE TO REVERSE IT: running the capsule on the desk
// would mean giving the walker its own state per thread, which is a language
// decision and not a window one.
// ONE THING THAT HAPPENED, WAITING TO BE RUN: which capsule answers it, WHAT it
// happened to, and the window it happened in. All three are settled ON THE DESK
// at the moment it happens and carried, rather than worked out later on the
// interpreter's thread -- because later the window may be gone. A press that
// closes the last window and a press queued behind it are both ordinary, and
// the second one is still owed the window it happened in, closed or not: a
// closed window is a thing satellite can hold and ask (`w.ok` is false).
//
// THE WINDOW IS FOUND BY WALKING UP (GTK-7), never by reading `inside_of`
// directly -- a button in a row points at the ROW.
//
// HANDLES AND NOT POINTERS, so nothing can be freed between the press and the
// run; the queue's own reference is what guarantees it.
struct AnEvent {
    std::string capsule;
    WindowHandle piece;    // what it happened to: the button, the text box, the window
    WindowHandle window;   // the window it happened in, null only if it was in none

    // AND WHAT IT SAID, for the kinds that have anything to say: a key (GTK-14)
    // and a person's answer to a question (GTK-11).
    //
    // IT TRAVELS ON THE EVENT AND NOT ON THE PIECE, and that is what keeps this
    // module free of a std::string written on the desk and read on the
    // interpreter. The desk fills this in; the INTERPRETER copies it onto the
    // piece as it takes the event off the queue, on its own thread, just before
    // running the capsule. So `the_window.key` reads a string only one thread
    // ever writes.
    //
    // `said_what` SAYS WHICH FIELD IT GOES IN, and it is an enum rather than one
    // shared string because a key pressed while a question is open would
    // otherwise overwrite the answer -- two different things a window is holding
    // at the same moment.
    //
    // AND WHERE IT HAPPENED, FOR A CLICK (GTK-15's leftover, 2026-09-22): the
    // point on the piece, in its own pixels. `a_place` says the two numbers
    // are filled in, and the interpreter copies them onto the piece the same
    // way it copies `said` -- one writer, on its own thread.
    //
    // AND A LINE A PERSON FINISHED IN A CONSOLE (GTK-17), the same way: the
    // desk read it off the pty, and the interpreter copies it onto the piece
    // for `.typed` to answer.
    enum What { nothing_said, a_key, an_answer, a_place, a_line };
    What said_what = nothing_said;
    std::string said;
    long long int across = 0;
    long long int down = 0;
};

// `may_collapse` IS TRUE FOR A CHANGE AND FALSE FOR A PRESS, and the difference
// is not a tuning knob (GTK-9).
//
// A SLIDER DRAGGED ACROSS THE SCREEN EMITS `value-changed` DOZENS OF TIMES. The
// capsule runs after the program's own lines, reads the value that is there
// THEN, and would answer the same number dozens of times over -- so consecutive
// identical events collapse into one. The capsule reads the live piece, so the
// one that runs sees the latest state: collapsing loses nothing a capsule could
// have observed.
//
// A PRESS NEVER COLLAPSES. Pressing a button three times IS three presses, and
// press-a-button.sh clicks three times and counts three lines. Two presses are
// two things a person did; two positions of one slider on the way somewhere are
// not two things a person did.
void the_desk_saw_something(const std::string &capsule, const WindowHandle &piece,
                            bool may_collapse = false, const std::string &said = std::string(),
                            AnEvent::What said_what = AnEvent::nothing_said,
                            long long int across = 0, long long int down = 0);

// WAITS FOR THE NEXT ONE, ON THE INTERPRETER'S THREAD. True with `capsule`
// filled in when there is one to run; false when every window is closed and
// nothing is left -- which is when the run is over. False at once when the desk
// was never opened.
//
// EVERY WINDOW OF THE PROGRAM'S, THAT IS. The console satl launched for itself
// (`is_satls_own`, GTK-17) is not one the run waits on: a program that printed
// three lines into it and returned is finished, and main() is what closes or
// holds that console afterwards.
bool the_desk_waits_for_something(AnEvent &happened);

// TAKES THE PROGRAM'S WINDOWS DOWN and leaves satl's own console, if there is
// one, on the screen: what a run that STOPPED inside a console does, so the
// person reads the report there without a dead button beside it (GTK-17).
void close_the_program_windows_now();

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
