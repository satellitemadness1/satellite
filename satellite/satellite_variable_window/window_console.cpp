// satellite/satellite_variable_window/window_console.cpp -- A CONSOLE: a window
// whose whole inside is a terminal, and what a program does to one.
// GTK_AND_NO_DEPENDENCIES.md GTK-17, the author: *"we especially need
// satellite.console to be a window with libvte"*.
//
// A CONSOLE IS A PTY, AND THAT IS THE WHOLE DESIGN. VTE draws whatever comes
// out of a pty's master and writes every key a person presses into it; the
// kernel's line discipline between the two is what makes a terminal a
// terminal -- it echoes what is typed, takes the backspaces, and hands over a
// line only when Enter has finished it. So a console is a VteTerminal with a
// VtePty, and satl holds the pty's OTHER end: `.display` is one write() to it,
// `.typed` is the desk reading a finished line off it, and nothing here
// re-implements editing the kernel has done since before GTK existed. It is
// also exactly the end satl's own stdio sits on in `satl --console`
// (console_launch.cpp), which is why the two are one widget.
//
// NOT `satellite.console.display` INTO A WINDOW. That word keeps writing to
// stdout, exactly as it always has; a console is a thing a program makes by
// name -- GTK-17's reading (2), the recommendation, and the author's own
// spelling of it on 2026-09-22: `satellite.console.new`.
//
// EVERY GTK AND VTE CALL HAPPENS INSIDE on_the_desk(), like its neighbours; a
// write() or read() on the pty is a syscall and happens where it is written,
// and the cast from a `void *` to a VteTerminal * is a cast, not a call.
// COMPILED WHERE pkg-config FINDS gtk4; the VTE half only where it found
// vte-2.91-gtk4 as well (SATELLITE_HAS_CONSOLE, make_support/047-window.mk),
// and every function's other half is at the bottom of this file, so a satl
// without VTE still lexes, checks and refuses the console words by name.
//
// FOUR HUNDRED LINES AGAINST "TRY TO BUILD FOR 300", and not split: the pty
// plumbing and the words on it are one subject, and the refusing half the stub
// rule keeps in this file is thirty of them. If it is ever cut, the seam is the
// desk's plumbing (the slave, the terminal, the reader, the teardown) against
// the words a program calls -- never a line number.

#include "satellite_window.hpp"
#include "console_scrolling.hpp"

#include "window_desk.hpp"

#include <string>

namespace satellite004 {

// THE ONE SENTENCE A satl BUILT WITHOUT VTE SAYS, wherever it says it -- here
// and in console_launch.cpp's other half.
bool without_a_console(std::string &why)
{
    why = "this satl was built without a console -- pkg-config found no vte-2.91-gtk4 when it was "
          "made (AlmaLinux/RHEL: dnf --enablerepo=crb install vte291-gtk4-devel; Debian/Ubuntu: apt "
          "install libvte-2.91-gtk4-dev), and build again";
    return false;
}

// ABOVE THE #if, because both halves ask it first: a line that is wrong on
// every build -- `.display` on a button -- is refused for what it is, not for
// what this satl was built without. A CLOSED piece is "it is closed" BEFORE
// its kind is judged, because window_is_closed is the code both callers pick
// for a null widget, and the sentence must be the one that code means.
bool a_console_that_is_open(const satellite_window &which, std::string &why)
{
    if (which.widget == nullptr) {
        why = "it is closed";
        return false;
    }
    if (which.piece != satellite_window::console) {
        why = std::string(which.piece_name()) +
              " is not a console -- satellite.console.new(\"a title\", 800, 600) makes one";
        return false;
    }
    if (!which.on_the_screen || which.terminal == nullptr || which.slave < 0) {
        why = "it is closed";
        return false;
    }
    return true;
}

// THE TWO SCROLL ROWS AS THE RUN HOLDS THEM (console_scrolling.hpp): true until config.ini or a
// line says otherwise -- structured-library.cpp sets them as satl starts, setting_writes.cpp on a
// write. Plain atomics, read on either thread.
std::atomic<bool> &console_scrolls_vertically()
{
    static std::atomic<bool> on{true};
    return on;
}

std::atomic<bool> &console_scrolls_horizontally()
{
    static std::atomic<bool> on{false};   // off unless written: wide lines wrap, and the console grows downward
    return on;
}

// satl'S OWN CONSOLE, FOR THE ROWS TO REACH (console_launch.cpp writes it once, on the interpreter's
// thread, when that console is open); null until then.
satellite_window *satls_own_console_for_scrolling = nullptr;

} // namespace satellite004

