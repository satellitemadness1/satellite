// satellite 004 (PLAN M0.5, 2026-09-17): ported from 003 revision 07's
// src/programs/satl-term/, nearly as-is. What changed here: --version prints the
// title lines from 004's rows (satellite/version/title_lines.hpp); every start
// shows the start-up block on stderr, as satl's does; a usage error exits 23
// command_line_not_understood where 003's was EXIT_USAGE 2, which is 004's
// display_error; --size refuses a number no window can be given (99999999999x1
// was cut to an int). The milestones, sections and src/ paths named below are 003's.
//
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

#include "child.hpp"
#include "menu.hpp"
#include "tabs.hpp"
#include "../satellite/machine/machine_codes.hpp"
#include "../satellite/machine/shown.hpp"
#include "../satellite/version/title_lines.hpp"

#include <climits>
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
    bool sized = false;  // --size was given; otherwise fit_cells() sizes it
};

// 120 COLUMNS BY 48 ROWS -- the author's ask of 2026-09-12, "the size of
// satellite.directory.list()". Measured that day at the prompt: the table is 89
// to 101 columns wide in the tree, its src/ and example/ and in $HOME, and
// $HOME's is 44 rows plus its header -- so 120 leaves room for a longer name
// and 48 holds the listing and the prompt under it. 800x600 was 88 by 27 in
// IBM Plex Mono 11, measured the same day on a headless mutter, and wrapped
// every one of them.
constexpr long kColumns = 120;
constexpr long kRows = 48;

// THE WINDOW IS SIZED IN CELLS BY MEASURING, NOT BY ASKING. The terminal sits in
// a GtkScrolledWindow, whose natural size is next to nothing, so a window left
// to its children's natural size opened at 46x73 pixels -- 4 by 2 cells --
// which is what the first attempt at this did. Instead the window opens at its
// pixel default, and on the first frame the terminal has cells this reads
// what one cell measures and how many fit, and resizes by the difference.
// Adding whole cells to an allocation adds exactly that many columns, whatever
// padding and chrome (the menu, the tab strip) the rest of the window holds, so
// the answer is exact rather than estimated. set_default_size and not a size
// request: a request is a MINIMUM, and a window nobody can shrink is not a
// default. Once, then the callback removes itself.
gboolean fit_cells(GtkWidget *widget, GdkFrameClock *, gpointer data)
{
    VteTerminal *terminal = VTE_TERMINAL(widget);
    GtkWidget *window = GTK_WIDGET(data);
    const long columns = vte_terminal_get_column_count(terminal);
    const long rows = vte_terminal_get_row_count(terminal);
    const long cell_width = vte_terminal_get_char_width(terminal);
    const long cell_height = vte_terminal_get_char_height(terminal);
    const int width = gtk_widget_get_width(window);
    const int height = gtk_widget_get_height(window);
    if (columns <= 0 || rows <= 0 || cell_width <= 0 || cell_height <= 0 ||
        width <= 0 || height <= 0 || gtk_widget_get_width(widget) <= 0)
        return G_SOURCE_CONTINUE;
    gtk_window_set_default_size(GTK_WINDOW(window),
                                width + (int)((kColumns - columns) * cell_width),
                                height + (int)((kRows - rows) * cell_height));
    return G_SOURCE_REMOVE;
}

WindowRequest requested;
std::string child_file;
std::vector<std::string> child_args;
bool hold_always = false;
int child_niceness = 19;

// "800x600" -- the spelling a person types, not the two numbers the language
// passes. Both halves must parse and both must be positive; a partly-parsed
// size is refused rather than half-applied, because a window 800 wide and
// nonsense high is not what anybody asked for.
//
// DIGITS ONLY, AND NO MORE THAN AN int HOLDS (004). strtol took " 800", "+800"
// and 99999999999, and the last was cast to an int and became a width nobody
// typed. GTK takes an int, so a larger number is refused rather than cut.
long whole_pixels(const std::string &digits)
{
    if (digits.empty() || digits.size() > 10 || digits.find_first_not_of("0123456789") != std::string::npos)
        return -1;
    const long n = strtol(digits.c_str(), nullptr, 10);
    return n > INT_MAX ? -1 : n;
}

