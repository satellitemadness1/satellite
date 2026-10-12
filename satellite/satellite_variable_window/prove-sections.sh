#!/bin/sh
# satellite/satellite_variable_window/prove-sections.sh -- PROVE A WINDOW'S SECTIONS ON A REAL
# COMPOSITOR: every section and widget measured as GTK laid it out, before and after a resize, and a
# real click to show where a wrapped one went. The author, 2026-10-10:
# "main_window.section("section_name", 100%, 50%) ... takes up 100% width and 100% height of half of
# the window, and we can accept plain numbers as pixels, and percentages get converted into pixels"
# (window_widgets.cpp).
#
# NOT IN check.sh, FOR press-a-button.sh's REASON: check.sh never opens a window. This opens its
# own, on a headless mutter of its own (`satlsec`), and closes it. Run it by hand, about 20 seconds:
#
#     sh satellite/satellite_variable_window/prove-sections.sh [path/to/satl]
#
# WHAT IT PROVES -- every size read back through `.width` and `.height`, which ask GTK:
#   1. two 50% by 100% sections sit side by side, each half the window's area, the whole height
#   2. a widget's share is of its SECTION: a file display at 100% by 50% of the left half
#   3. sections hold sections: 25% of the left half; pixels are pixels, 300 by 200, and 50% of
#      those is 150 by 100
#   4. a share follows the window: resized to 1000 by 700, the halves are 500 wide
#   5. the author's own lines: a file display at 100% by 100% of a 100% by 50% section; a second
#      100% by 50% section goes UNDER it (a new row); three 50%s make a row of two and one under
#   6. a widget given no size in a section is all of it, 100% by 100%
#   7. WHERE a section goes, which no size can show -- one wrapped under the row and one cut off at
#      the right edge measure the same: three sections, the third too wide for the first row, and a
#      REAL CLICK where it must be if it wrapped, on the left under the first -- 10 pixels from the
#      window's bottom edge, so 100% is all of the window, with no strip left under the sections.
#      Its capsule says so and closes the window; press-a-button.sh's clicker, mutter's RemoteDesktop.
#   8. sizes read on the line after `.open()` are the laid-out ones -- nothing here sleeps first
#   9. a widget made across a window's top AFTER its section is above it: a click at the top of
#      the window lands in the input, and the section's capsule runs only for a click at the bottom
#  10. a window does sections or `.append`, one or the other: each way round is refused, by name
#      (the review of 2026-10-10 found an appended button hidden in a strip under the sections)
#
# press-a-button.sh's four traps hold here, and the two that matter are kept: XDG_RUNTIME_DIR is the
# REAL one, because mutter makes the socket it is told to there, and satl is moved off the private
# session bus. Mutter is stopped by its PID.

set -u
here=$(cd "$(dirname "$0")" && pwd)
satl=${1:-"$here/../../build/satl"}
work=$(mktemp -d)
failed=0

[ -x "$satl" ] || { echo "prove-sections.sh: no $satl -- run make first"; exit 1; }
command -v mutter >/dev/null || { echo "prove-sections.sh: mutter is not installed"; exit 1; }
[ -d "${XDG_RUNTIME_DIR:-}" ] || { echo "prove-sections.sh: XDG_RUNTIME_DIR must be the real one"; exit 1; }

cat > "$work/sections.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("sections", "prove-sections.sh", 800, 600)
    w.satellite.section("left", 50%, 100%)
    w.satellite.section("right", 50%, 100%)
    w.left.satellite.widgets.display.file("files", 100%, 50%)
    w.left.satellite.section("quarter", 100%, 25%)
    w.right.satellite.section("px", 300, 200)
    w.right.px.satellite.section("half", 50%, 50%)
    w.satellite.open()
    satellite.console.display(w.left.satellite.width)
    satellite.console.display(w.left.satellite.height)
    satellite.console.display(w.right.satellite.width)
    satellite.console.display(w.right.satellite.height)
    satellite.console.display(w.left.files.satellite.width)
    satellite.console.display(w.left.files.satellite.height)
    satellite.console.display(w.left.quarter.satellite.height)
    satellite.console.display(w.right.px.satellite.width)
    satellite.console.display(w.right.px.satellite.height)
    satellite.console.display(w.right.px.half.satellite.width)
    satellite.console.display(w.right.px.half.satellite.height)
    w.satellite.resize(1000, 700)
    satellite.time.sleep(500ms)
    satellite.console.display(w.left.satellite.width)
    satellite.console.display(w.right.satellite.width)
    satellite.console.display(w.right.px.satellite.width)
    w.satellite.close()
    satellite.return(satellite)
}
SATL