#if SATELLITE_HAS_CONSOLE

#include "console_feed.hpp"
#include "console_shadow.hpp"
#include "window_console.hpp"
#include "window_frame.hpp"

#include <glib-unix.h>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

namespace satellite004 {

void dress_the_terminal(VteTerminal *terminal)
{
    // ITS TWO COLOURS ARE THE PERSON'S (the author, 2026-10-05: "we can have a
    // setting that sets the background and the text color"): console.background
    // and console.text in config.ini, which File > Settings… writes -- and when
    // they are absent, the light blue field and "the text is still 000000", as
    // satl-term's look was (satl-term/terminal.cpp). The shadow worked out from
    // them is console_shadow.cpp's. What a program prints and what a person types
    // are one colour on one ground, the opening display with them. IBM Plex Mono
    // is the font satl carries (WIN-1),
    // and the comma is doing real work: a Pango family list falls through to
    // the system's monospace rather than to whatever fontconfig picks for a
    // name it does not have -- which can be proportional, and a proportional
    // terminal is unusable rather than merely different. ITS SIZE IS THE
    // PERSON'S (the author, 2026-09-22: "I want to make my font way smaller"):
    // config.ini's console.font_size, which File > Settings… writes.
    const GdkRGBA background = a_console_colour(kConsoleBackgroundRow, kAuthorsBackground);
    const GdkRGBA foreground = a_console_colour(kConsoleTextRow, kAuthorsText);
    vte_terminal_set_colors(terminal, &foreground, &background, nullptr, 0);
    set_console_font_points(terminal, console_font_points());
    // NO CEILING ON WHAT A CONSOLE KEEPS (the author, 2026-10-07: "so it can scroll
    // for however long that the text is"). 10000 lines until then; -1 is VTE's
    // "every line", on disk when memory would rather not.
    vte_terminal_set_scrollback_lines(terminal, -1);
}

namespace {

// WHY THE PTY COULD NOT BE MADE, carried out of the desk's lambda. Written ON
// THE DESK; read by console_new once on_the_desk() has waited.
std::string pty_trouble;

// THE CONSOLE'S OTHER END. A VtePty holds the MASTER; satl needs the SLAVE --
// the side a child's stdio would sit on. VTE's own way of opening it,
// vte_pty_child_setup(), is written for a FORKED child: it calls setsid() and
// _exit()s when that fails, which in THIS process would be satl exiting. So
// the slave is opened by hand, from the master's name. O_NOCTTY, so that
// opening it does not make it the controlling terminal: whether it becomes
// one is console_launch.cpp's decision and nobody else's.
int open_the_slave_of(VtePty *pty, std::string &why)
{
    char name[256];
    if (ptsname_r(vte_pty_get_fd(pty), name, sizeof name) != 0) {
        why = std::string("the pty has no name -- ") + std::strerror(errno);
        return -1;
    }
    const int fd = open(name, O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (fd < 0)
        why = std::string("could not open ") + name + " -- " + std::strerror(errno);
    return fd;
}

// THE CONSOLE WENT AWAY, whoever took it away. ON THE DESK'S THREAD, out of
// `destroy`, and connected BEFORE the desk's own handler (window_frame.hpp says
// why), so the handle is whole here: the reader is stopped and the pty is let
// go of -- which closes the MASTER once the terminal has let go of its own
// reference. A program's `.display` after this is refused as "it is closed",
// and one that slipped in between meets the closed master and is refused with
// EIO, rather than blocking for ever on a pty nobody drains.
//
// A PERSON CLOSING satl'S OWN CONSOLE IS HANGING UP ON THE INTERPRETER, and it
// is told so the way closing any terminal tells a program: SIGHUP. The kernel
// says it itself when the pty is satl's controlling terminal (the master
// closing hangs up the session); when it is not -- satl started from a shell,
// see console_launch.cpp -- this says it. NOT when satl closed the console
// itself because the run was over: that is `closing_on_purpose`, and nobody
// hung up.
void the_console_went_away(GtkWidget *, gpointer user_data)
{
    satellite_window *console = static_cast<satellite_window *>(user_data);
    // satl'S OWN CONSOLE IS FED STRAIGHT (console_feed.cpp): no more of that, and a flush waiting
    // on it goes on -- before the terminal is let go of.
    if (console->is_satls_own)
        console_feed_stops();
    if (console->typed_watch != 0) {
        g_source_remove(console->typed_watch);
        console->typed_watch = 0;
    }
    // THE SLAVE IS NOT CLOSED HERE -- satellite_window.hpp says why: it goes
    // with the handle, so a write the interpreter is about to make cannot
    // land in a file that reused the number. `terminal` nulled is what every
    // later call reads as "it is closed".
    if (console->pty != nullptr) {
        g_object_unref(console->pty);
        console->pty = nullptr;
    }
    console->terminal = nullptr;
    if (console->is_satls_own && !console->closing_on_purpose)
        kill(getpid(), SIGHUP);
}

// A LINE WAS FINISHED IN A PROGRAM'S CONSOLE. ON THE DESK'S THREAD, out of
// the slave being readable. THE PTY IS IN CANONICAL MODE, which is what a
// fresh pty is: a read() answers one whole line, and the slave is readable
// only once Enter has finished one -- the kernel has echoed it, taken the
// backspaces, and stripped nothing but what the person un-typed. So a readable
// slave IS a line, and this never blocks and never reads half of one. It
// travels on the event the way a key's name does, and the interpreter copies
// it onto the piece for `.typed` to answer (window_run.cpp).
gboolean a_line_was_finished(gint fd, GIOCondition condition, gpointer user_data)
{
    satellite_window *console = static_cast<satellite_window *>(user_data);
    // THE PTY WENT AWAY UNDER THE READER -- which the_console_went_away never
    // lets happen, since it removes this source first; kept for the day
    // something else closes a master.
    if ((condition & (G_IO_HUP | G_IO_ERR | G_IO_NVAL)) != 0) {
        console->typed_watch = 0;
        return G_SOURCE_REMOVE;
    }
    char bytes[4096];
    const ssize_t got = read(fd, bytes, sizeof bytes);
    if (got < 0) {
        if (errno == EINTR || errno == EAGAIN)
            return G_SOURCE_CONTINUE;
        console->typed_watch = 0;
        return G_SOURCE_REMOVE;
    }
    // CTRL-D AT THE START OF A LINE IS A read() OF NOTHING, and the pty is
    // still there: canonical mode answers 0 for VEOF, once, and is not sticky.
    // Nothing was typed, so nothing is delivered and the reader STAYS. The
    // first draft read 0 as the end and removed itself, so one reflex
    // keystroke stopped `.typed` for good with nothing said (a fresh reader,
    // 2026-09-22) -- the answer that is wrong and does not say so.
    if (got == 0)
        return G_SOURCE_CONTINUE;
    console->line_so_far.append(bytes, static_cast<std::size_t>(got));
    // A LINE IS DELIVERED WHEN IT IS FINISHED, AND ONLY THEN. Ctrl-D in the
    // middle of one makes canonical mode hand over what was typed so far with
    // no newline; that is not a line a person entered, so it waits for the
    // rest of itself. Enter ends it.
    if (console->line_so_far.back() != '\n')
        return G_SOURCE_CONTINUE;
    std::string line;
    line.swap(console->line_so_far);
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
        line.pop_back();
    if (!console->when_typed.empty())
        the_desk_saw_something(console->when_typed, console->shared_from_this(), false, line,
                               AnEvent::a_line);
    return G_SOURCE_CONTINUE;
}

// WHAT GOES IN A CONSOLE'S FRAME: the terminal, with a pty of its own. ON THE
// DESK, from frame_new, with the column that fills the window.
void a_terminal_to_type_in(satellite_window &made, GtkWidget *window, GtkWidget *column)
{
    GtkWidget *terminal = vte_terminal_new();
    dress_the_terminal(VTE_TERMINAL(terminal));
    gtk_widget_set_hexpand(terminal, TRUE);
    gtk_widget_set_vexpand(terminal, TRUE);
    // ON A GROUND, so the shadow under its text can be seen (console_shadow.cpp)
    // -- AND A SCROLLBAR DOWN ITS RIGHT SIDE (the author, 2026-10-07: "When the
    // text is wider than the console window, I need it to scroll vertically so
    // that more text can be displayed vertically, for as however big the text
    // is"). A line wider than the console wraps onto the lines below -- VTE's
    // own auto-wrap, which nothing here turns off -- the scrollback has no
    // ceiling (dress_the_terminal), and the bar moves the terminal's OWN
    // adjustment, so dragging it, the wheel and Shift+Page Up all move one view.
    GtkAdjustment *scrolls = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(terminal));
    if (scrolls == nullptr) {
        scrolls = gtk_adjustment_new(0, 0, 0, 1, 1, 1);
        gtk_scrollable_set_vadjustment(GTK_SCROLLABLE(terminal), scrolls);
    }
    // THE GROUND IN A VIEWPORT OF ITS OWN, IN A SCROLLER: the viewport is what lets the terminal be
    // WIDER than the window when sideways scrolling is on (apply_console_scrolling sets its policy
    // to the terminal's natural width, 2048 cells), and what hands it the window's width when it
    // is off. The bar beside it is the terminal's own; the scroller's bar runs along the bottom.
    GtkWidget *viewport = gtk_viewport_new(nullptr, nullptr);
    gtk_viewport_set_child(GTK_VIEWPORT(viewport), a_ground_under(terminal));
    GtkWidget *scroller = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), viewport);
    gtk_widget_set_hexpand(scroller, TRUE);
    gtk_widget_set_vexpand(scroller, TRUE);
    GtkWidget *bar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, scrolls);
    GtkWidget *across = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(across, TRUE);
    gtk_widget_set_vexpand(across, TRUE);
    gtk_box_append(GTK_BOX(across), scroller);
    gtk_box_append(GTK_BOX(across), bar);
    gtk_box_append(GTK_BOX(column), across);
    made.terminal = terminal;
    made.scroller = scroller;
    made.vertical_bar = bar;
    apply_console_scrolling(made);
    GError *trouble = nullptr;
    VtePty *pty = vte_pty_new_sync(VTE_PTY_DEFAULT, nullptr, &trouble);
    if (pty == nullptr) {
        pty_trouble = trouble != nullptr && trouble->message != nullptr ? trouble->message : "no pty";
        if (trouble != nullptr)
            g_error_free(trouble);
    } else {
        // Q-VTE-1'S STOPGAP. Built without gnutls, VTE feeds a red WARNING into
        // every terminal it makes, about scrollback spilled to disk unencrypted.
        // Whether to vendor gnutls is the author's question; until it is, the
        // line is DROPPED here. Not cleared: VTE only QUEUES it at construction,
        // and vte_terminal_reset -- this stopgap until 2026-09-22 -- cleared a
        // screen the line had not reached yet, so it arrived anyway, the first
        // line of every console (the author saw it; Save output as… proved it).
        // A terminal empties that queue when it lets go of a pty
        // (Terminal::unset_pty), and here the queue holds that line and nothing
        // else -- so it is given a spare pty first and then its own. NEVER
        // vte_terminal_set_pty(nullptr), which segfaults VTE 0.84.1
        // (Widget::set_pty takes the impl of null): tried, every console
        // died with signal 11. Remove the spare when gnutls is in --
        // vte_get_features() says +GNUTLS then.
        if (VtePty *spare = vte_pty_new_sync(VTE_PTY_DEFAULT, nullptr, nullptr)) {
            vte_terminal_set_pty(VTE_TERMINAL(terminal), spare);
            g_object_unref(spare);   // the terminal lets it go on the next line
        }
        vte_terminal_set_pty(VTE_TERMINAL(terminal), pty);
        made.pty = pty;   // OURS: the terminal took a reference of its own
    }
    g_signal_connect(window, "destroy", G_CALLBACK(the_console_went_away), &made);
    gtk_widget_grab_focus(terminal);
}

} // namespace

