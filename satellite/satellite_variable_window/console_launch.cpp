// satellite/satellite_variable_window/console_launch.cpp -- THE CONSOLE satl
// LAUNCHES FOR ITSELF: `satl --console [file]`. GTK_AND_NO_DEPENDENCIES.md
// GTK-17, the author on 2026-09-22: *"this will finally allow satl to start a
// satellite.console.new"*.
//
// THE SAME WINDOW A PROGRAM GETS, AND THEN satl'S OWN STDIN, STDOUT AND STDERR
// ARE MOVED ONTO ITS PTY. In THIS process -- no second binary, no exec, no
// fork -- which is the whole difference from 003's handover to satl-term, and
// why the two things WIN-9 said a handover loses are not lost: the exit status
// is this process's own, and stdout is the pty, which is the window. A .satl
// double-clicked, or `satl --console` from a launcher with nowhere to print,
// puts what the program prints on a screen, and exits with what the program
// stopped on.
//
// AND IT OPENS ON ITS OWN WHEN NOTHING GAVE satl A CONSOLE -- WIN-9, ruled by
// the author on 2026-09-22: *"satl has to, when it's not ran in a console, take
// you to it's prompt"*, and *"we are getting rid of satl-term and replacing it
// with something built in to the satl exe"*. `--console` asks for it outright;
// window_run.cpp's nobody_gave_satl_a_console() is when satl asks for itself.
// satl-term is gone, and its File menu, its keys, its size, its look, its
// niceness and its end-of-run policy are here.
//
// WHAT HAPPENS AT THE END IS satl-term'S POLICY, PORTED: a file that finished
// cleanly closes the console at once (a window that outlived every clean run
// would be a window nobody asked to keep); a run that STOPPED holds it with
// the code and its name on the last line, until a person presses a key,
// because the console is holding the only copy of the reason; and the prompt
// holds too, because a screen full of what a person typed is theirs.
//
// COMPILED WHERE pkg-config FINDS gtk4; the VTE half only with vte-2.91-gtk4,
// and the other half is at the bottom, as in window_console.cpp.

#include "satellite_window.hpp"

#include "window_desk.hpp"

#include <string>

#if SATELLITE_HAS_CONSOLE

#include "window_console.hpp"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