bool parse_size(const std::string &text, WindowRequest &into)
{
    const size_t by = text.find('x');
    if (by == std::string::npos || by == 0 || by + 1 == text.size())
        return false;

    const long w = whole_pixels(text.substr(0, by));
    if (w <= 0)
        return false;

    const long h = whole_pixels(text.substr(by + 1));
    if (h <= 0)
        return false;

    into.width = (int)w;
    into.height = (int)h;
    into.sized = true;
    return true;
}

void activate(GtkApplication *app, gpointer)
{
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), requested.title.c_str());
    // --size IS PIXELS, for M24's `satellite.window.console.new("title", 800,
    // 600)`, and is taken as given. Without it fit_cells() below turns the
    // pixel default into kColumns by kRows once the terminal has cells.
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
    if (!requested.sized)
        gtk_widget_add_tick_callback(satellite::tabs_terminal_in_front(),
                                     fit_cells, window, nullptr);

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
           "       --version                      the version, revision and build\n"
           "       --help                         this\n"
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
    // shown(): a word the user typed may hold ESC or BEL, and it is quoted back
    // as text, never as bytes the terminal would obey (DESIGN §9).
    fprintf(stderr, "satl-term: %s\n", satellite004::shown(complaint).c_str());
    fputs(usage_text().c_str(), stderr);
    return satellite004::command_line_not_understood;
}

// Whether satellite_config.hpp says to show the start-up block, the same row
// satl reads. Missing means true, as it does for satl (arguments.cpp).
bool startup_display()
{
    for (const satellite_argument_row &row : return_arguments_vector())
        if (row.name == "arguments.startup_display" && row.is_flag)
            return row.flag;
    return true;
}

// The title lines, a rule of 63 dashes and an empty line: satl's start-up block.
std::string startup_block()
{
    return satellite004::title_lines_from_config() + std::string(63, '-') + "\n\n";
}

} // namespace

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv + (argc > 0 ? 1 : 0), argv + argc);

    // Answered BEFORE GTK is touched, so that asking the terminal what it is
    // needs no display: `satl-term --version` works over ssh and in a package
    // build the same way `satl --version` does.
    //
    // 004: THE WHOLE COMMAND LINE, as satl's --version and --help are, and a
    // refused write is reported rather than exiting 0 over nothing printed.
    if (!args.empty() && (args[0] == "--version" || args[0] == "-V" || args[0] == "-h" || args[0] == "--help")) {
        if (args.size() > 1)
            return usage_error(args[0] + " takes no other words");
        const std::string text = args[0] == "--version" || args[0] == "-V"
                                     ? satellite004::title_lines_from_config()
                                     : startup_block() + usage_text();
        if (fputs(text.c_str(), stdout) < 0 || fflush(stdout) != 0) {
            fprintf(stderr, "satl-term: the output refused the lines\n");
            return satellite004::display_error;
        }
        return satellite004::success;
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

            // An optional minus, then digits only, as --size takes digits only (004).
            const std::string digits = args[1].rfind('-', 0) == 0 ? args[1].substr(1) : args[1];
            const long n = digits.empty() || digits.size() > 2 ||
                                   digits.find_first_not_of("0123456789") != std::string::npos
                               ? 99 : strtol(args[1].c_str(), nullptr, 10);
            if (n < -20 || n > 19)
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

    // AN EMPTY FILE NAME IS REFUSED (004). child.cpp reads an empty file as "the
    // prompt", so `satl-term "" words` opened a prompt and dropped the words.
    if (!args.empty() && args[0].empty())
        return usage_error("the file name is empty");

    if (!args.empty()) {
        child_file = args[0];
        child_args.assign(args.begin() + 1, args.end());
    }

    // THE START-UP BLOCK, as every start of satl shows it (PLAN M0.5): on
    // stderr, for the shell that started the window. The tab shows the block of
    // the satl it runs, which is the one that matters when the two differ.
    if (startup_display())
        fputs(startup_block().c_str(), stderr);

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