// HOW WIDE A CONSOLE IS WHEN IT SCROLLS SIDEWAYS (the author, 2026-10-07): a table of a hundred
// percent columns is under a thousand characters, and VTE keeps only what a line holds.
constexpr long kWideConsoleColumns = 2048;

// WHAT THE SWITCHES DO TO A CONSOLE, ON THE DESK. Up and down: its bar and its scrollback, every
// line or none. Sideways: the viewport allocates the terminal its NATURAL width, which is its
// column count, set to 2048 -- or, off, the window's width, from which VTE counts its columns as
// it always did; the scroller's bar along the bottom appears only when there is more to see.
void apply_console_scrolling(satellite_window &console)
{
    if (console.terminal == nullptr || console.scroller == nullptr)
        return;
    VteTerminal *terminal = terminal_of(console);
    const bool up_and_down = window_scrolls(console, true);
    const bool sideways = window_scrolls(console, false);
    vte_terminal_set_scrollback_lines(terminal, up_and_down ? -1 : 0);
    if (console.vertical_bar != nullptr)
        gtk_widget_set_visible(static_cast<GtkWidget *>(console.vertical_bar), up_and_down);
    GtkWidget *scroller = static_cast<GtkWidget *>(console.scroller);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroller), sideways ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER,
                                   GTK_POLICY_NEVER);
    GtkWidget *viewport = gtk_scrolled_window_get_child(GTK_SCROLLED_WINDOW(scroller));
    if (GTK_IS_VIEWPORT(viewport))
        gtk_scrollable_set_hscroll_policy(GTK_SCROLLABLE(viewport), sideways ? GTK_SCROLL_NATURAL : GTK_SCROLL_MINIMUM);
    const long rows = vte_terminal_get_row_count(terminal);
    if (sideways) {
        vte_terminal_set_size(terminal, kWideConsoleColumns, rows);
    } else {
        const long cell = vte_terminal_get_char_width(terminal);
        const int wide = gtk_widget_get_width(scroller);
        if (cell > 0 && wide > 0)
            vte_terminal_set_size(terminal, wide / cell > 0 ? wide / cell : 1, rows);
    }
}

