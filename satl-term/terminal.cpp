// satellite 004 (PLAN M0.5, 2026-09-17): ported from 003 revision 07's
// src/programs/satl-term/, nearly as-is. What changed here: a held tab names
// the machine code satl stopped on -- "satl stopped on machine code 14
// (not_built_yet)" -- and the signal that killed it; 0 is 004's success. 004's
// satl has no Ctrl-C handler yet, so Ctrl-C at a running program is a signal
// here. The milestones and sections named below are 003's.
//
// The VTE terminal widget and the interpreter it hosts.
//
// THE WINDOW NEVER INTERPRETS ANYTHING. This file paints a terminal, puts the
// sibling `satl` on the other side of its pty -- child.cpp is what knows where
// that is and what it is told -- and renders the bytes that come back. That is
// what makes the two-binary split nearly free: the two-process shape was
// already there, and the only question the split answers is which binary gets
// spawned. PLAN.md M1.5, and DESIGN.md §10.3 for why the window's thread is not
// the user's problem.

#include "terminal.hpp"
#include "child.hpp"
#include "../satellite/machine/machine_codes.hpp"
#include "../satellite/machine/shown.hpp"

#include <cstdio>
#include <cstring>
#include <sys/wait.h>

namespace satellite {
namespace {

// Everything this file knows about ONE terminal, and it hangs off the widget
// rather than off the file.
//
// THESE WERE FILE STATICS UNTIL File > New tab, and the comment they carried --
// that a VTE callback has one gpointer and it was already spoken for by the
// window -- is now answered the other way round: the gpointer carries THIS, and
// the window is reached through the tabs that own it. Left as statics, a second
// tab would have been a second interpreter writing to the first one's flags,
// and that is not a bug that announces itself: it looks like Ctrl-C in the idle
// tab closing the busy one.
struct Session {
    GtkWidget *widget = nullptr;

    // Whether there is still an interpreter on the other side of the pty.
    // keys.cpp asks this at the moment a key arrives, because it is the whole
    // of what Ctrl-C means -- a live child owns the key and a dead one does not.
    //
    // SET OPTIMISTICALLY, BEFORE THE SPAWN IS KNOWN TO HAVE WORKED, and cleared
    // if it did not. The honest-looking version -- set it in the spawn callback
    // -- leaves a window of a few milliseconds where the child is starting and
    // this answers `false`, and a Ctrl-C landing in that window would be read as
    // "nothing is running" and close the tab out from under a program that was
    // about to run. Wrong in this direction costs a keystroke passed harmlessly
    // to a pty; wrong in the other costs the run.
    bool child_alive = false;

    // Whether the screen is being held with its child gone -- exactly the state
    // its own message calls "press any key to close". CLEARED BY terminal_run:
    // "nothing brings a child back" was true of this flag until the day the
    // File menu could start one.
    bool held = false;

    // Whether to keep this terminal up after a CLEAN exit. A failing child
    // holds it regardless; see on_child_exited.
    //
    // NOT CONSTANT FOR THE LIFE OF A TAB, SINCE M22. It follows what the tab is
    // RUNNING -- set for the prompt, clear for a file -- and terminal_run
    // recomputes it, because Open... starts a file in a tab that may well have
    // been a prompt a moment ago.
    bool hold_clean_exit = false;

    // `--hold`, which outranks the above and does not change. Kept separately
    // for exactly that reason: recomputing hold_clean_exit from the file would
    // otherwise silently throw the flag the user passed on the command line
    // away the first time they opened something.
    bool hold_forced = false;

