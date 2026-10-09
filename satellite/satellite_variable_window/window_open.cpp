// satellite/satellite_variable_window/window_open.cpp -- `my_window.open()`.
// satellite_window.hpp says what it answers; this is how.
//
// The author, 2026-10-03: *"for satellite.window on a satellite.variable.window
// object, the user needs to be able to type in: window_object.open() and it
// opens the window object that was already declared"* -- gtkcar/satl_window.satl:
//
//     satellite.variable.window local_window = satellite.window.new("gtkcar", 450, 250)
//
//     local_window.open()
//     local_window.close()
//
// 003 wrote the same three lines for a console, in the author's own example
// (old_versions/second_satellite/example/gui_example.satl):
// `my_terminal.open() // window appears`.
//
// `satellite.window.new` STILL PUTS THE WINDOW UP, as it has since WIN-3 -- the
// author, 2026-09-20: "get a window to appear when the window syntax is called"
// -- so every program written since keeps its window with no line changed. What
// `.open()` adds is the two things `new` never did: it WAITS until the window
// has been drawn, and it puts back a window that has closed. Whether `new`
// should stop showing a window until `.open()` -- 003's shape -- is the
// author's; it would be put_the_frame_up moving out of frame_new.
//
// COMPILED ONLY WHERE pkg-config FINDS gtk4, like its neighbours.

#include "satellite_window.hpp"

#include "window_desk.hpp"
#include "window_frame.hpp"

namespace satellite004 {

bool window_open(satellite_window &which, std::string &why)
{
    if (!which.is_a_window()) {
        why = std::string(which.piece_name()) + " is not opened -- it is on a screen once it is appended "
              "into a window, and a window is what opens";
        return false;
    }
    // THE DESK IS THERE FOR AS LONG AS A PROGRAM CAN HOLD A WINDOW -- it stops
    // only after main -- so this answers at once; a stopped desk would leave the
    // frame below posted to a loop nobody runs, and is refused instead.
    if (!open_the_desk(why)) {
        if (why.empty())
            why = "satl's window thread has already stopped";
        return false;
    }
    // A CONSOLE THAT HAS CLOSED IS NOT PUT BACK. Its frame could be, but its pty
    // closed with it, and a console is its pty: a new one would be an empty
    // screen in satl's own colours with the program's `.colour` and `.font` lost
    // -- VTE kept those, not the handle -- and that is a different console
    // wearing this one's name. satellite.console.new makes a new one honestly.
    const bool a_console = which.piece == satellite_window::console;
    // OPEN OR CLOSED IS ASKED UNDER THE DESK'S LOCK, and a closed window is
    // taken back in the same breath (window_desk.hpp), so a person closing it
    // at this very moment is either before the question or after the answer.
    if (!the_desk_still_holds(which.shared_from_this(), !a_console)) {
        if (a_console) {
            why = "a console that has closed cannot be opened again -- its pty and everything on its "
                  "screen went with it; satellite.console.new(\"a title\", 800, 600) makes another";
            return false;
        }
        satellite_window *raw = &which;
        on_the_desk([raw] { put_the_frame_up(*raw, a_fixed_to_place_pieces_in); });
    }
    // OPEN ALREADY, OR OPEN AGAIN: nothing else is done to it -- bringing it to
    // the front is `.focus()` -- and the line waits for its first frame if it
    // has not had one. `new` and then `.open()` on the very next line is the
    // author's own program, and that frame is the whole of what the line is for.
    the_desk_waits_until_it_is_drawn(which);
    return true;
}

} // namespace satellite004