// A ROW WAS WRITTEN: satl's own console takes it at once, on the desk. FROM THE INTERPRETER'S THREAD.
void scrolling_rows_changed()
{
    satellite_window *console = satls_own_console_for_scrolling;
    if (console == nullptr || console->widget == nullptr)
        return;
    on_the_desk([console] { apply_console_scrolling(*console); });
}

namespace {

// EVERY BYTE, HOWEVER MANY WRITES IT TAKES. A pty takes a few kilobytes at a
// time and the desk drains it as the terminal draws; this thread is never the
// desk, so the desk is free to. EINTR is a signal that arrived, not a refusal.
bool write_the_whole_of(int fd, const std::string &bytes, std::string &why)
{
    const char *at = bytes.data();
    std::size_t left = bytes.size();
    while (left > 0) {
        const ssize_t wrote = write(fd, at, left);
        if (wrote < 0) {
            if (errno == EINTR)
                continue;
            why = std::string("the console refused it -- ") + std::strerror(errno);
            return false;
        }
        at += wrote;
        left -= static_cast<std::size_t>(wrote);
    }
    return true;
}

} // namespace

WindowHandle console_new(const std::string &title, unsigned long long int width,
                         unsigned long long int height, std::string &why)
{
    WindowHandle made = frame_new(satellite_window::console, title, width, height, why, a_terminal_to_type_in);
    if (made == nullptr)
        return nullptr;
    std::string trouble;
    if (made->pty == nullptr)
        trouble = "the console could not get a pty -- " + pty_trouble;
    else
        made->slave = open_the_slave_of(static_cast<VtePty *>(made->pty), trouble);
    if (made->slave < 0) {
        // A FRAME WITH NO PTY IS NOT A CONSOLE. Taken down before anybody has
        // seen it, and the refusal carries the reason.
        std::string ignored;
        window_close(*made, ignored);
        why = trouble;
        return nullptr;
    }
    return made;
}

