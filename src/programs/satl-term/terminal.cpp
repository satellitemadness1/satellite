// The VTE terminal widget and the interpreter it hosts.
//
// THE WINDOW NEVER INTERPRETS ANYTHING. This file paints a terminal, puts the
// sibling `satl` on the other side of its pty -- child.cpp is what knows where
// that is and what it is told -- and renders the bytes that come back. That is
// what makes the two-binary split nearly free: the two-process shape was
// already there, and the only question the split answers is which binary gets
// spawned. PLAN.md M1.5, and DESIGN.md §10.3 for why the window's thread is not
// the user's problem.

#include "programs/satl-term/terminal.hpp"
#include "programs/satl-term/child.hpp"
#include "programs/opening.hpp"

#include <cstdio>
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
    bool hold_clean_exit = false;

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
    const std::string text = "\r\n" + line + "\r\n";
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

// HOLD ON FAILURE ALWAYS. CLOSE ON SUCCESS ONLY UNTIL M22.
//
// The failure half is permanent: a terminal whose child FAILED is holding the
// only copy of the reason, and closing it destroys the message. It is what
// makes this binary demonstrable before the prompt exists -- `satl --repl`
// today answers "the prompt is not built yet -- it lands at M22" and exits
// EXIT_NOT_YET, so the window stays up with the explanation on it.
//
// THE CLEAN-EXIT ARM BELOW IS M1.5's AND M22 DELETES IT, which is written
// here rather than discovered there. Today the child runs for milliseconds and
// a window outliving every one of them is a window nobody asked to keep. Once
// there is a prompt the question reverses: a person who has been typing has a
// screen full of what they did, the exit word ends a session rather than a
// window, and the close button is how a window closes. PLAN.md M22.
void on_child_exited(VteTerminal *terminal, int status, gpointer user_data)
{
    Session *session = (Session *)user_data;

    // BEFORE ANYTHING ELSE IN THIS FUNCTION, because hold_open below hands the
    // keyboard its second meaning and that meaning is "there is no interpreter".
    session->child_alive = false;

    const bool exited = WIFEXITED(status);
    const int code = exited ? WEXITSTATUS(status) : -1;

    if (!session->hold_clean_exit && exited && code == EXIT_FINE) {
        terminal_finish(session->widget);
        return;
    }

    if (!exited)
        say(terminal, "[satl-term] the interpreter was killed by a signal.");
    else if (code != EXIT_FINE)
        say(terminal, "[satl-term] the interpreter exited " +
                          std::to_string(code) + ".");

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
            error->message);
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
    say(terminal, "[satl-term] cannot find satl beside me.");
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
    session->hold_clean_exit = hold_always;
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
