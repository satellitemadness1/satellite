#!/bin/sh
# satellite/satellite_variable_window/press-a-button.sh -- PRESS A REAL BUTTON
# AND PROVE A SATELLITE CAPSULE RAN. SATELLITE_WINDOW.md WIN-11.
#
# THIS IS NOT IN check.sh AND CANNOT BE. check.sh's window rows all run with no
# display on purpose -- a row that opened a window would put one on the screen of
# whoever ran the suite. This script opens one, clicks it, and closes it, on a
# compositor of its own. Run it by hand; it takes about twenty seconds.
#
#     sh satellite/satellite_variable_window/press-a-button.sh
#
# WHAT IT PROVES, end to end and in this order:
#   1. satl opens a window with a button in it on a real compositor
#   2. main RETURNS with the window still open, and the run does not end
#   3. a real pointer click makes GTK emit `clicked` on the real GtkButton
#   4. that reaches the desk, which queues the capsule's NAME
#   5. the INTERPRETER's thread takes it off and walks the capsule -- three
#      clicks print three lines, in the order they were made
#   6. closing the window ends the run, and satl exits 0 with its output flushed
#
# ---------------------------------------------------------------------------
# THE FOUR TRAPS, each of which cost a run the first time.
# ---------------------------------------------------------------------------
#
# 1. `env -u WAYLAND_DISPLAY` IS NOT HEADLESS. libwayland falls back to
#    $XDG_RUNTIME_DIR/wayland-0 and reaches the real desktop -- a window opened
#    on the author's own screen this way on 2026-09-20. This script does the
#    opposite of hiding: it names a compositor of its own, `satlwin`, and
#    XDG_RUNTIME_DIR must be the REAL one, because that is where mutter puts the
#    socket it is told to make.
#
# 2. satl MUST NOT SEE THE SESSION BUS, and this one hangs rather than fails.
#    CLOSED 2026-09-22: the author ruled Q-WIN-11a "defend", the desk turns
#    portals off before opening a display, and prove-canvas-tabs-menus.sh's
#    file stage runs satl ON this bus and exits 0. The env -u stays here as
#    belt and braces, and the history below is why it was ever needed.
#    `gtk_init_check` calls gdk_display_should_use_portal -> check_portal_interface
#    (gdk/gdk.c:525), a SYNCHRONOUS g_dbus_connection_call_sync to
#    org.freedesktop.portal.Settings with no timeout of satl's. Under
#    dbus-run-session the portal is activatable but cannot finish starting, so
#    the call never returns: satl sits in gtk_init_check for ever, the desk never
#    sets desk_tried, and the interpreter waits in open_the_desk. It reads
#    exactly like satl locking up and it is not satl. `env -u
#    DBUS_SESSION_BUS_ADDRESS` is the whole fix -- and NOT because satl then has
#    no bus (read from GLib's source, 2026-09-22): with the variable unset GLib
#    falls back to $XDG_RUNTIME_DIR/bus, the user's REAL session bus, whose
#    portal answers. satl is moved to a bus that works; mutter and the clicker
#    keep the one dbus-run-session made. The capped probe at gdk.c:525 is where
#    the trace was caught; the unbounded wait on that path is the ReadAll at
#    gdksettings-wayland.c:477, and which one held is not settled.
#
# 3. MUTTER IS KILLED BY PID, never `pkill -f mutter` -- that pattern matches
#    inside this file and kills the shell running it.
#
# 4. A SATELLITE FILE IS SAVED WHEN ITS FRAME ENDS, not when it is appended to.
#    An early attempt at this test wrote evidence to a file from main and read it
#    back while main was still open: the file existed and was empty, which reads
#    exactly like the window word having failed. The console is what is checked
#    here, and it is checked AFTER satl has exited and flushed.
#
# ---------------------------------------------------------------------------
# WHY THE CLICK GOES THROUGH D-BUS. There is no sway, no wtype, no ydotool and
# no uinput on this machine, and XTest cannot reach a Wayland client. mutter's
# own org.gnome.Mutter.RemoteDesktop is what headless mutter offers instead:
# CreateSession, Start, NotifyPointerMotionRelative, NotifyPointerButton. THE
# SESSION DIES WITH THE CONNECTION THAT MADE IT, so the whole click sequence must
# happen inside ONE process -- a shell loop of `busctl call` creates a session
# per call and each one is destroyed before the next. That is why the clicker is
# Python and not three lines of busctl. The method is
# NotifyPointerMotionRELATIVE: the ABSOLUTE one wants a ScreenCast stream path,
# which would drag PipeWire in for nothing.
set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
satl="$root/build/satl"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

[ -x "$satl" ] || { echo "press-a-button.sh: no $satl -- run make first"; exit 1; }
command -v mutter >/dev/null || { echo "press-a-button.sh: mutter is not installed"; exit 1; }
command -v dbus-run-session >/dev/null || { echo "press-a-button.sh: dbus-run-session is not installed"; exit 1; }
[ -d "${XDG_RUNTIME_DIR:-}" ] || { echo "press-a-button.sh: XDG_RUNTIME_DIR must be the real one"; exit 1; }