bool console_display(satellite_window &which, const std::string &line, std::string &why)
{
    if (!a_console_that_is_open(which, why))
        return false;
    // ONE LINE, ENDED, as satellite.console.display ends one. The '\n' becomes
    // CR LF on the way through the pty (OPOST and ONLCR are a fresh pty's own),
    // which is what the terminal wants; nothing here has to know that.
    return write_the_whole_of(which.slave, line + '\n', why);
}

bool console_typed(satellite_window &which, const std::string &capsule, std::string &why)
{
    if (!a_console_that_is_open(which, why))
        return false;
    satellite_window *raw = &which;
    bool closed = false;
    on_the_desk([raw, &capsule, &closed] {
        // ASKED AGAIN ON THE DESK, where the answer cannot change under it: a
        // person may have closed the console between the check above and this
        // parcel, and the_console_went_away -- on this thread -- has nulled
        // the terminal and removed any reader. A watch added now would outlive
        // the window and read a closed fd, or a file the program opened next.
        if (raw->terminal == nullptr) {
            closed = true;
            return;
        }
        raw->when_typed = capsule;
        // THE READER IS ADDED ONCE, on the desk's own context -- which is the
        // default one, the desk parks in g_main_loop_run there -- so a line
        // arrives on the desk's thread and nowhere else. A second `.typed`
        // replaces the name and keeps the reader.
        if (raw->typed_watch == 0)
            raw->typed_watch = g_unix_fd_add(raw->slave, G_IO_IN, a_line_was_finished, raw);
    });
    if (closed) {
        why = "it is closed";
        return false;
    }
    return true;
}

