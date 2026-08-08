// satl-term — the GTK4 + VTE window that hosts the interpreter.
//
// A SEPARATE BINARY from `satl`, and the split is measured rather than
// tidy-minded. Linking the interpreter against gtk4 and vte pulls 119 shared
// objects — pango, harfbuzz, cairo, gdk-pixbuf and a hundred more — and the
// dynamic linker loads every one of them before main() runs, on every
// invocation, including `satl --run` which touches none of them.
//
//     satl --run hello.satl, linked against gtk4        25.9 ms
//     the same interpreter, not linked against gtk4      2.5 ms
//     a bare int main(){return 0;}                       2.2 ms
//
// So 23.4 of those 25.9 ms were the loader, and the interpreter's own share of
// hello world is 0.3 ms. Splitting the binary is what makes that visible: it
// takes satellite's startup from 1.3x SLOWER than CPython to 7.9x faster.
//
// §9's two-process architecture is what makes the split nearly free. The window
// never interpreted anything anyway — it spawns a child into the terminal's PTY
// and renders its bytes — so the only change is which binary it spawns: the
// sibling `satl` rather than itself.

#include <gtk/gtk.h>
#include <vte/vte.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

// ---------------------------------------------------------------------------
// Window side (GTK4 + VTE)
// ---------------------------------------------------------------------------

// What the terminal child should run. An empty file means the REPL.
static std::string g_child_file;
static std::vector<std::string> g_child_args;

// The window's palette. Black is the default foreground, so everything that
// arrives without an escape sequence -- what the user types, program output,
// error reports -- is black on the light blue field. The prompt overrides it
// per part with COLOR_USER / COLOR_CWD.
static void apply_colors(VteTerminal *terminal)
{
    GdkRGBA background, foreground;
    gdk_rgba_parse(&background, "#90D5FF");
    gdk_rgba_parse(&foreground, "#000000");

    // Palette left null: only the default pair is being set, and the 16 ANSI
    // indices keep VTE's own values.
    vte_terminal_set_colors(terminal, &foreground, &background, nullptr, 0);
}

// IBM Plex Mono, and the comma is doing real work: a Pango family field takes
// an ordered list, so an absent Plex falls through to whatever the system calls
// monospace rather than to whatever fontconfig picks when asked for a family
// that does not exist -- which can be proportional, and a proportional terminal
// is unusable rather than merely different.
//
// The font is NOT read from satellite.library the way system.max_depth is,
// because satl-term links window.o alone and nothing of the runtime; pulling
// the Library in to make one string configurable would drag the interpreter
// into a binary whose whole point is that the interpreter is not in it (DESIGN
// section 9). It is a Recommends on fonts-ibm-plex and a fallback instead.
static void apply_font(VteTerminal *terminal)
{
    PangoFontDescription *font =
        pango_font_description_from_string("IBM Plex Mono,monospace 11");
    vte_terminal_set_font(terminal, font);
    pango_font_description_free(font);
}

static void on_child_exited(VteTerminal *, int, gpointer window)
{
    gtk_window_destroy(GTK_WINDOW(window));
}

static void on_spawn_done(VteTerminal *, GPid, GError *error, gpointer window)
{
    if (error) {
        fprintf(stderr, "satl-term: failed to spawn the interpreter: %s\n",
                error->message);
        gtk_window_destroy(GTK_WINDOW(window));
    }
}

static void activate(GtkApplication *app, gpointer)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "satellite");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    GtkWidget *terminal = vte_terminal_new();
    apply_colors(VTE_TERMINAL(terminal));
    apply_font(VTE_TERMINAL(terminal));

    GtkWidget *scrolled = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), terminal);
    gtk_window_set_child(GTK_WINDOW(window), scrolled);

    g_signal_connect(terminal, "child-exited",
                     G_CALLBACK(on_child_exited), window);

    // Spawn the INTERPRETER into the terminal's PTY — the sibling `satl`
    // next to this binary, not this binary again. That is the whole of the
    // split: the window keeps its 119 shared objects and the interpreter keeps
    // its 6, and neither pays for the other.
    char self[4096];
    ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n < 0) {
        fprintf(stderr, "satl-term: readlink /proc/self/exe failed\n");
        gtk_window_destroy(GTK_WINDOW(window));
        return;
    }
    self[n] = '\0';

    std::string interpreter(self);
    const size_t slash = interpreter.rfind('/');
    interpreter = (slash == std::string::npos ? std::string()
                                              : interpreter.substr(0, slash + 1)) +
                  "satl";

    std::vector<std::string> child;
    child.push_back(interpreter);
    if (g_child_file.empty()) {
        child.push_back("--repl");
    } else {
        child.push_back("--run");
        child.push_back(g_child_file);
        child.insert(child.end(), g_child_args.begin(), g_child_args.end());
    }

    std::vector<char *> argv;
    argv.reserve(child.size() + 1);
    for (std::string &word : child)
        argv.push_back(word.data());
    argv.push_back(nullptr);

    vte_terminal_spawn_async(VTE_TERMINAL(terminal),
                             VTE_PTY_DEFAULT,
                             nullptr,        // inherit cwd
                             argv.data(),
                             nullptr,        // inherit environment
                             G_SPAWN_DEFAULT,
                             nullptr, nullptr, nullptr,  // no child setup
                             -1,             // default timeout
                             nullptr,        // no cancellable
                             on_spawn_done,
                             window);

    gtk_window_present(GTK_WINDOW(window));
}

static void usage()
{
    fprintf(stderr,
            "usage: satl-term                              window, spawns the repl\n"
            "       satl-term <file> [args]               window running <file>\n"
            "\n"
            "the interpreter itself is `satl`, and it does not link gtk.\n");
}

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);

    if (args.size() > 1 && (args[1] == "-h" || args[1] == "--help")) {
        usage();
        return 2;
    }

    if (args.size() > 1) {
        g_child_file = args[1];
        g_child_args.assign(args.begin() + 2, args.end());
    }

    // Never hand the user's argv to GApplication — it treats the extra words
    // as files to open. It gets the program name and nothing else.
    char *gtk_argv[] = { argv[0], nullptr };

    // The application id is also the program name, because prgname is what GTK
    // puts on the toplevel — the Wayland app_id, and the X11 WM_CLASS instance
    // — and a desktop shell associates a window with its launcher by matching
    // that string against an installed .desktop file name. Left at argv[0] the
    // window announces itself as "satl-term" while the entry installed beside
    // it is org.satellite.terminal.desktop, and a window that matches no entry
    // has no icon and cannot be pinned. GApplication would otherwise set
    // prgname from argv[0] itself, so this has to come first.
    g_set_prgname("org.satellite.terminal");

    // NON_UNIQUE rather than the single-instance default, and the reason is
    // the file argument. DESIGN.md §9 spells out what the default does: a
    // second process registering the same id becomes a remote and sends
    // `activate` to the primary. The file is parsed into the REMOTE's
    // g_child_file and activate then runs in the PRIMARY, reading its own copy
    // — empty — so `satl-term prog.satl` with a window already open opened a
    // second REPL and silently dropped the program it was asked to run. One
    // process per invocation also gives the spawned interpreter the
    // directory the user typed the command in, which is what makes a relative
    // path resolve. §9's rule that the interpreter child must never construct
    // a GtkApplication of its own is untouched by this: it still must not.
    GtkApplication *app = gtk_application_new("org.satellite.terminal",
                                              G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
    int status = g_application_run(G_APPLICATION(app), 1, gtk_argv);
    g_object_unref(app);
    return status;
}
