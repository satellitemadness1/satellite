// satl-term -- the GTK4 window that hosts the interpreter.
//
// A SEPARATE BINARY from `satl`, and the split is measured rather than
// tidy-minded. Linking the interpreter against gtk4 and vte pulls a hundred
// shared objects -- pango, harfbuzz, cairo, gdk-pixbuf and the rest -- and the
// dynamic linker loads every one of them before main() runs, on every
// invocation, including the ones that touch no pixels. On this machine the
// first satellite's satl-term resolves 78 against this satl's 6, and
// make_support/040-sources.mk is where that count was taken.
//
// FOURTH BINARY AND NOT A FIFTH. It links the window and nothing of the
// runtime, so it has nothing for -march to act on and is built once at the
// baseline -- like satl-cpu-level and unlike satl, which is built twice. It
// gets the haswell interpreter for free by spawning whichever `satl` the
// installer chose. PLAN.md M1.5, and §4.2's table.
//
// THIS IS THE SAME WINDOW THE LANGUAGE HANDS OUT. A satellite program asks for
// one with
//
//     satellite.variable.window my_console =
//         satellite.window.console.new("window_title", 800, 600)
//
// -- a string and two numbers -- and that is why the title and the size below
// are ARGUMENTS rather than literals. satl-term is the first caller of that
// signature and must not be a special case of it; the day M24's dlopen'd
// library calls the same three values in, nothing here should have to move.
// `satellite.window.console` and its `new` are NOT YET NUMBERED -- see the note
// in SCRATCH.md/SESSION.md; the author owns the numbering.

#include "programs/opening.hpp"
#include "programs/satl-term/child.hpp"
#include "programs/satl-term/menu.hpp"
#include "programs/satl-term/tabs.hpp"
#include "system_facts/version.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

// The three values satellite.window.console.new takes, in the same order and
// with the same meanings. Defaulted here so that `satl-term` with no arguments
// is the same call with the arguments the language will default.
struct WindowRequest {
    std::string title = "satellite";
    int width = 800;
    int height = 600;
};

WindowRequest requested;
std::string child_file;
std::vector<std::string> child_args;
bool hold_always = false;
int child_niceness = 19;

// "800x600" -- the spelling a person types, not the two numbers the language
// passes. Both halves must parse and both must be positive; a partly-parsed
// size is refused rather than half-applied, because a window 800 wide and
// nonsense high is not what anybody asked for.
bool parse_size(const std::string &text, WindowRequest &into)
{
    const size_t by = text.find('x');
    if (by == std::string::npos || by == 0 || by + 1 == text.size())
        return false;

    char *rest = nullptr;
    const long w = strtol(text.substr(0, by).c_str(), &rest, 10);
    if (*rest != '\0' || w <= 0)
        return false;

    const long h = strtol(text.substr(by + 1).c_str(), &rest, 10);
    if (*rest != '\0' || h <= 0)
        return false;

    into.width = (int)w;
    into.height = (int)h;
    return true;
}

void activate(GtkApplication *app, gpointer)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), requested.title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(window),
                                requested.width, requested.height);

    // The menu above and the terminals below. A plain box and not a header bar:
    // the title bar belongs to the desktop, which draws it with the buttons
    // this machine's user has chosen and puts requested.title in it -- and a
    // window that draws its own to hold four menu items has taken that over to
    // save a row of pixels.
    GtkWidget *stack = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(stack), satellite::menu_bar_new(window));

    GtkWidget *tabs = satellite::tabs_new(window, hold_always);
    gtk_widget_set_vexpand(tabs, TRUE);
    gtk_box_append(GTK_BOX(stack), tabs);

    gtk_window_set_child(GTK_WINDOW(window), stack);

    // THE FIRST TAB IS THE COMMAND LINE'S, and it is opened by the same call
    // File > New tab makes. A window started with a program and a window that
    // was handed one an hour later hold the same kind of tab, which is what
    // stops the menu from being a second way of doing this with its own bugs.
    satellite::tabs_open_tab(child_file, child_args);

    gtk_window_present(GTK_WINDOW(window));
}

std::string usage_text()
{
    return "usage: satl-term                      a window running the prompt\n"
           "       satl-term <file> [args]        a window running <file>\n"
           "\n"
           "       --title <text>                 the window title\n"
           "       --size <width>x<height>        the window size, in pixels\n"
           "       --hold                         keep the window after a clean exit\n"
           "       --nice <-20..19>               the interpreter's priority (default 19)\n"
           "       --version                      what this build is\n"
           "\n"
           "the interpreter itself is `satl`, and it does not link gtk.\n"
           "\n"
           "the title and the two numbers are the same three arguments a program\n"
           "passes to satellite.window.console.new(\"title\", 800, 600); this\n"
           "binary is the first caller of that signature and not a special case\n"
           "of it.\n"
           "\n"
           "a window whose interpreter FAILS is held open either way, so the\n"
           "reason stays on the screen. --hold holds a clean exit too.\n"
           "\n"
           "the interpreter is spawned at niceness 19 -- the lowest priority --\n"
           "so that a program using every core leaves the desktop answering. A\n"
           "shell alias cannot do this: a launcher runs this binary directly and\n"
           "bash never sees the line. --nice 0 asks for the ordinary priority.\n";
}