namespace satellite004 {
namespace {

// THE DESK MUST NEVER WRITE TO THE PTY, AND THIS IS WHAT KEEPS IT OFF. Found
// by a fresh reader on 2026-09-22 and reproduced on the binary before the fix:
// the pty's master is drained by VTE on the DESK'S thread and by nobody else,
// and a pty holds about twelve kilobytes. A program that prints faster than
// the terminal draws fills it; the interpreter's write() waits for room, which
// is ordinary flow control; then the desk prints ONE line to stderr -- a
// Gtk-CRITICAL, a Gtk-WARNING, GDK_DEBUG's own chatter -- and blocks on the
// same buffer, which only it can empty. Neither thread ever wakes, and nothing
// is printed: the worst defect this design can have.
//
// EVERYTHING THE DESK PRINTS GOES THROUGH THE C STREAMS -- GLib's default log
// writer is fputs(stderr), GDK's debug output is vfprintf(stderr) -- and
// EVERYTHING THE INTERPRETER PRINTS GOES THROUGH std::cout AND std::cerr, which
// since main()'s sync_with_stdio(false) write to the DESCRIPTORS 1 and 2
// directly. So the descriptors move to the pty and the C streams stay on what
// satl was started with: a person's shell, or a launcher's journal, which is
// where a GTK application's warnings go anyway. glibc's stdout and stderr are
// plain variables and may be reassigned. Called BEFORE the dup2, so what is
// kept is the original; with an original that is closed -- a launcher may do
// that -- the stream is /dev/null, and a warning is lost rather than a
// deadlock kept.
FILE *a_stream_kept_on(int original)
{
    int kept = fcntl(original, F_DUPFD_CLOEXEC, 0);
    if (kept < 0)
        kept = open("/dev/null", O_WRONLY | O_CLOEXEC);
    if (kept < 0)
        return nullptr;
    FILE *stream = fdopen(kept, "w");
    if (stream == nullptr)
        close(kept);
    return stream;
}

void keep_the_c_streams_off_the_pty()
{
    std::fflush(stdout);
    std::fflush(stderr);
    if (FILE *out = a_stream_kept_on(STDOUT_FILENO)) {
        setvbuf(out, nullptr, _IOLBF, 0);
        stdout = out;
    }
    if (FILE *err = a_stream_kept_on(STDERR_FILENO)) {
        setvbuf(err, nullptr, _IONBF, 0);
        stderr = err;
    }
}

// THE ONE CONSOLE THAT IS satl'S OWN, held here so main() can find it after
// run_satl's locals are gone. A handle and not a pointer, so that the desk
// letting go of a closed window leaves something to ask whether it is there.
WindowHandle the_interpreters_console;

// 120 COLUMNS BY 48 ROWS -- satl-term's size, the author's ask of 2026-09-12
// ("the size of satellite.directory.list()") -- reached the way satl-term
// reaches it (satl-term/window.cpp): the window opens at a pixel size, and on
// the first frame the terminal has cells this reads what one cell measures and
// how many fit, and resizes the window by the difference. Adding whole cells
// to an allocation adds exactly that many columns, whatever chrome the frame
// holds, so it is exact rather than estimated. Once; then it removes itself.
//
// 133 SINCE 2026-09-24, when the listing gained its size column after type: the
// widest size, "1023.999 kb", and the two spaces after it are 13 cells, so a name
// keeps the room it had at 120. 150 SINCE 2026-09-25, for its files and sub
// columns: "files" (or a count to 99999) and two spaces are 7, and "(999999)" (a
// text file's lines, or a count to 99999999) and two spaces are 10.
constexpr long kColumns = 150;
constexpr long kRows = 48;

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
    if (columns <= 0 || rows <= 0 || cell_width <= 0 || cell_height <= 0 || width <= 0 || height <= 0 ||
        gtk_widget_get_width(widget) <= 0)
        return G_SOURCE_CONTINUE;
    gtk_window_set_default_size(GTK_WINDOW(window), width + static_cast<int>((kColumns - columns) * cell_width),
                                height + static_cast<int>((kRows - rows) * cell_height));
    return G_SOURCE_REMOVE;
}

// ---------------------------------------------------------------------------
// THE KEYBOARD, satl-term'S RULES PORTED (satl-term/keys.cpp, removed 2026-09-22
// when satl became the one application). ONE CONTROLLER, on the window, in the
// capture phase, so it sees a key before the terminal does -- and the rule the
// whole of it is built on, in satl-term's words: THE KEY IS ONLY EVER TAKEN
// WHEN THERE IS NOBODY TO GIVE IT TO.
//
//   Ctrl-V               pastes, always: somebody answering a program with a
//                        path on their clipboard should not have to type it.
//                        The byte it replaces, 0x16, nothing here asks for.
//   Ctrl-C, running      the program's. From a launcher the pty is satl's
//                        controlling terminal and the kernel makes it SIGINT;
//                        from a shell it is not, and n_tty would DROP the byte
//                        with no group to signal, so this sends SIGINT itself
//                        when the pty is cooked -- and lets the byte through
//                        when the prompt has it raw, which wants 0x03.
//   Ctrl-C, held         copies what is highlighted; with nothing highlighted
//                        it closes, as any key does.
//   any key, held        closes -- but NOT a modifier on its own, which is the
//                        first half of every Ctrl-C and would close the window
//                        a moment before the C arrived (satl-term's trap).
//
// Shift is allowed through with Control, as satl-term allowed it; Alt and Super
// are the window manager's. There is deliberately no Ctrl-Shift-C that copies
// while a program runs: the author asked for Ctrl-C to stop a run, and a second
// spelling doing the opposite is a decision satl-term already made.
// ---------------------------------------------------------------------------

// WRITTEN ONCE ON THE INTERPRETER'S THREAD, before the parcel that installs the
// controller -- which is what orders it for the desk -- and read on the desk.
bool the_pty_is_satls_terminal = false;

// WRITTEN AND READ ON THE DESK ONLY: set by the parcel that puts up the hold.
bool the_console_is_held = false;

bool is_only_a_modifier(guint keyval)
{
    switch (keyval) {
    case GDK_KEY_Control_L: case GDK_KEY_Control_R: case GDK_KEY_Shift_L: case GDK_KEY_Shift_R:
    case GDK_KEY_Alt_L: case GDK_KEY_Alt_R: case GDK_KEY_Super_L: case GDK_KEY_Super_R:
    case GDK_KEY_Meta_L: case GDK_KEY_Meta_R: case GDK_KEY_ISO_Level3_Shift:
    case GDK_KEY_Caps_Lock: case GDK_KEY_Num_Lock:
        return true;
    default:
        return false;
    }
}

// THE HOLD IS OVER, ON THE DESK: closed on purpose, so it is not read as a
// person hanging up on the interpreter.
void close_it_on_purpose(satellite_window &console)
{
    console.closing_on_purpose = true;
    gtk_window_destroy(GTK_WINDOW(static_cast<GtkWidget *>(console.widget)));
}

gboolean the_keys(GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer user_data)
{
    satellite_window *console = static_cast<satellite_window *>(user_data);
    if (console->widget == nullptr || console->terminal == nullptr)
        return FALSE;
    VteTerminal *terminal = terminal_of(*console);
    const bool a_control_chord = (state & GDK_CONTROL_MASK) != 0 && (state & (GDK_ALT_MASK | GDK_SUPER_MASK)) == 0;
    if (a_control_chord && (keyval == GDK_KEY_v || keyval == GDK_KEY_V)) {
        vte_terminal_paste_clipboard(terminal);
        return TRUE;
    }
    if (a_control_chord && (keyval == GDK_KEY_c || keyval == GDK_KEY_C)) {
        if (the_console_is_held) {
            // _format AND NOT vte_terminal_copy_clipboard(), satl-term's reason:
            // the other would paste this window's colours into whatever got it.
            if (vte_terminal_get_has_selection(terminal))
                vte_terminal_copy_clipboard_format(terminal, VTE_FORMAT_TEXT);
            else
                close_it_on_purpose(*console);
            return TRUE;
        }
        struct termios now;
        if (!the_pty_is_satls_terminal && tcgetattr(console->slave, &now) == 0 && (now.c_lflag & ISIG) != 0) {
            kill(getpid(), SIGINT);
            return TRUE;
        }
        return FALSE;
    }
    if (the_console_is_held && !is_only_a_modifier(keyval)) {
        close_it_on_purpose(*console);
        return TRUE;
    }
    return FALSE;
}

} // namespace