cat > "$work/press.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_pressed()
{
    satellite.console.display("the button was pressed")
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("press test", 800, 600)
    satellite.variable.window b = satellite.window.button("press me")
    b.pressed(when_pressed)
    w.append(b, 400, 300)
    satellite.console.display("main is finished, and the window is still open")
    satellite.return(satellite)
}
SATL

# THE SECOND PROGRAM, AND IT NEEDS NO POINTER AT ALL. `.press()` is the program
# pressing its own button, and it proves three things a click cannot:
#   * a press queues and runs AFTER main's own lines -- main's line prints first
#   * presses run in the order they were made
#   * a press is still DRAINED after the window has been closed. main closes the
#     window on the line after pressing twice, and both capsules still run. That
#     is the_desk_waits_for_something testing the queue BEFORE the windows, and it
#     is the one case where a person pressed something and would otherwise have
#     watched nothing happen.
cat > "$work/selfpress.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_pressed()
{
    satellite.console.display("the button was pressed")
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("self press", 800, 600)
    satellite.variable.window b = satellite.window.button("press me")
    b.pressed(when_pressed)
    w.append(b, 400, 300)

    b.press()
    b.press()
    satellite.console.display("main pressed it twice and is closing the window")
    w.close()

    satellite.return(satellite)
}
SATL

# THE THIRD PROGRAM: THE CAPSULE CLOSES THE WINDOW IT WAS PRESSED IN. That is
# the whole point of a press being able to take arguments -- there are no
# globals, so main's name for the window is not something a capsule can see, and
# before 2026-09-21 a pressed capsule could print and write files and nothing
# else. Nobody closes the window here but the capsule, so an exit 0 IS the proof.
cat > "$work/reach.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_pressed(satellite.variable.window the_piece, satellite.variable.window its_window)
{
    satellite.console.display("the capsule was handed the window: " + its_window.title)
    its_window.close()
    satellite.console.display("the capsule closed it")
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("reached from a press", 800, 600)
    satellite.variable.window b = satellite.window.button("press me")
    b.pressed(when_pressed)
    w.append(b, 400, 300)
    b.press()
    satellite.console.display("main is finished and has NOT closed anything")
    satellite.return(satellite)
}
SATL

# THE CLICKER. One process, one bus connection, one RemoteDesktop session.
cat > "$work/click.py" <<'PY'
import sys, time
import gi
gi.require_version('Gio', '2.0')
from gi.repository import Gio

bus = Gio.bus_get_sync(Gio.BusType.SESSION, None)
desktop = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
        'org.gnome.Mutter.RemoteDesktop', '/org/gnome/Mutter/RemoteDesktop',
        'org.gnome.Mutter.RemoteDesktop', None)
session = Gio.DBusProxy.new_sync(bus, Gio.DBusProxyFlags.NONE, None,
        'org.gnome.Mutter.RemoteDesktop', desktop.CreateSession(),
        'org.gnome.Mutter.RemoteDesktop.Session', None)
session.Start()
BTN_LEFT = 272                       # evdev, not X11: BTN_LEFT is 0x110

def click(x, y):
    # PARKED FIRST, because the motion is RELATIVE and where the pointer starts
    # is not ours to know. A move far past the top left clamps to 0,0.
    session.NotifyPointerMotionRelative('(dd)', -9000.0, -9000.0)
    session.NotifyPointerMotionRelative('(dd)', float(x), float(y))
    time.sleep(0.15)
    session.NotifyPointerButton('(ib)', BTN_LEFT, True)
    time.sleep(0.08)
    session.NotifyPointerButton('(ib)', BTN_LEFT, False)
    time.sleep(0.3)

for spot in sys.argv[1:]:
    across, down = spot.split(',')
    click(int(across), int(down))
time.sleep(1.0)
session.Stop()
PY

cat > "$work/inside.sh" <<'INSIDE'
set -u
mutter --headless --virtual-monitor 1280x800 --wayland-display=satlwin >"$work/mutter.log" 2>&1 &
mutter_pid=$!
tries=0
while [ ! -S "$XDG_RUNTIME_DIR/satlwin" ] && [ $tries -lt 60 ]; do sleep 0.25; tries=$((tries + 1)); done
if [ ! -S "$XDG_RUNTIME_DIR/satlwin" ]; then
    echo "press-a-button.sh: mutter never made its socket"; kill $mutter_pid 2>/dev/null; exit 1
fi

# NO SESSION BUS FOR satl -- trap 2 above. NO DISPLAY EITHER: the vendored GTK
# has no X11 backend at all (WIN-10), so a DISPLAY left set is only a way to be
# confusing.
env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS WAYLAND_DISPLAY=satlwin \
    "$satl" "$work/press.satl" >"$work/satl.out" 2>&1 &
satl_pid=$!
sleep 4

