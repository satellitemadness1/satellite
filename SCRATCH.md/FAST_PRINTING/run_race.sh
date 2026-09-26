#!/bin/sh
# SCRATCH.md/FAST_PRINTING/run_race.sh -- runs build/vte_race/vte_print_race on a
# headless mutter of its own (prove-console.sh's way): its own Wayland socket and a
# private session bus, so no window reaches the desktop. About a minute.
#     sh SCRATCH.md/FAST_PRINTING/run_race.sh
# The results land in build/vte_race/race.err.
#
# THE FONTS AND KEYBOARD DATA ARE satl's OWN, from the folder satl's console unpacks
# them into (window_spill.cpp). If there is none, run prove-console.sh, which makes it -- never open
# the console on the author's desktop for it.
set -u
root=$(cd "$(dirname "$0")/../.." && pwd)
out="$root/build/vte_race"
runtime=/run/user/$(id -u)
[ -x "$out/vte_print_race" ] || { echo "run build_race.sh first"; exit 1; }
spill=$(ls -d "$runtime"/satl-window-* 2>/dev/null | tail -1)
[ -n "$spill" ] || { echo "no $runtime/satl-window-* -- run satellite/satellite_variable_window/prove-console.sh, which makes it (never open the console on the desktop)"; exit 1; }
cd "$out"
cat > inside.sh <<INSIDE
# A socket left by a mutter that did not clean up would pass the wait below at once, and the
# race would find nobody there ("Failed to open display"). The name is the race's own.
rm -f "\$XDG_RUNTIME_DIR/satlrace" "\$XDG_RUNTIME_DIR/satlrace.lock"
mutter --headless --virtual-monitor 1280x800 --wayland-display=satlrace > mutter.log 2>&1 &
mpid=\$!
tries=0
while [ ! -S "\$XDG_RUNTIME_DIR/satlrace" ] && [ \$tries -lt 60 ]; do sleep 0.25; tries=\$((tries+1)); done
[ -S "\$XDG_RUNTIME_DIR/satlrace" ] || { echo "mutter never made its socket"; kill \$mpid; exit 1; }
env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS -u XAUTHORITY WAYLAND_DISPLAY=satlrace GDK_BACKEND=wayland \
    GDK_DEBUG=no-portals${RACE_GDK_DEBUG:+,$RACE_GDK_DEBUG} XKB_CONFIG_ROOT=$spill/xkb FONTCONFIG_FILE=$spill/fonts.conf FONTCONFIG_PATH=$spill \
    GSETTINGS_SCHEMA_DIR=$spill/schemas timeout 600 ./vte_print_race < /dev/null > race.stdout 2> race.err
echo "race exit \$?"
kill \$mpid
INSIDE
XDG_RUNTIME_DIR=$runtime dbus-run-session -- sh inside.sh 2>/dev/null
grep -v "^A connection to the bus" race.err