cat > "$work/his.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.window main_window = satellite.window.new("window title", "version", 800, 600)
    main_window.satellite.section("section_name", 100%, 50%)
    main_window.section_name.satellite.widgets.display.file("my_file_display", 100%, 100%)
    main_window.satellite.section("under", 100%, 50%)
    main_window.under.satellite.section("one", 50%, 50%)
    main_window.under.satellite.section("two", 50%, 50%)
    main_window.under.satellite.section("three", 50%, 50%)
    main_window.under.three.satellite.widgets.display.input("typed_here")
    main_window.satellite.open()
    satellite.console.display(main_window.section_name.satellite.width)
    satellite.console.display(main_window.section_name.satellite.height)
    satellite.console.display(main_window.section_name.my_file_display.satellite.width)
    satellite.console.display(main_window.section_name.my_file_display.satellite.height)
    satellite.console.display(main_window.under.satellite.height)
    satellite.console.display(main_window.under.one.satellite.width)
    satellite.console.display(main_window.under.three.satellite.width)
    satellite.console.display(main_window.under.three.satellite.height)
    satellite.console.display(main_window.under.three.typed_here.satellite.height)
    main_window.satellite.close()
    satellite.return(satellite)
}
SATL

cat > "$work/wrap.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_one(satellite.variable.window the_piece, satellite.variable.window its_window)
{
    satellite.console.display("clicked one")
    its_window.satellite.close()
}

satellite.capsule when_two(satellite.variable.window the_piece, satellite.variable.window its_window)
{
    satellite.console.display("clicked two")
    its_window.satellite.close()
}

satellite.capsule when_three(satellite.variable.window the_piece, satellite.variable.window its_window)
{
    satellite.console.display("clicked three")
    its_window.satellite.close()
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("wrap", "prove-sections.sh", 800, 600)
    w.satellite.section("one", 50%, 10%)
    w.satellite.section("two", 50%, 10%)
    w.satellite.section("three", 50%, 90%)
    w.one.satellite.clicked(when_one)
    w.two.satellite.clicked(when_two)
    w.three.satellite.clicked(when_three)
    w.satellite.open()
    satellite.console.display("ready")
    satellite.return(satellite)
}
SATL

cat > "$work/order.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_all(satellite.variable.window the_piece, satellite.variable.window its_window)
{
    satellite.console.display(the_piece.satellite.down)
    its_window.satellite.close()
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("order", "prove-sections.sh", 800, 600)
    w.satellite.section("all", 100%, 100%)
    w.satellite.widgets.display.input("typed_here")
    w.all.satellite.clicked(when_all)
    w.satellite.open()
    satellite.console.display("ready")
    satellite.return(satellite)
}
SATL

cat > "$work/append_after.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("append after", "prove-sections.sh", 800, 600)
    w.satellite.section("top", 100%, 50%)
    satellite.variable.window b = satellite.window.button("press me")
    w.satellite.append(b, 400, 300)
    w.satellite.close()
    satellite.return(satellite)
}
SATL

cat > "$work/section_after.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("section after", "prove-sections.sh", 800, 600)
    satellite.variable.window b = satellite.window.button("press me")
    w.satellite.append(b, 400, 300)
    w.satellite.section("top", 100%, 50%)
    w.satellite.close()
    satellite.return(satellite)
}
SATL

# THE CLICKER, press-a-button.sh's: one process, one bus connection, one RemoteDesktop session.
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
BTN_LEFT = 272

def click(x, y):
    session.NotifyPointerMotionRelative('(dd)', -9000.0, -9000.0)
    session.NotifyPointerMotionRelative('(dd)', float(x), float(y))
    time.sleep(0.15)
    session.NotifyPointerButton('(ib)', BTN_LEFT, True)
    time.sleep(0.08)
    session.NotifyPointerButton('(ib)', BTN_LEFT, False)
    time.sleep(0.4)