bool console_clear(satellite_window &which, std::string &why)
{
    if (!a_console_that_is_open(which, why))
        return false;
    // HOME, CLEAR THE SCREEN, CLEAR THE SCROLLBACK: the three sequences a
    // `clear` types. Written to the pty and not done to the widget, so it
    // lands AFTER whatever `.display` has already sent and not before it.
    return write_the_whole_of(which.slave, "\033[H\033[2J\033[3J", why);
}

bool console_home(satellite_window &which, std::string &why)
{
    if (!a_console_that_is_open(which, why))
        return false;
    return write_the_whole_of(which.slave, "\033[H", why);
}

bool console_cells_of(satellite_window &which, bool the_rows, long long int &out, std::string &why)
{
    if (!a_console_that_is_open(which, why))
        return false;
    VteTerminal *terminal = terminal_of(which);
    long got = 0;
    on_the_desk([terminal, the_rows, &got] {
        got = the_rows ? vte_terminal_get_row_count(terminal) : vte_terminal_get_column_count(terminal);
    });
    out = got;
    return true;
}

bool console_set_colour(satellite_window &which, const std::string &colour, bool behind, std::string &why)
{
    if (!a_console_that_is_open(which, why))
        return false;
    VteTerminal *terminal = terminal_of(which);
    bool read = false;
    on_the_desk([terminal, &colour, behind, &read] {
        GdkRGBA rgba;
        read = gdk_rgba_parse(&rgba, colour.c_str()) != FALSE;
        if (!read)
            return;
        if (behind)
            vte_terminal_set_color_background(terminal, &rgba);
        else
            vte_terminal_set_color_foreground(terminal, &rgba);
    });
    if (!read) {
        // GDK COULD NOT READ IT, and unlike a stylesheet it says so: nothing
        // was changed, and the console still wears what it wore.
        why = "GTK could not read \"" + colour + "\" as a colour, so nothing was changed";
        return false;
    }
    return true;
}

