#!/bin/sh
# satellite/satellite_variable_window/prove-canvas-tabs-menus.sh -- DRAW ON A
# CANVAS, CLICK A TAB, WALK A MENU INTO A MENU, TICK ONE OF A RADIO, on a
# compositor of its own. GTK_AND_NO_DEPENDENCIES.md GTK-15, GTK-16, GTK-12's
# leftovers (2026-09-22), and the same day GTK-15's four leftovers and GTK-3's
# radio.
#
# THIS IS NOT IN check.sh AND CANNOT BE, for press-a-button.sh's reason: every
# window row there runs with no display on purpose. This opens windows on a
# headless mutter, drives them with a real pointer and real keys through
# mutter's own RemoteDesktop, and checks what satl printed AFTER it exited.
# Run it by hand; it takes about forty seconds.
#
#     sh satellite/satellite_variable_window/prove-canvas-tabs-menus.sh
#     KEEP_WORK=1 sh ...    keeps the work folder, with before.png and after.png
#
# WHAT IT PROVES, in three stages:
#   canvas  a 400x300 canvas is drawn on (two lines, a box, a circle, words in
#           IBM Plex Mono 18, then a four-pixel OUTLINED box, circle and arc
#           and a filled slice) BEFORE it is on the screen and saved to a PNG;
#           a real pointer click at its CENTRE runs its .clicked capsule, which
#           is handed the canvas and its window, reads WHERE the click landed
#           (.across 200, .down 150), draws MORE on it, saves a second PNG and
#           closes the window. The picture is the proof that the display list
#           replays, and looking at one is what found the pen defect
#           (window_canvas.cpp, stroke_it).
#   one_of  three words as a radio; .chosen reads the first from the start,
#           .chosen("large") and reads it back; a real pointer click on the
#           MIDDLE button runs the .changed capsule with .chosen answering
#           "medium", and the capsule closes the window.
#   tabs    two pages named by their pieces' .title; .chosen read and written;
#           a page renamed while in the set; a real pointer click on the
#           renamed tab runs the .changed capsule, whose .chosen reads the new
#           name, and closes the window.
#   menus   File with Open, Save, a separator and Recent under it, More under
#           Recent with deep.satl in it, Recent renamed in place; real keys --
#           F10 Down Down Down Right Right Return -- pick deep.satl, and the
#           capsule is handed the menu it was on and the window.
#   file    GTK-11's file dialog, built 2026-09-22 on the author's ruling of
#           Q-WIN-11a ("defend"): .choose_a_file(when_chosen) opens GTK's own
#           chooser; real keys type a path and Return; the capsule reads it in
#           .answer and closes the window. AND THIS STAGE RUNS satl ON THE
#           SESSION BUS dbus-run-session MADE -- the bus that hung satl for
#           an afternoon (trap 2) -- because the desk turns portals off before
#           opening a display now. A hang here is the defence failing, and
#           finish() would report it.
#
# THE TRAPS ARE press-a-button.sh'S FOUR; read them there. Two more found here:
#   5. `wait $pid` on a process started inside $(...) answers 127 -- the child
#      belongs to the subshell. Start satl in the current shell and read $!.
#   6. THE FIRST KEY OF A REMOTE-DESKTOP SESSION IS SWALLOWED (GTK-14 measured
#      it), so every key sequence starts with Escape.
set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
satl="$root/build/satl"
work=$(mktemp -d)
keep=${KEEP_WORK:-}
[ -n "$keep" ] || trap 'rm -rf "$work"' EXIT

[ -x "$satl" ] || { echo "prove-canvas-tabs-menus.sh: no $satl -- run make first"; exit 1; }
command -v mutter >/dev/null || { echo "prove-canvas-tabs-menus.sh: mutter is not installed"; exit 1; }
command -v dbus-run-session >/dev/null || { echo "prove-canvas-tabs-menus.sh: dbus-run-session is not installed"; exit 1; }
[ -d "${XDG_RUNTIME_DIR:-}" ] || { echo "prove-canvas-tabs-menus.sh: XDG_RUNTIME_DIR must be the real one"; exit 1; }