int usage_error(const std::string &complaint)
{
    fprintf(stderr, "satl-term: %s\n", complaint.c_str());
    fputs(usage_text().c_str(), stderr);
    return satellite::EXIT_USAGE;
}

} // namespace

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv + (argc > 0 ? 1 : 0), argv + argc);

    // Answered BEFORE GTK is touched, so that asking the terminal what it is
    // needs no display: `satl-term --version` works over ssh and in a package
    // build the same way `satl --version` does.
    if (!args.empty() && (args[0] == "--version" || args[0] == "-V")) {
        fputs(satellite::version_text("satl-term").c_str(), stdout);
        return satellite::EXIT_FINE;
    }
    if (!args.empty() && (args[0] == "-h" || args[0] == "--help")) {
        fputs(usage_text().c_str(), stdout);
        return satellite::EXIT_FINE;
    }

    // The window's own options come first and consume themselves, so whatever
    // is left is the file and the arguments meant for the PROGRAM. A loop
    // rather than a chain of positional tests, because --title and --size are
    // order-independent and a user should not have to learn that they are not.
    while (!args.empty() && args[0].rfind("--", 0) == 0) {
        const std::string flag = args[0];

        if (flag == "--hold") {
            hold_always = true;
            args.erase(args.begin());
        } else if (flag == "--title") {
            if (args.size() < 2)
                return usage_error("--title needs text after it");
            requested.title = args[1];
            args.erase(args.begin(), args.begin() + 2);
        } else if (flag == "--nice") {
            if (args.size() < 2)
                return usage_error("--nice needs a number after it");

            char *rest = nullptr;
            const long n = strtol(args[1].c_str(), &rest, 10);
            if (*rest != '\0' || n < -20 || n > 19)
                return usage_error("--nice wants a number from -20 to 19, and "
                                   "got " + args[1]);

            child_niceness = (int)n;
            args.erase(args.begin(), args.begin() + 2);
        } else if (flag == "--size") {
            if (args.size() < 2)
                return usage_error("--size needs <width>x<height> after it");
            if (!parse_size(args[1], requested))
                return usage_error("--size wants two positive numbers, as in "
                                   "800x600, and got " + args[1]);
            args.erase(args.begin(), args.begin() + 2);
        } else {
            // NOT swallowed as a filename. A misspelled flag that gets treated
            // as a program name produces a window complaining about a file the
            // user never mentioned, which sends them looking in the wrong place.
            return usage_error("unknown option " + flag);
        }
    }

    if (!args.empty()) {
        child_file = args[0];
        child_args.assign(args.begin() + 1, args.end());
    }

    // Told to child.cpp BEFORE the window exists, because the first tab is
    // opened from activate() and there is no moment after that where no
    // interpreter has been spawned yet.
    satellite::child_set_nice(child_niceness);

    // Never hand the user's argv to GApplication -- it treats the extra words
    // as files to open. It gets the program name and nothing else.
    char *gtk_argv[] = { argv[0], nullptr };

    // The application id is also the program name, because prgname is what GTK
    // puts on the toplevel -- the Wayland app_id, and the X11 WM_CLASS instance
    // -- and a desktop shell associates a window with its launcher by matching
    // that string against an installed .desktop file name. Left at argv[0] the
    // window announces itself as "satl-term" while the entry installed beside
    // it is org.satellite.terminal.desktop, and a window that matches no entry
    // has no icon and cannot be pinned. GApplication would otherwise set
    // prgname from argv[0] itself, so this has to come first.
    //
    // The window TITLE is a separate thing and is the user's string; this is
    // the identity the desktop files under, and it does not vary per window.
    g_set_prgname("org.satellite.terminal");

    // NON_UNIQUE rather than the single-instance default, and the reason is the
    // file argument. Under the default, a second process registering the same
    // id becomes a remote and sends `activate` to the primary -- so the file
    // would be parsed into the REMOTE's child_file while activate ran in the
    // PRIMARY, reading its own empty copy. `satl-term prog.satl` with a window
    // already open would open a second prompt and silently drop the program it
    // was asked to run. One process per invocation also gives the spawned
    // interpreter the directory the user typed the command in, which is what
    // makes a relative path resolve.
    GtkApplication *app = gtk_application_new("org.satellite.terminal",
                                              G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), nullptr);
    const int status = g_application_run(G_APPLICATION(app), 1, gtk_argv);
    g_object_unref(app);
    return status;
}