# THE BUTTON IS AT THE CENTRE OF AN 800x600 WINDOW that mutter centres on a
# 1280x800 monitor, so it is near (640, 437) -- 400,300 inside the window, plus
# the window's own corner at (240, 100), plus GTK's client-side titlebar. A click
# that misses lands on the empty GtkFixed and costs nothing, which is why this
# sweeps a little rather than insisting on one pixel. THE LAST THREE ARE THE
# WINDOW'S CLOSE BUTTON, top right of that same window: closing it is what ends
# the run, and an exit 0 with the output flushed is half of what this proves.
/usr/bin/python3 "$work/click.py" 640,437 640,437 640,437 1015,118 1000,118 1025,120 \
    >"$work/click.log" 2>&1 || { echo "press-a-button.sh: the clicker failed"; cat "$work/click.log"; }

waited=0
while kill -0 $satl_pid 2>/dev/null && [ $waited -lt 20 ]; do sleep 0.5; waited=$((waited + 1)); done
if kill -0 $satl_pid 2>/dev/null; then
    echo "press-a-button.sh: satl did not exit -- the window was never closed"
    kill $satl_pid 2>/dev/null
fi
wait $satl_pid 2>/dev/null
echo "exit $?" > "$work/exit"

# STAGE TWO: no pointer, no D-Bus, no coordinates. The program presses itself.
env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS WAYLAND_DISPLAY=satlwin \
    timeout 30 "$satl" "$work/selfpress.satl" >"$work/self.out" 2>&1
echo "exit $?" > "$work/self_exit"

# STAGE THREE: nobody closes the window but the capsule.
env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS WAYLAND_DISPLAY=satlwin \
    timeout 30 "$satl" "$work/reach.satl" >"$work/reach.out" 2>&1
echo "exit $?" > "$work/reach_exit"

kill $mutter_pid 2>/dev/null
wait $mutter_pid 2>/dev/null
INSIDE

work="$work" satl="$satl" dbus-run-session -- sh "$work/inside.sh"

status=$(cat "$work/exit" 2>/dev/null || echo "exit ?")
presses=$(grep -c '^the button was pressed$' "$work/satl.out" 2>/dev/null || echo 0)
after=$(grep -c '^main is finished, and the window is still open$' "$work/satl.out" 2>/dev/null || echo 0)

self_status=$(cat "$work/self_exit" 2>/dev/null || echo "exit ?")
self_presses=$(grep -c '^the button was pressed$' "$work/self.out" 2>/dev/null || echo 0)
# THE ORDER IS THE CLAIM, so it is read and not just counted: main's own line
# must come BEFORE both presses, because a press waits for the program to finish.
self_order=$(grep -n -E '^(main pressed it twice and is closing the window|the button was pressed)$' \
             "$work/self.out" 2>/dev/null | cut -d: -f2- | tr '\n' '|')

reach_status=$(cat "$work/reach_exit" 2>/dev/null || echo "exit ?")
reach_closed=$(grep -c '^the capsule closed it$' "$work/reach.out" 2>/dev/null || echo 0)
reach_title=$(grep -c '^the capsule was handed the window: reached from a press$' "$work/reach.out" 2>/dev/null || echo 0)

echo "---------------------------------------------------------------"
echo "A REAL POINTER"
echo "  satl $status        (0 -- the window was closed and the run ended)"
echo "  main returned with the window open: $after   (want 1)"
echo "  capsule runs from real clicks:      $presses   (want 3)"
sed -n '/^main is finished/,$p' "$work/satl.out" 2>/dev/null | sed 's/^/    /'
echo "THE PROGRAM PRESSING ITSELF -- .press()"
echo "  satl $self_status        (0 -- drained after the window closed)"
echo "  capsule runs from .press():         $self_presses   (want 2)"
sed -n '/^main pressed it twice/,$p' "$work/self.out" 2>/dev/null | sed 's/^/    /'
echo "THE CAPSULE REACHING ITS OWN WINDOW -- a capsule taking the piece and the window"
echo "  satl $reach_status        (0 -- and NOBODY closed the window but the capsule)"
echo "  the window arrived, by name:        $reach_title   (want 1)"
echo "  the capsule closed it:              $reach_closed   (want 1)"
sed -n '/^main is finished and has NOT/,$p' "$work/reach.out" 2>/dev/null | sed 's/^/    /'
echo "---------------------------------------------------------------"

want_order='main pressed it twice and is closing the window|the button was pressed|the button was pressed|'
[ "$status" = "exit 0" ] && [ "$after" = "1" ] && [ "$presses" = "3" ] &&
[ "$self_status" = "exit 0" ] && [ "$self_presses" = "2" ] && [ "$self_order" = "$want_order" ] &&
[ "$reach_status" = "exit 0" ] && [ "$reach_title" = "1" ] && [ "$reach_closed" = "1" ] || {
    echo "press-a-button.sh: FAILED"; exit 1; }
echo "press-a-button.sh: a button was pressed -- by a person and by the program -- and satellite ran"