cat > "$work/canvas.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_clicked(satellite.variable.window the_canvas, satellite.variable.window its_window)
{
    satellite.console.display("the canvas was clicked, and it measures")
    satellite.console.display(the_canvas.width)
    satellite.console.display(the_canvas.height)
    satellite.console.display("the click landed at")
    satellite.console.display(the_canvas.across)
    satellite.console.display(the_canvas.down)
    the_canvas.colour("#ff3030")
    the_canvas.circle(300, 200, 40)
    the_canvas.write(200, 260, "drawn after a click")
    the_canvas.save("after.png")
    satellite.console.display("saved after.png")
    its_window.close()
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("a canvas", 800, 600)
    satellite.variable.window c = satellite.window.canvas(400, 300)
    c.background("#202830")
    c.colour("#40c8ff")
    c.line(0, 0, 399, 299)
    c.line(0, 299, 399, 0)
    c.box(20, 20, 80, 50)
    c.colour("#ffd040")
    c.circle(100, 200, 30)
    c.font("IBM Plex Mono", 18)
    c.write(150, 40, "hello from satellite")
    c.thickness(4)
    c.outline(1)
    c.colour("#ff80ff")
    c.box(300, 20, 80, 50)
    c.circle(340, 200, 30)
    c.arc(200, 150, 60, 0, 270)
    c.outline(0)
    c.colour("#80ff80")
    c.arc(200, 150, 40, 270, 360)
    c.thickness(1)
    satellite.console.display("the pen is back to")
    satellite.console.display(c.thickness)
    satellite.console.display(c.outline)
    c.clicked(when_clicked)
    w.append(c, 400, 300)
    c.save("before.png")
    satellite.console.display("main drew and saved before.png, and the canvas measures")
    satellite.console.display(c.width)
    satellite.console.display(c.height)
    satellite.return(satellite)
}
SATL

cat > "$work/tabs.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_switched(satellite.variable.window the_tabs, satellite.variable.window its_window)
{
    satellite.console.display("a person switched to the tab named " + the_tabs.chosen)
    its_window.close()
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("tabs", 800, 600)
    satellite.variable.window t = satellite.window.tabs()
    satellite.variable.window first = satellite.window.label("the first page")
    satellite.variable.window second = satellite.window.text_area("the second page")
    first.title("First")
    second.title("Second")
    t.append(first)
    t.append(second)
    satellite.console.display("in front: " + t.chosen)
    t.chosen("Second")
    satellite.console.display("now in front: " + t.chosen)
    first.title("Alpha")
    satellite.console.display("renamed while in the tabs: " + first.title)
    t.changed(when_switched)
    w.append(t, 400, 300)
    satellite.console.display("the tabs measure")
    satellite.console.display(t.width)
    satellite.console.display(t.height)
    satellite.return(satellite)
}
SATL

cat > "$work/oneof.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_picked(satellite.variable.window the_group, satellite.variable.window its_window)
{
    satellite.console.display("a person picked " + the_group.chosen)
    its_window.close()
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("one of", 800, 600)
    satellite.variable.window g = satellite.window.one_of({"small", "medium", "large"})
    satellite.console.display("ticked from the start: " + g.chosen)
    g.chosen("large")
    satellite.console.display("now ticked: " + g.chosen)
    g.changed(when_picked)
    w.append(g, 400, 300)
    satellite.console.display("the group measures")
    satellite.console.display(g.width)
    satellite.console.display(g.height)
    satellite.return(satellite)
}
SATL

cat > "$work/file.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_chosen(satellite.variable.window the_window, satellite.variable.window its_window)
{
    satellite.console.display("chose: " + its_window.answer)
    its_window.close()
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("a file", 800, 600)
    w.choose_a_file(when_chosen)
    satellite.console.display("the chooser is up")
    satellite.return(satellite)
}
SATL