for spot in sys.argv[1:]:
    across, down = spot.split(',')
    click(int(across), int(down))
time.sleep(1.0)
session.Stop()
PY

cat > "$work/inside.sh" <<'INSIDE'
set -u
mutter --headless --virtual-monitor 1280x800 --wayland-display=satlsec >"$work/mutter.log" 2>&1 &
mutter_pid=$!
tries=0
while [ ! -S "$XDG_RUNTIME_DIR/satlsec" ] && [ $tries -lt 60 ]; do sleep 0.25; tries=$((tries + 1)); done
[ -S "$XDG_RUNTIME_DIR/satlsec" ] || { echo "mutter never made its socket"; kill $mutter_pid; exit 1; }
mkdir -p "$work/home/.satl"
for program in sections his append_after section_after; do
    env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS -u GDK_BACKEND -u XAUTHORITY HOME="$work/home" WAYLAND_DISPLAY=satlsec \
        timeout 30 "$satl" "$work/$program.satl" </dev/null >"$work/$program.out" 2>"$work/$program.err"
    echo $? > "$work/$program.exit"
done
# THE WINDOW IS 800x600, CENTRED ON THE 1280x800 MONITOR WITH ITS CORNER AT (240, 100), as
# press-a-button.sh found, and GTK's titlebar takes its top 47 pixels. Each click goes twice -- the
# first click on a newly mapped window can be swallowed -- and the capsule closes the window, which
# ends the run.
clicked() {   # clicked <program> <spot>... -- run it, click, wait for it to end
    env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS -u GDK_BACKEND -u XAUTHORITY HOME="$work/home" WAYLAND_DISPLAY=satlsec \
        "$satl" "$work/$1.satl" </dev/null >"$work/$1.out" 2>"$work/$1.err" &
    satl_pid=$!
    program=$1
    shift
    sleep 4
    /usr/bin/python3 "$work/click.py" "$@" >>"$work/click.log" 2>&1
    waited=0
    while kill -0 $satl_pid 2>/dev/null && [ $waited -lt 20 ]; do sleep 0.5; waited=$((waited + 1)); done
    kill $satl_pid 2>/dev/null
    wait $satl_pid 2>/dev/null
    echo $? > "$work/$program.exit"
}
# THE THIRD SECTION, IF IT WRAPPED, is the rest of the height on the left: (320, 690) is 80 across
# and 10 above the window's bottom edge.
clicked wrap 320,690 320,690
# THE INPUT ACROSS THE TOP, THEN THE BOTTOM: (640, 157) is 10 under the titlebar, (640, 690) 10 above
# the bottom edge. The capsule says how far down the section the click that reached it landed.
clicked order 640,157 640,157 640,690
kill $mutter_pid 2>/dev/null; wait $mutter_pid 2>/dev/null
INSIDE
work="$work" satl="$satl" dbus-run-session -- sh "$work/inside.sh" >/dev/null 2>&1

said() { sed -n "$2p" "$work/$1.out"; }
check() {   # check <what> <true or false>
    if [ "$2" = true ]; then echo "ok    $1"; else echo "FAIL  $1"; failed=1; fi
}
is() { [ "$1" ] && [ "$2" ] && [ "$1" -eq "$2" ] 2>/dev/null && echo true || echo false; }

check "the first program exits 0 ($(cat "$work/sections.exit"))" "$(is "$(cat "$work/sections.exit")" 0)"
left_w=$(said sections 1); left_h=$(said sections 2); right_w=$(said sections 3); right_h=$(said sections 4)
check "two 50% x 100% sections side by side: ${left_w}x${left_h} and ${right_w}x${right_h}" \
      "$( [ "$(is "$left_w" "$right_w")" = true ] && [ "$(is "$left_h" "$right_h")" = true ] && [ "${left_w:-0}" -ge 395 ] && [ "${left_w:-0}" -le 400 ] && echo true || echo false)"
check "a file display at 100% x 50% of the left half: $(said sections 5)x$(said sections 6)" \
      "$( [ "$(is "$(said sections 5)" "$left_w")" = true ] && [ "$(is "$(said sections 6)" $((left_h / 2)))" = true ] && echo true || echo false)"
