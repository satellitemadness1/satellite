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

#else

// NOTHING TO HOLD OPEN: no window word ever answered a window in this build.
void windows_hold_the_run_open(bool) {}

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