cat > "$work/menus.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_open(satellite.variable.window the_menu, satellite.variable.window its_window)
{
    satellite.console.display("picked Open on " + the_menu.text)
    its_window.close()
}
satellite.capsule when_save(satellite.variable.window the_menu, satellite.variable.window its_window)
{
    satellite.console.display("picked Save on " + the_menu.text)
    its_window.close()
}
satellite.capsule when_one(satellite.variable.window the_menu, satellite.variable.window its_window)
{
    satellite.console.display("picked one.satl on " + the_menu.text)
    its_window.close()
}
satellite.capsule when_two(satellite.variable.window the_menu, satellite.variable.window its_window)
{
    satellite.console.display("picked two.satl on " + the_menu.text)
    its_window.close()
}
satellite.capsule when_deep(satellite.variable.window the_menu, satellite.variable.window its_window)
{
    satellite.console.display("picked deep.satl on " + the_menu.text + " in " + its_window.title)
    its_window.close()
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("menus", 800, 600)
    satellite.variable.window file = satellite.window.menu("File")
    satellite.variable.window recent = satellite.window.menu("Recent")
    satellite.variable.window more = satellite.window.menu("More")
    more.item(when_deep, "deep.satl")
    recent.menu(more)
    recent.item(when_one, "one.satl")
    recent.item(when_two, "two.satl")
    file.item(when_open, "Open")
    file.item(when_save, "Save")
    file.separator()
    file.menu(recent)
    w.menu(file)
    recent.text("Recently")
    satellite.console.display("the bar is up")
    satellite.return(satellite)
}
SATL

# THE DRIVER: one process, one bus connection, one RemoteDesktop session. An
# argument "x,y" is a pointer click at that screen point; any other argument is
# a key by name.
cat > "$work/drive.py" <<'DRIVER'
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
BTN_LEFT = 272
KEYS = {'escape': 0xff1b, 'f10': 0xffc7, 'down': 0xff54, 'up': 0xff52, 'right': 0xff53,
        'left': 0xff51, 'return': 0xff0d, 'space': 0x020, 'shift': 0xffe1}

def click(x, y):
    session.NotifyPointerMotionRelative('(dd)', -9000.0, -9000.0)
    session.NotifyPointerMotionRelative('(dd)', float(x), float(y))
    time.sleep(0.15)
    session.NotifyPointerButton('(ib)', BTN_LEFT, True)
    time.sleep(0.08)
    session.NotifyPointerButton('(ib)', BTN_LEFT, False)
    time.sleep(0.4)

def key(name):
    sym = KEYS[name]
    session.NotifyKeyboardKeysym('(ub)', sym, True)
    time.sleep(0.06)
    session.NotifyKeyboardKeysym('(ub)', sym, False)
    time.sleep(0.35)

# "type:some text" TYPES IT, a character at a time: a printable ASCII
# character's keysym is its own code, and mutter synthesises the Shift a
# capital or a symbol needs (GTK-14 measured it).
for step in sys.argv[1:]:
    if step.startswith('type:'):
        for ch in step[5:]:
            session.NotifyKeyboardKeysym('(ub)', ord(ch), True)
            time.sleep(0.03)
            session.NotifyKeyboardKeysym('(ub)', ord(ch), False)
            time.sleep(0.05)
        time.sleep(0.3)
    elif ',' in step:
        x, y = step.split(',')
        click(int(x), int(y))
    else:
        key(step)
time.sleep(1.0)
session.Stop()
DRIVER

cat > "$work/inside.sh" <<'INSIDE'
set -u
cd "$work"
mutter --headless --virtual-monitor 1280x800 --wayland-display=satlwin >"$work/mutter.log" 2>&1 &
mutter_pid=$!
tries=0
while [ ! -S "$XDG_RUNTIME_DIR/satlwin" ] && [ $tries -lt 60 ]; do sleep 0.25; tries=$((tries + 1)); done
[ -S "$XDG_RUNTIME_DIR/satlwin" ] || { echo "mutter never made its socket"; kill $mutter_pid 2>/dev/null; exit 1; }

# NO SESSION BUS FOR satl, NO DISPLAY EITHER -- press-a-button.sh's traps 1 and 2.
start() { env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS WAYLAND_DISPLAY=satlwin "$satl" "$work/$1.satl" >"$work/$1.out" 2>&1 & pid=$!; }
finish() { waited=0
    while kill -0 $pid 2>/dev/null && [ $waited -lt 40 ]; do sleep 0.5; waited=$((waited + 1)); done
    if kill -0 $pid 2>/dev/null; then echo "$1: satl did not exit -- the window was never closed" >> "$work/$1.out"; kill $pid 2>/dev/null; fi
    wait $pid 2>/dev/null; echo "exit $?" > "$work/$1.exit"; }