check "a section at 25% of the left half's height: $(said sections 7)" "$(is "$(said sections 7)" $((left_h / 4)))"
check "pixels are pixels: $(said sections 8)x$(said sections 9)" \
      "$( [ "$(is "$(said sections 8)" 300)" = true ] && [ "$(is "$(said sections 9)" 200)" = true ] && echo true || echo false)"
check "50% x 50% of 300 x 200 pixels: $(said sections 10)x$(said sections 11)" \
      "$( [ "$(is "$(said sections 10)" 150)" = true ] && [ "$(is "$(said sections 11)" 100)" = true ] && echo true || echo false)"
check "resized to 1000 x 700, the halves follow: $(said sections 12) and $(said sections 13), the pixels stay $(said sections 14)" \
      "$( [ "$(is "$(said sections 12)" "$(said sections 13)")" = true ] && [ "$(said sections 12)" -gt "${left_w:-0}" ] 2>/dev/null && [ "$(is "$(said sections 14)" 300)" = true ] && echo true || echo false)"

check "the author's program exits 0 ($(cat "$work/his.exit"))" "$(is "$(cat "$work/his.exit")" 0)"
top_w=$(said his 1); top_h=$(said his 2)
check "his file display is 100% x 100% of his 100% x 50% section: $(said his 3)x$(said his 4) in ${top_w}x${top_h}" \
      "$( [ "$(is "$(said his 3)" "$top_w")" = true ] && [ "$(is "$(said his 4)" "$top_h")" = true ] && echo true || echo false)"
check "a second 100% x 50% section goes under the first, as tall: $(said his 5)" "$(is "$(said his 5)" "$top_h")"
check "three 50% sections: two in a row, the third under them, as wide: $(said his 6) and $(said his 7)" \
      "$(is "$(said his 6)" "$(said his 7)")"
check "an input given no size fills its section: $(said his 9) tall in $(said his 8)" \
      "$( [ "${top_h:-0}" -gt 0 ] && [ "$(said his 9)" -ge $(( $(said his 8) - 4 )) ] 2>/dev/null && echo true || echo false)"

check "a real click 10 pixels from the window's bottom, on the left, lands in the third section -- it wrapped, and 100% is the whole window: $(grep '^clicked' "$work/wrap.out" | tr '\n' ' ')" \
      "$( [ "$(grep -c '^clicked three$' "$work/wrap.out")" -ge 1 ] && [ "$(grep -c '^clicked one$\|^clicked two$' "$work/wrap.out")" -eq 0 ] && echo true || echo false)"
check "... and its capsule closed the window, so the run ended with 0 ($(cat "$work/wrap.exit"))" "$(is "$(cat "$work/wrap.exit")" 0)"

down=$(grep -x '[0-9]*' "$work/order.out" | head -1)
check "an input made across the top after a section is above it: a click at the top missed the section, one at the bottom reached it ${down:-?} down" \
      "$( [ "${down:-0}" -gt 300 ] && echo true || echo false)"
check "a window with a section refuses .append, by name ($(cat "$work/append_after.exit"))" \
      "$( [ "$(is "$(cat "$work/append_after.exit")" 27)" = true ] && [ "$(tr '\n' ' ' < "$work/append_after.err" | grep -c 'is laid out in sections')" -ge 1 ] && echo true || echo false)"
check "a window with an appended piece refuses a section, by name ($(cat "$work/section_after.exit"))" \
      "$( [ "$(is "$(cat "$work/section_after.exit")" 13)" = true ] && [ "$(tr '\n' ' ' < "$work/section_after.err" | grep -c 'one or the other')" -ge 1 ] && echo true || echo false)"

if [ $failed -ne 0 ]; then
    echo "--- what satl printed:"; cat "$work/sections.out" "$work/his.out" "$work/wrap.out" "$work/order.out"
    echo "--- its errors:"; grep -v '^$\|frame_timings' "$work/sections.err" "$work/his.err" "$work/wrap.err" "$work/order.err" "$work/append_after.err" "$work/section_after.err" | head -40
    echo "--- the clicker:"; cat "$work/click.log"
fi
rm -rf "$work"
exit $failed
