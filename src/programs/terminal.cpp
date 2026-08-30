// The VTE terminal widget and the interpreter it hosts.
//
// THE WINDOW NEVER INTERPRETS ANYTHING. This file spawns the sibling `satl`
// into a PTY and renders the bytes that come back, which is what makes the
// two-binary split nearly free: the two-process shape was already there, and
// the only question the split answers is which binary gets spawned. PLAN.md
// M1.5, and DESIGN.md §10.3 for why the window's thread is not the user's
// problem.

#include "programs/terminal.hpp"
#include "programs/opening.hpp"

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

gboolean on_key(GtkEventControllerKey *, guint, guint, GdkModifierType,
                gpointer window)
{
    gtk_window_destroy(GTK_WINDOW(window));
    return TRUE;
}

// Any key closes a window that is being held.
//
// ON THE WINDOW AND IN THE CAPTURE PHASE, not on the terminal. The terminal
// keeps keyboard focus after its child is gone and would otherwise swallow the
// keystroke; capture runs before the focused widget sees it. A held window
// still has its close button, so this is a convenience -- but a message that
// says "press any key" and then ignores one is worse than no message.
void hold_open(VteTerminal *terminal, GtkWidget *window)
{
    say(terminal, "[satl-term] press any key to close this window.");

    GtkEventController *keys = gtk_event_controller_key_new();
    gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
    g_signal_connect(keys, "key-pressed", G_CALLBACK(on_key), window);
    gtk_widget_add_controller(window, keys);
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

    hold_open(terminal, window);
}

// NOT a silent destroy. A window that closes the instant it opens tells the
// user nothing, and the thing that just failed is the one thing this binary
// exists to do. It goes to stderr as well, because the person who typed
// `satl-term` in a shell is looking there.
void on_spawn_done(VteTerminal *terminal, GPid, GError *error, gpointer user_data)
{
    if (!error)
        return;

    GtkWidget *window = GTK_WIDGET(user_data);

    fprintf(stderr, "satl-term: failed to spawn the interpreter: %s\n",
            error->message);
    say(terminal, std::string("[satl-term] could not start the interpreter: ") +
                      error->message);
    hold_open(terminal, window);
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

    g_signal_connect(terminal, "child-exited",
                     G_CALLBACK(on_child_exited), window);

    const std::string interpreter = interpreter_beside_me();
    if (interpreter.empty()) {
        fprintf(stderr, "satl-term: cannot find myself on disk, so I cannot "
                        "find satl beside me\n");
        say(terminal, "[satl-term] cannot find satl beside me.");
        hold_open(terminal, window);
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
