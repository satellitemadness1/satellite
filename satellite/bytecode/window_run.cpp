// satellite/bytecode/window_run.cpp -- THE WINDOW'S OWN RUN (WIN-11): every
// event off the desk's queue, one at a time, into the capsule that answers it,
// on the interpreter's thread; and the wait that holds the run open while a
// window is. window_readers.hpp says how the six window files were cut.
//
// WHAT AN EVENT SAID IS COPIED ONTO THE PIECE HERE, and that is the whole
// reason it travelled on the event (GTK-14, GTK-11, GTK-15's leftover): a
// key's name, a person's answer or a click's point written by the desk and
// read by a capsule would be a value with two threads on it; written here it
// has one writer, and the capsule about to run is the only reader.

#include "window_readers.hpp"

#include "../satellite_variable_window/window_desk.hpp"

#include <functional>
#include <string>

#include <cstdlib>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace satellite004 {

#if SATELLITE_HAS_WINDOW

void windows_hold_the_run_open(bool the_program_finished)
{
    windows_stay_open_until_closed(the_program_finished);
}

signed long long int windows_run_until_they_are_closed(
    const std::function<signed long long int(const std::string &, const WindowHandle &,
                                             const WindowHandle &)> &run_a_capsule)
{
    AnEvent happened;
    while (the_desk_waits_for_something(happened)) {
        // WHAT IT SAID IS COPIED ONTO THE PIECE **HERE**, ON THE INTERPRETER'S
        // THREAD, and that is the whole reason it travelled on the event
        // (GTK-14). A key's name written by the desk and read by a capsule
        // would be a std::string with two threads on it; written here it has
        // one writer, and the capsule that is about to run is the only reader.
        if (happened.piece != nullptr) {
            if (happened.said_what == AnEvent::a_key)
                happened.piece->last_key = happened.said;
            else if (happened.said_what == AnEvent::an_answer)
                happened.piece->last_answer = happened.said;
            // AND WHERE A CLICK LANDED (GTK-15's leftover), the same way and
            // for the same reason: two numbers with one writer.
            else if (happened.said_what == AnEvent::a_place) {
                happened.piece->last_across = happened.across;
                happened.piece->last_down = happened.down;
            }
            // AND A LINE FINISHED IN A CONSOLE (GTK-17), the same way again.
            else if (happened.said_what == AnEvent::a_line)
                happened.piece->last_typed = happened.said;
        }
        const signed long long int stopped = run_a_capsule(happened.capsule, happened.piece, happened.window);
        // A CAPSULE THAT STOPPED STOPS THE RUN, the same as a line of main
        // would have. The report is already printed by the time this answers,
        // and main() takes the windows down on a code that stops -- a person
        // told their program stopped must not be left pressing a button that
        // still looks alive.
        if (stops_the_program(stopped))
            return stopped;
    }
    return success;
}

// ---------------------------------------------------------------------------
// THE CONSOLE satl LAUNCHES FOR ITSELF (GTK-17): `satl --console`.
// ---------------------------------------------------------------------------
namespace {

// WHETHER A CLEAN END HOLDS THE CONSOLE: true for the prompt, false for a file.
// Set when it is opened, read when the run is done -- satl-term's policy, in
// console_launch.cpp's own words.
bool console_holds_after_a_clean_run = false;

} // namespace