# THE CANVAS IS CENTRED AT 400,300 IN AN 800x600 WINDOW that mutter centres on
# a 1280x800 monitor: the window's corner is at (240, 100) and its client-side
# title bar is 37 tall, so the canvas's own centre is at (640, 437).
start canvas; sleep 4
/usr/bin/python3 "$work/drive.py" 640,437 >"$work/canvas.drive" 2>&1
finish canvas

# THE TABS MEASURE 302 BY 189 and are centred at 400,300, so the tab bar runs
# across y = 205..241 in the window and the first tab starts at x = 249. A
# small sweep, because a tab's width is the theme's; a click on the tab already
# in front does nothing, and one that misses lands on the fixed and costs nothing.
start tabs; sleep 4
/usr/bin/python3 "$work/drive.py" 505,360 520,360 535,360 520,352 520,368 >"$work/tabs.drive" 2>&1
finish tabs

# ESCAPE FIRST (trap 6). F10 opens File; Down three times reaches Recently past
# the separator; Right opens it; Right again opens More, whose first item is
# focused; Return picks it.
start menus; sleep 4
/usr/bin/python3 "$work/drive.py" escape f10 down down down right right return >"$work/menus.drive" 2>&1
finish menus

# THE FILE DIALOG, ON THE BUS THAT USED TO HANG satl. `start_on_the_bus` is
# `start` without `-u DBUS_SESSION_BUS_ADDRESS`: dbus-run-session's bus, where
# the portal is activatable and never finishes starting -- the trap that cost
# an afternoon. With portals turned off before the display opens, satl never
# asks. Then a warm-up key (the first of a session is swallowed), "/" to open
# the chooser's location entry, the rest of the path, and Return.
printf 'the file satellite chose\n' > "$work/chosen.txt"
start_on_the_bus() { env -u DISPLAY WAYLAND_DISPLAY=satlwin "$satl" "$work/$1.satl" >"$work/$1.out" 2>&1 & pid=$!; }
start_on_the_bus file; sleep 5
/usr/bin/python3 "$work/drive.py" shift "type:$work/chosen.txt" return >"$work/file.drive" 2>&1
finish file

# THE RADIO IS CENTRED AT 400,300 TOO, so its middle button -- "medium", the
# second of three -- is at the same screen point the canvas's centre was. Two
# clicks, because the first click on a freshly mapped window can be swallowed
# (press-a-button.sh, trap 1) and a second click on a radio already ticked
# changes nothing.
start oneof; sleep 4
/usr/bin/python3 "$work/drive.py" 640,437 640,437 >"$work/oneof.drive" 2>&1
finish oneof

kill $mutter_pid 2>/dev/null; wait $mutter_pid 2>/dev/null
INSIDE

work="$work" satl="$satl" dbus-run-session -- sh "$work/inside.sh"

canvas_status=$(cat "$work/canvas.exit" 2>/dev/null || echo "exit ?")
canvas_clicked=$(grep -c '^the canvas was clicked, and it measures$' "$work/canvas.out" 2>/dev/null || echo 0)
canvas_saved=$(grep -c '^saved after.png$' "$work/canvas.out" 2>/dev/null || echo 0)
canvas_size=$(grep -x '400\|300' "$work/canvas.out" 2>/dev/null | tr '\n' ' ')
canvas_landed=$(grep -A2 '^the click landed at$' "$work/canvas.out" 2>/dev/null | tail -2 | tr '\n' ' ')
canvas_pen=$(grep -A2 '^the pen is back to$' "$work/canvas.out" 2>/dev/null | tail -2 | tr '\n' ' ')
oneof_status=$(cat "$work/oneof.exit" 2>/dev/null || echo "exit ?")
file_status=$(cat "$work/file.exit" 2>/dev/null || echo "exit ?")
file_chosen=$(grep -c "^chose: $work/chosen.txt\$" "$work/file.out" 2>/dev/null || echo 0)
file_hung=$(grep -c 'satl did not exit' "$work/file.out" 2>/dev/null); [ -n "$file_hung" ] || file_hung=missing
oneof_order=$(grep -E '^(ticked from the start: small|now ticked: large|a person picked medium)$' "$work/oneof.out" 2>/dev/null | tr '\n' '|')
pngs=$(ls "$work"/before.png "$work"/after.png 2>/dev/null | wc -l)
tabs_status=$(cat "$work/tabs.exit" 2>/dev/null || echo "exit ?")
tabs_order=$(grep -E '^(in front: First|now in front: Second|renamed while in the tabs: Alpha|a person switched to the tab named Alpha)$' "$work/tabs.out" 2>/dev/null | tr '\n' '|')
menus_status=$(cat "$work/menus.exit" 2>/dev/null || echo "exit ?")
menus_picked=$(grep -c '^picked deep.satl on More in menus$' "$work/menus.out" 2>/dev/null || echo 0)

