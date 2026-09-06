// The VTE terminal widget and the interpreter it hosts.
//
// THE WINDOW NEVER INTERPRETS ANYTHING. This file spawns the sibling `satl`
// into a PTY and renders the bytes that come back, which is what makes the
// two-binary split nearly free: the two-process shape was already there, and
// the only question the split answers is which binary gets spawned. PLAN.md
// M1.5, and DESIGN.md §10.3 for why the window's thread is not the user's
// problem.

#include "programs/satl-term/terminal.hpp"
#include "programs/opening.hpp"
#include "programs/satl-term/keys.hpp"

#include <vte/vte.h>

#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>

namespace satellite {
namespace {

// Whether to keep the window up after a CLEAN exit. A failing child holds it
// regardless; see on_child_exited. A file static because the VTE callbacks
// carry one gpointer and it is already spoken for by the window.
bool hold_clean_exit = false;

// Whether there is still an interpreter on the other side of the pty. keys.cpp
// asks this at the moment a key arrives, because it is the whole of what
// Ctrl-C means -- a live child owns the key and a dead one does not.
//
// SET OPTIMISTICALLY, BEFORE THE SPAWN IS KNOWN TO HAVE WORKED, and cleared by
// on_spawn_done if it did not. The honest-looking version -- set it in the
// spawn callback -- leaves a window of a few milliseconds where the child is
// starting and this answers `false`, and a Ctrl-C landing in that window would
// be read as "nothing is running" and close the window out from under a
// program that was about to run. Wrong in this direction costs a keystroke
// passed harmlessly to a pty; wrong in the other costs the run.
bool child_alive = false;

// The question keys.cpp holds a pointer to. A function rather than the bool
// itself, so that what the window may know about the child stays one door
// wide: the day a child is tracked by pid rather than by flag, this is the
// only line that learns it.
bool interpreter_is_running()
{
    return child_alive;
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
// the terminal being told what to draw; by every point this is called there is
// nothing on the other side of the PTY to write to.
void say(VteTerminal *terminal, const std::string &line)
{
    const std::string text = "\r\n" + line + "\r\n";
    vte_terminal_feed(terminal, text.c_str(), (gssize)text.size());
}

// Any key closes a window that is being held.
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
// A held window still has its close button, so this is a convenience -- but a
// message that says "press any key" and then ignores one is worse than no
// message.
void hold_open(VteTerminal *terminal)
{
    say(terminal, "[satl-term] press any key to close this window, or "
                  "ctrl+c to copy what you have highlighted.");

    close_on_any_key();
}

// HOLD ON FAILURE ALWAYS. CLOSE ON SUCCESS ONLY UNTIL M22.
//
// The failure half is permanent: a terminal whose child FAILED is holding the
// only copy of the reason, and destroying the window destroys the message. It
// is what makes this binary demonstrable before the prompt exists -- `satl
// --repl` today answers "the prompt is not built yet -- it lands at M22" and
// exits EXIT_NOT_YET, so the window stays up with the explanation on it.
//
// THE CLEAN-EXIT ARM BELOW IS M1.5's AND M22 DELETES IT, which is written
// here rather than discovered there. Today the child runs for milliseconds and
// a window outliving every one of them is a window nobody asked to keep. Once
// there is a prompt the question reverses: a person who has been typing has a
// screen full of what they did, the exit word ends a session rather than a
// window, and the close button is how a window closes. PLAN.md M22.
void on_child_exited(VteTerminal *terminal, int status, gpointer user_data)
{
    GtkWidget *window = GTK_WIDGET(user_data);

    // BEFORE ANYTHING ELSE IN THIS FUNCTION, because hold_open below hands the
    // keyboard its second meaning and that meaning is "there is no interpreter".
    child_alive = false;

    const bool exited = WIFEXITED(status);
    const int code = exited ? WEXITSTATUS(status) : -1;

    if (!hold_clean_exit && exited && code == EXIT_FINE) {
        gtk_window_destroy(GTK_WINDOW(window));
        return;
    }

    if (!exited)
        say(terminal, "[satl-term] the interpreter was killed by a signal.");
    else if (code != EXIT_FINE)
        say(terminal, "[satl-term] the interpreter exited " +
                          std::to_string(code) + ".");

    hold_open(terminal);
}

// NOT a silent destroy. A window that closes the instant it opens tells the
// user nothing, and the thing that just failed is the one thing this binary
// exists to do. It goes to stderr as well, because the person who typed
// `satl-term` in a shell is looking there.
void on_spawn_done(VteTerminal *terminal, GPid, GError *error, gpointer)
{
    if (!error)
        return;

    // The optimism above is corrected here, and this is the only place that
    // can: a spawn that fails never produces a child, so "child-exited" never
    // fires and nothing else would ever clear the flag.
    child_alive = false;

    fprintf(stderr, "satl-term: failed to spawn the interpreter: %s\n",
            error->message);
    say(terminal, std::string("[satl-term] could not start the interpreter: ") +
                      error->message);
    hold_open(terminal);
}

// The `satl` sitting NEXT TO this binary -- not this binary again, and not
// whatever PATH happens to resolve.
//
// /proc/self/exe rather than argv[0], because argv[0] is whatever the caller
// chose to put there and a desktop launcher's is not a path at all. An empty
// return means the question could not be answered, which the caller reports
// rather than papering over with a guess.
std::string interpreter_beside_me()
{
    char self[4096];
    const ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n < 0)
        return std::string();
    self[n] = '\0';

    const std::string path(self);
    const size_t slash = path.rfind('/');
    return (slash == std::string::npos ? std::string()
                                       : path.substr(0, slash + 1)) + "satl";
}

} // namespace

GtkWidget *terminal_new(GtkWidget *window,
                        const std::string &file,
                        const std::vector<std::string> &args,
                        bool hold_always)
{
    hold_clean_exit = hold_always;

    GtkWidget *widget = vte_terminal_new();
    VteTerminal *terminal = VTE_TERMINAL(widget);
    apply_colors(terminal);
    apply_font(terminal);

    // THE KEYBOARD IS INSTALLED BEFORE ANYTHING CAN FAIL, and the order is
    // load-bearing rather than tidy: both failure paths below reach hold_open,
    // hold_open now calls keys.cpp's close_on_any_key(), and a flag set on a
    // controller that has not been added yet is a window that says "press any
    // key" and answers none of them.
    install_key_bindings(window, widget, interpreter_is_running);

    g_signal_connect(terminal, "child-exited",
                     G_CALLBACK(on_child_exited), window);

    const std::string interpreter = interpreter_beside_me();
    if (interpreter.empty()) {
        fprintf(stderr, "satl-term: cannot find myself on disk, so I cannot "
                        "find satl beside me\n");
        say(terminal, "[satl-term] cannot find satl beside me.");
        hold_open(terminal);
        return widget;
    }

    std::vector<std::string> child{ interpreter };
    if (file.empty()) {
        child.push_back("--repl");
    } else {
        child.push_back("--run");
        child.push_back(file);
        child.insert(child.end(), args.begin(), args.end());
    }

    std::vector<char *> child_argv;
    child_argv.reserve(child.size() + 1);
    for (std::string &word : child)
        child_argv.push_back(word.data());
    child_argv.push_back(nullptr);

    // The child is told it is in THIS window, and that is the whole content of
    // the message: the interpreter's prompt picks colors against a background,
    // and this process is the only one that knows what the background is --
    // apply_colors above painted it. Without this, satl has to guess, and a
    // guess of white-on-light-blue is an invisible prompt.
    //
    // The environment is COPIED rather than replaced, so the child still
    // inherits everything else it had -- PATH, HOME, TERM and the rest.
    gchar **child_env = g_environ_setenv(g_get_environ(), "SATL_TERM", "1", TRUE);

    // See child_alive's note: true from the moment the spawn is ASKED FOR, so
    // that a Ctrl-C arriving while the interpreter is still starting is the
    // child's key and not the window's.
    child_alive = true;

    vte_terminal_spawn_async(terminal,
                             VTE_PTY_DEFAULT,
                             nullptr,          // inherit the working directory
                             child_argv.data(),
                             child_env,
                             G_SPAWN_DEFAULT,
                             nullptr, nullptr, nullptr,   // no child setup
                             -1,               // default timeout
                             nullptr,          // no cancellable
                             on_spawn_done,
                             window);

    // spawn_async copies what it needs; this side owns the array.
    g_strfreev(child_env);

    return widget;
}

} // namespace satellite