bool console_set_font(satellite_window &which, const std::string &face, long long int size, std::string &why)
{
    if (!a_console_that_is_open(which, why))
        return false;
    VteTerminal *terminal = terminal_of(which);
    // PANGO'S OWN SPELLING, WITH "px", so the size is the same pixels `.font`
    // means on every other piece -- GTK-10's stylesheet says px too.
    const std::string wanted = face + " " + std::to_string(size) + "px";
    on_the_desk([terminal, &wanted] {
        PangoFontDescription *font = pango_font_description_from_string(wanted.c_str());
        vte_terminal_set_font(terminal, font);
        pango_font_description_free(font);
    });
    return true;
}

} // namespace satellite004

#else

// A satl WITH GTK AND NO VTE: every console word lexes, checks, and refuses
// here by name, with the package to install -- 047-window.mk's oldest rule,
// one library further down.
namespace satellite004 {

// A BAD SIZE IS THE PROGRAM'S ON EVERY BUILD, so it is refused first, in
// frame_new's own words -- window_calls.cpp picks the code from the size.
WindowHandle console_new(const std::string &, unsigned long long int width, unsigned long long int height,
                         std::string &why)
{
    if (width == 0 || height == 0)
        why = "a window's width and height must both be more than 0";
    else if (width > 32767 || height > 32767)
        why = "a window's width and height must each be 32767 or less";
    else
        without_a_console(why);
    return nullptr;
}
// AND A PIECE THAT IS NOT A CONSOLE IS REFUSED FOR THAT, on every build, before
// the build is blamed: a_console_that_is_open is above the #if for this.
bool console_display(satellite_window &which, const std::string &, std::string &why)
{
    return a_console_that_is_open(which, why) && without_a_console(why);
}
bool console_typed(satellite_window &which, const std::string &, std::string &why)
{
    return a_console_that_is_open(which, why) && without_a_console(why);
}
bool console_clear(satellite_window &which, std::string &why)
{
    return a_console_that_is_open(which, why) && without_a_console(why);
}
bool console_home(satellite_window &which, std::string &why)
{
    return a_console_that_is_open(which, why) && without_a_console(why);
}
bool console_cells_of(satellite_window &which, bool, long long int &, std::string &why)
{
    return a_console_that_is_open(which, why) && without_a_console(why);
}
bool console_set_colour(satellite_window &which, const std::string &, bool, std::string &why)
{
    return a_console_that_is_open(which, why) && without_a_console(why);
}
bool console_set_font(satellite_window &which, const std::string &, long long int, std::string &why)
{
    return a_console_that_is_open(which, why) && without_a_console(why);
}

} // namespace satellite004

namespace satellite004 {
// THE OLD no-VTE BRANCH, kept only because the #if above it still exists: satellite is always built
// with GTK and VTE and runs only with a monitor (the author, 2026-10-05 and 2026-10-07).
void apply_console_scrolling(satellite_window &) {}
void scrolling_rows_changed() {}
} // namespace satellite004

#endif