// WHETHER NOBODY GAVE satl A CONSOLE -- WIN-9, ruled by the author on
// 2026-09-22: "satl has to, when it's not ran in a console, take you to it's
// prompt". 003's six reasons, less the one that was satl-term (it is gone, and
// satl opens its own console now, in this process):
//
//   1. SATL_NO_WINDOW is set -- the lever for a launcher whose command line
//      somebody else wrote, and for every harness that runs satl without a
//      terminal (check.sh sets it for the whole suite);
//   2. there IS a controlling terminal -- /dev/tty opens -- so satl is where a
//      person is looking, pipeline or not. isatty(1) is the trap here: `satl
//      --words | grep x` has a pipe on stdout and would open a window;
//   3. stdout is a PIPE or a REGULAR FILE -- somebody arranged to read what
//      satl prints, even from a cron job or a systemd unit with no terminal;
//   4. there is no display to put a window on.
//
// ANY OF THEM AND satl PRINTS WHERE IT WAS POINTED. What is left -- no terminal,
// output going nowhere anybody reads (/dev/null, the session journal's socket),
// and a screen -- is a .satl double-clicked or satl started from a menu, and it
// gets a console. /dev/null IS THE CASE THIS EXISTS FOR, so a sweep that sends
// satl's stdout there from a harness with no terminal opens windows: redirect to
// a file, or set SATL_NO_WINDOW (satl-console-handover-trap, 003's lesson).
//
// O_NOCTTY, so that asking whether there is a controlling terminal cannot make
// /dev/tty one -- a test that changes what it tests.
bool nobody_gave_satl_a_console()
{
    const char *off = std::getenv("SATL_NO_WINDOW");
    if (off != nullptr && off[0] != '\0')
        return false;
    const int tty = open("/dev/tty", O_RDONLY | O_NOCTTY | O_CLOEXEC);
    if (tty >= 0) {
        close(tty);
        return false;
    }
    struct stat out;
    if (fstat(STDOUT_FILENO, &out) == 0 && (S_ISFIFO(out.st_mode) || S_ISREG(out.st_mode)))
        return false;
    const char *wayland = std::getenv("WAYLAND_DISPLAY");
    const char *x11 = std::getenv("DISPLAY");
    return (wayland != nullptr && wayland[0] != '\0') || (x11 != nullptr && x11[0] != '\0');
}

signed long long int open_satls_own_console(const std::string &title, bool holds_after_a_clean_run,
                                            bool quietly)
{
    std::string why;
    if (!open_the_interpreters_console(title, why)) {
        // SATL ASKED FOR IT ITSELF AND COULD NOT HAVE IT: it carries on where it
        // was pointed, saying nothing -- there is nowhere to say it, which is why
        // it was asking. 003's sixth reason, kept.
        if (quietly)
            return success;
        // NO DISPLAY IS THE MACHINE'S; NO CONSOLE IN THIS satl IS THE BUILD'S.
        // Told apart by the sentence, as call_window_word tells "no display".
        const bool not_built = why.find("built without a console") != std::string::npos;
        return report_error("satl(console): the console could not be opened -- " + why,
                            not_built ? not_built_yet : no_display);
    }
    console_holds_after_a_clean_run = holds_after_a_clean_run;
    return success;
}

bool satls_own_console_is_open() { return the_interpreters_console_is_open(); }

void satls_own_console_is_done(signed long long int code)
{
    const bool stopped = stops_the_program(code);
    std::string message;
    if (stopped)
        message = "[satl] stopped on machine code " + std::to_string(code) + " (" + machine_code_name(code) +
                  ") -- press any key to close";
    else if (console_holds_after_a_clean_run)
        message = "[satl] the session is over -- press any key to close";
    the_interpreters_console_is_done(stopped || console_holds_after_a_clean_run, message);
}

#else

// NOTHING TO HOLD OPEN: no window word ever answered a window in this build.
void windows_hold_the_run_open(bool) {}

// AND NO CONSOLE TO LAUNCH: the one sentence a satl without a window says,
// with the code a thing not built answers -- and nobody to ask for one on
// satl's behalf, so a satl with no window prints where it was pointed.
bool nobody_gave_satl_a_console() { return false; }

signed long long int open_satls_own_console(const std::string &, bool, bool quietly)
{
    if (quietly)
        return success;
    return report_error("satl(console): this satl was built without a window -- pkg-config found no gtk4 "
                        "when it was made, so it has no console to open. Install gtk4-devel and "
                        "vte291-gtk4-devel (AlmaLinux/RHEL, the second from CRB; Debian/Ubuntu: "
                        "libgtk-4-dev and libvte-2.91-gtk4-dev) and build again",
                        not_built_yet);
}
bool satls_own_console_is_open() { return false; }
void satls_own_console_is_done(signed long long int) {}

// AND NOTHING TO PRESS. No window word answered a window, so no button was made
// and no press can be waiting.
signed long long int windows_run_until_they_are_closed(
    const std::function<signed long long int(const std::string &, const WindowHandle &,
                                             const WindowHandle &)> &)
{
    return success;
}

#endif

} // namespace satellite004