    TerminalFinished finished = nullptr;
};

// The widget OWNS its Session and frees it when it is finalised, which is what
// makes a closed tab leak nothing and makes it impossible to be holding the
// state of a terminal that is gone.
const char *SESSION = "satl-term-session";

Session *session_of(GtkWidget *terminal)
{
    if (!terminal)
        return nullptr;
    return (Session *)g_object_get_data(G_OBJECT(terminal), SESSION);
}

void forget_session(gpointer memory)
{
    delete (Session *)memory;
}

// The window's palette. Black is the default foreground, so everything that
// arrives without an escape sequence -- what the user types, program output,
// error reports -- is black on the light blue field.
//
// Palette left null: only the default pair is being set, and the 16 ANSI
// indices keep VTE's own values.
void apply_colors(VteTerminal *terminal)
{
    GdkRGBA background, foreground;
    gdk_rgba_parse(&background, "#90D5FF");
    gdk_rgba_parse(&foreground, "#000000");

    vte_terminal_set_colors(terminal, &foreground, &background, nullptr, 0);
}

// IBM Plex Mono, and the comma is doing real work: a Pango family field takes
// an ordered list, so an absent Plex falls through to whatever the system calls
// monospace rather than to whatever fontconfig picks when asked for a family
// that does not exist -- which can be proportional, and a proportional terminal
// is unusable rather than merely different.
//
// The font is NOT read from satellite.library the way system.max_depth will be,
// because this binary links the window and nothing of the runtime; pulling the
// library in to make one string configurable would drag the interpreter into a
// binary whose whole point is that the interpreter is not in it.
void apply_font(VteTerminal *terminal)
{
    PangoFontDescription *font =
        pango_font_description_from_string("IBM Plex Mono,monospace 11");
    vte_terminal_set_font(terminal, font);
    pango_font_description_free(font);
}

// Write a line to the DISPLAY rather than to the child. vte_terminal_feed is
// the terminal being told what to draw; everything said this way is the window
// talking about itself, and none of it is for the program.
void say(VteTerminal *terminal, const std::string &line)
{
    // shown() (004): a file name or a spawn error can hold ESC or BEL -- a folder
    // named with them, or a program picked in File > Open -- and this is the
    // window talking, so it says them as text (DESIGN §9). No line said here
    // carries a control byte of its own.
    const std::string text = "\r\n" + satellite004::shown(line) + "\r\n";
    vte_terminal_feed(terminal, text.c_str(), (gssize)text.size());
}

// Any key closes a terminal that is being held.
//
// THE CONTROLLER IS keys.cpp's AND NOT THIS FILE'S, which is a change from how
// this read at M1.5. It used to add a capture phase controller of its own here,
// and that was correct while "any key closes" was the only thing this window
// thought about a keystroke. It is now one of three rules -- Ctrl-C copies a
// selection when no interpreter is running, which is EXACTLY this state -- and
// two capture phase controllers on one widget answer the same press in whatever
// order GTK holds them in. One controller, one decision, one file that can be
// read to find out what a key does.
//
// The message stays here because it is the exit policy talking, and the policy
// is this file's. keys.cpp is what makes it true.
//
// IT NO LONGER SAYS "WINDOW". A held tab closes the tab and the last tab takes
// the window with it, so either noun would be right in one case and a lie in
// the other. The verb on its own is true in both.
void hold_open(Session *session)
{
    say(VTE_TERMINAL(session->widget),
        "[satl-term] press any key to close, or ctrl+c to copy what you have "
        "highlighted.");

    session->held = true;
}

// HOLD ON FAILURE ALWAYS. CLOSE ON SUCCESS ONLY WHEN A FILE WAS RUN.
//
// The failure half is permanent: a terminal whose child FAILED is holding the
// only copy of the reason, and closing it destroys the message. It is what made
// this binary demonstrable before the prompt existed -- `satl --repl` answered
// "the prompt is not built yet -- it lands at M22" and exited EXIT_NOT_YET, so
// the window stayed up with the explanation on it.
//
// THE CLEAN-EXIT ARM BELOW WAS M1.5's, AND M22 NARROWED IT RATHER THAN
// DELETING IT -- terminal_new below carries the argument, and the short version
// is that tabs arrived between PLAN writing that sentence and this milestone
// reaching it. A tab running the prompt now sets hold_clean_exit for itself, so
// the exit word ends a session and leaves the screen; a tab running a file
// still closes when the file is done, which is what TERM.md's "a tab closes
// when its interpreter is finished with" means and what Open... relies on.
void on_child_exited(VteTerminal *terminal, int status, gpointer user_data)
{
    Session *session = (Session *)user_data;

    // BEFORE ANYTHING ELSE IN THIS FUNCTION, because hold_open below hands the
    // keyboard its second meaning and that meaning is "there is no interpreter".
    session->child_alive = false;

    const bool exited = WIFEXITED(status);
    const int code = exited ? WEXITSTATUS(status) : -1;

    if (!session->hold_clean_exit && exited && code == satellite004::success) {
        terminal_finish(session->widget);
        return;
    }

    // THE CODE BY NAME (PLAN M0.5). An exit status is 8 bits and a machine code
    // is not, so satl exits 255 for any code that does not fit and has already
    // written the whole code above this line (exit_status.hpp). A satl from 003
    // beside this binary exits 003's statuses, which these names do not describe.
    if (!exited && WIFSIGNALED(status))
        say(terminal, "[satl-term] satl was stopped by signal " + std::to_string(WTERMSIG(status)) + " (" +
                          strsignal(WTERMSIG(status)) + ").");
    else if (!exited)
        say(terminal, "[satl-term] the interpreter was killed by a signal.");
    else if (code == 255)
        say(terminal, "[satl-term] satl stopped on a machine code an exit status cannot hold; "
                      "it is written in full above.");
    else if (code != satellite004::success)
        say(terminal, "[satl-term] satl stopped on machine code " + std::to_string(code) + " (" +
                          satellite004::machine_code_name(code) + ").");

    hold_open(session);
}

// NOT a silent close. A terminal that vanishes the instant it opens tells the
// user nothing, and the thing that just failed is the one thing this binary
// exists to do. child.cpp has already said it to stderr, for the person who
// typed `satl-term` in a shell; this is the half they can see.
void on_spawn_done(VteTerminal *terminal, GPid, GError *error, gpointer user_data)
{
    if (!error)
        return;

    Session *session = (Session *)user_data;

    // The optimism above is corrected here, and this is the only place that
    // can: a spawn that fails never produces a child, so "child-exited" never
    // fires and nothing else would ever clear the flag.
    session->child_alive = false;

    fprintf(stderr, "satl-term: failed to spawn the interpreter: %s\n",
            satellite004::shown(error->message).c_str());
    say(terminal, std::string("[satl-term] could not start the interpreter: ") +
                      error->message);
    hold_open(session);
}

// Put a child on the pty, and answer for it if there is nowhere to get one.
// The two callers -- a terminal being built, and a finished terminal being
// handed a second program -- reach the interpreter by this one road.
void start_child(Session *session,
                 const std::string &file,
                 const std::vector<std::string> &args)
{
    VteTerminal *terminal = VTE_TERMINAL(session->widget);

    // See child_alive's note: true from the moment the spawn is ASKED FOR, so
    // that a Ctrl-C arriving while the interpreter is still starting is the
    // child's key and not the window's.
    session->child_alive = true;

    if (child_spawn(terminal, file, args, on_spawn_done, session))
        return;

    session->child_alive = false;
    say(terminal, "[satl-term] cannot read my own path on disk, so I cannot find the satl beside me.");
    hold_open(session);
}

} // namespace

GtkWidget *terminal_new(const std::string &file,
                        const std::vector<std::string> &args,
                        bool hold_always,
                        TerminalFinished finished)
{
    GtkWidget *widget = vte_terminal_new();
    VteTerminal *terminal = VTE_TERMINAL(widget);

    Session *session = new Session();
    session->widget = widget;

    // M22, AND IT IS THE MILESTONE'S SECOND HALF. `|| file.empty()` is the
    // whole change: a tab with no file runs the PROMPT (child.cpp adds
    // `--repl` in exactly that case), and a prompt that exits cleanly must
    // leave its screen behind. PLAN M22: "A person who has been typing at a
    // prompt has a screen full of what they did, and the exit word is the end
    // of a session rather than the end of a window."
    //
    // AND A TAB RUNNING A FILE STILL CLOSES, which is where this departs from
    // PLAN's letter and keeps its argument. §8 says the clean-exit arm "is
    // marked as M1.5's and this milestone removes it" -- written before tabs
    // existed, when the only child was the window's own and removing the arm
    // and holding the prompt were the same act. They are not any more: TERM.md
    // has "a tab closes when its interpreter is finished with", Open... runs a
    // file in a tab, and deleting the arm would leave every finished program
    // sitting in a tab the user has to dismiss by hand. The reason PLAN gives
    // for closing -- "the child runs for milliseconds and a window that
    // outlived every one of them would only ever be a window nobody asked to
    // keep" -- is still exactly true of a file, and no longer true of a prompt.
    // So the arm stays and the prompt opts out of it.
    session->hold_forced = hold_always;
    session->hold_clean_exit = hold_always || file.empty();
    session->finished = finished;
    g_object_set_data_full(G_OBJECT(widget), SESSION, session, forget_session);

    apply_colors(terminal);
    apply_font(terminal);

    // CONNECTED BEFORE THE SPAWN, and the order is load-bearing rather than
    // tidy: a child that exits before this line runs is a child whose exit
    // nobody hears, and the tab it was in would sit there forever holding a
    // screen its own policy meant to close.
    g_signal_connect(terminal, "child-exited",
                     G_CALLBACK(on_child_exited), session);

    start_child(session, file, args);
    return widget;
}

void terminal_run(GtkWidget *terminal,
                  const std::string &file,
                  const std::vector<std::string> &args)
{
    Session *session = session_of(terminal);
    if (!session || session->child_alive)
        return;

    // The held screen is DISMISSED HERE rather than by the keystroke it was
    // waiting for, and it has to be: the next line puts a child back on the pty,
    // and a tab still answering "any key closes me" would close itself under the
    // first character somebody typed at the program they had just opened.
    session->held = false;

    // THE POLICY FOLLOWS THE CHILD, and this line is why hold_forced exists.
    // A tab that was a prompt holds on a clean exit; the moment a FILE is run
    // in it that stops being true, or every program opened from the File menu
    // would leave a tab behind for the user to dismiss.
    session->hold_clean_exit = session->hold_forced || file.empty();

    say(VTE_TERMINAL(terminal), "[satl-term] running " + file);
    start_child(session, file, args);
}

bool terminal_is_running(GtkWidget *terminal)
{
    const Session *session = session_of(terminal);
    return session && session->child_alive;
}

bool terminal_is_held(GtkWidget *terminal)
{
    const Session *session = session_of(terminal);
    return session && session->held;
}

void terminal_finish(GtkWidget *terminal)
{
    Session *session = session_of(terminal);
    if (session && session->finished)
        session->finished(terminal);
}

} // namespace satellite