bool open_the_interpreters_console(const std::string &title, std::string &why)
{
    if (the_interpreters_console != nullptr) {
        why = "satl's own console is already open";
        return false;
    }
    // THE LAUNCHER'S NAME FOR IT, SET BEFORE THE DESK STARTS GTK. With no
    // GtkApplication, GDK hands the compositor g_get_prgname() as the window's
    // app id, and a desktop shell matches a window to its launcher by that id:
    // this is what puts the Satellite icon on the window and lets a person pin
    // it. It is the .desktop file's own name (satellite_enterprise/icons/), and
    // satl-term's, whose launcher satl now is.
    g_set_prgname("org.satellite.terminal");
    // THE PIXEL SIZE IS A STARTING POINT: fit_cells makes it 150 by 48 cells on
    // the first frame, whatever the font measured.
    WindowHandle console = console_new(title, 1000, 700, why);
    if (console == nullptr)
        return false;
    console->is_satls_own = true;

    // THE INTERPRETER RUNS AT THE LOWEST PRIORITY THERE IS, and the window does
    // not -- satl-term's rule (satl-term/child.cpp), ported. Measured there on
    // 2026-09-12: an interpreter running 23 compute threads at priority 0 on this
    // 24-thread machine took every core and the desktop stopped answering. A
    // launcher's satl cannot be niced by a shell alias, so it nices itself; it
    // costs an idle machine nothing. ON LINUX A NICE VALUE IS A THREAD'S, and new
    // threads inherit their creator's: called here, on the interpreter's thread,
    // after console_new started the desk and before run_satl starts the startup
    // threads, it lowers the interpreter and every thread it will start and
    // leaves the desk -- which draws this window -- where it was. A refusal
    // leaves it where it was, which is what not asking would have done.
    setpriority(PRIO_PROCESS, 0, 19);

    // THE PTY BECOMES satl'S TERMINAL, AS FAR AS IT CAN. setsid() so that the
    // pty CAN be the controlling terminal; it fails when satl is already a
    // process group leader -- which is every satl a shell started, for job
    // control -- and it is not needed for a satl a launcher started: measured
    // 2026-09-22 on this desktop, nautilus and ptyxis are their own session
    // leaders already (pid == pgid == sid). TIOCSCTTY then succeeds for a
    // session leader with no terminal, the launcher case and the one this
    // exists for, and fails from a shell, whose own terminal satl keeps. Both
    // are fine: the pty carries every byte either way, and the one thing a
    // controlling terminal adds -- Ctrl-C as a signal -- the_keys adds where
    // the kernel will not.
    setsid();
    the_pty_is_satls_terminal = ioctl(console->slave, TIOCSCTTY, 0) == 0;

    // NOTHING SAID SO FAR IS LOST: what is buffered goes where it was pointed
    // before the switch, and everything after it lands in the window -- except
    // what the DESK says, which stays where satl was started; the function
    // says why, and it is the one line here that must come before the dup2.
    std::cout.flush();
    std::cerr.flush();
    keep_the_c_streams_off_the_pty();
    if (dup2(console->slave, STDIN_FILENO) < 0 || dup2(console->slave, STDOUT_FILENO) < 0 ||
        dup2(console->slave, STDERR_FILENO) < 0) {
        why = std::string("could not move satl's own input and output onto the console -- ") +
              std::strerror(errno);
        std::string ignored;
        window_close(*console, ignored);
        return false;
    }
    // `slave` STAYS OPEN ON THE HANDLE, as every console's does, and here
    // the_keys' tcgetattr reads it. Nothing of a program's ever reaches it:
    // this handle is never a Value.

    satellite_window *raw = console.get();
    on_the_desk([raw] {
        gtk_widget_add_tick_callback(static_cast<GtkWidget *>(raw->terminal), fit_cells, raw->widget, nullptr);
        give_it_a_file_menu(*raw);
        give_it_a_status_bar(*raw);
        GtkEventController *keys = gtk_event_controller_key_new();
        gtk_event_controller_set_propagation_phase(keys, GTK_PHASE_CAPTURE);
        g_signal_connect(keys, "key-pressed", G_CALLBACK(the_keys), raw);
        gtk_widget_add_controller(static_cast<GtkWidget *>(raw->widget), keys);
    });
    the_interpreters_console = console;
    return true;
}