echo "---------------------------------------------------------------"
echo "A CANVAS, DRAWN ON BEFORE AND AFTER A REAL CLICK"
echo "  satl $canvas_status        (0 -- the capsule closed the window)"
echo "  the click reached the canvas:       $canvas_clicked   (want 1)"
echo "  it measured:                        $canvas_size  (want 400 300 400 300)"
echo "  the click landed at:                $canvas_landed  (want 200 150, give or take a pixel)"
echo "  the pen read back:                  $canvas_pen  (want 1 false)"
echo "  drew after the click and saved:     $canvas_saved   (want 1)"
echo "  PNGs written:                       $pngs   (want 2)"
echo "A SET OF TABS, SWITCHED BY A REAL CLICK"
echo "  satl $tabs_status"
echo "  $tabs_order"
echo "A MENU INSIDE A MENU INSIDE A MENU, WALKED WITH REAL KEYS"
echo "  satl $menus_status"
echo "  deep.satl picked, capsule handed More and the window: $menus_picked   (want 1)"
echo "A RADIO, TICKED BY A REAL CLICK ON ITS MIDDLE BUTTON"
echo "  satl $oneof_status"
echo "  $oneof_order"
echo "A FILE CHOSEN BY TYPING ITS PATH, ON THE BUS THAT USED TO HANG satl"
echo "  satl $file_status        (0 -- and it was started WITH the session bus)"
echo "  satl hung on the bus:               $file_hung   (want 0 -- the defence)"
echo "  the capsule read the typed path:    $file_chosen   (want 1)"
[ -n "$keep" ] && echo "the work folder is kept: $work (before.png and after.png are in it)"
echo "---------------------------------------------------------------"

want_tabs='in front: First|now in front: Second|renamed while in the tabs: Alpha|a person switched to the tab named Alpha|'
want_oneof='ticked from the start: small|now ticked: large|a person picked medium|'
# WHERE THE CLICK LANDED IS JUDGED WITHIN A FEW PIXELS: the pointer is moved
# by relative steps and a compositor rounds, so 200 150 is the aim and not a
# promise; a click that lands on a different piece would be off by fifty.
landed_across=$(echo "$canvas_landed" | awk '{print $1+0}'); landed_down=$(echo "$canvas_landed" | awk '{print $2+0}')
landed_ok=0
[ "$landed_across" -ge 195 ] && [ "$landed_across" -le 205 ] && [ "$landed_down" -ge 145 ] && [ "$landed_down" -le 155 ] && landed_ok=1
[ "$canvas_status" = "exit 0" ] && [ "$canvas_clicked" = "1" ] && [ "$canvas_saved" = "1" ] && [ "$pngs" = "2" ] &&
[ "$canvas_size" = "400 300 400 300 " ] && [ "$landed_ok" = "1" ] && [ "$canvas_pen" = "1 false " ] &&
[ "$tabs_status" = "exit 0" ] && [ "$tabs_order" = "$want_tabs" ] &&
[ "$menus_status" = "exit 0" ] && [ "$menus_picked" = "1" ] &&
[ "$oneof_status" = "exit 0" ] && [ "$oneof_order" = "$want_oneof" ] &&
[ "$file_status" = "exit 0" ] && [ "$file_hung" = "0" ] && [ "$file_chosen" = "1" ] || {
    echo "prove-canvas-tabs-menus.sh: FAILED"; exit 1; }
echo "prove-canvas-tabs-menus.sh: a canvas was drawn on and clicked at its centre, a tab was clicked, a menu was walked, a radio was ticked and a file was chosen on the bus that used to hang -- and satellite ran"