bool the_interpreters_console_is_open()
{
    return the_interpreters_console != nullptr && the_interpreters_console->widget != nullptr &&
           the_interpreters_console->on_the_screen;
}

void the_interpreters_console_is_done(bool hold, const std::string &message)
{
    WindowHandle console = the_interpreters_console;
    if (console == nullptr || console->widget == nullptr || !console->on_the_screen)
        return;
    // NOBODY IS HANGING UP NOW. Closing this console closes the pty's master,
    // and when the pty is satl's controlling terminal the kernel says SIGHUP
    // for that -- which would end satl by a signal, with the code it stopped
    // on lost, at the very moment it was about to exit with it. The run is
    // over; a hangup from here on means nothing.
    std::signal(SIGHUP, SIG_IGN);
    satellite_window *raw = console.get();
    if (!hold) {
        on_the_desk([raw] { close_it_on_purpose(*raw); });
        return;
    }
    // THE PROGRAM'S OWN WINDOWS GO DOWN, as they do on a stopped run with no
    // console (close_the_desk_now); the console stays, with the message on it.
    close_the_program_windows_now();
    // THE MESSAGE IS WRITTEN TO satl'S OWN STDERR -- which is the pty -- and
    // not fed to the terminal: fed, it would draw at once, AHEAD of whatever
    // the program printed last that the desk has not drained yet. Written, it
    // queues behind everything already on its way, as a last line should.
    const std::string line = "\n" + message + "\n";
    const ssize_t wrote = write(STDERR_FILENO, line.data(), line.size());
    (void)wrote;   // the pty may be gone; then there is nobody to tell
    on_the_desk([] { the_console_is_held = true; });
}

} // namespace satellite004

#else

namespace satellite004 {

bool open_the_interpreters_console(const std::string &, std::string &why) { return without_a_console(why); }
bool the_interpreters_console_is_open() { return false; }
void the_interpreters_console_is_done(bool, const std::string &) {}

} // namespace satellite004

#endif
