#!/bin/sh
# SCRATCH.md/FAST_PRINTING/console_race.sh -- STEP 5'S OWN TIMING: `satl --console prog.satl`
# in satl's own window, on a headless mutter of its own (prove-console.sh's way: its own
# Wayland socket, a private session bus, no DISPLAY -- nothing reaches the desktop), two
# builds alternating, best of N. HOW TO MEASURE's loop proves the file and pipe path did not
# get slower; this is the only thing that times the console itself.
#
#     sh SCRATCH.md/FAST_PRINTING/console_race.sh <old satl> <new satl> [rounds] [programs]
#     sh SCRATCH.md/FAST_PRINTING/console_race.sh $S/before/satl build/satl 5 "lines numbers big"
#
# Each satl must have its word libraries beside it (satellite-numbers/), as HOW TO MEASURE's
# copy keeps them. XDG_RUNTIME_DIR must be the real one (mutter makes its socket there).
#
# THE PROGRAMS are SCRATCH.md/DISPLAY_THREADS/bench_{lines,numbers,big}.satl, each with one
# line added at the end: it writes <name>.done. A program that stopped before its end -- S840
# when the console could not keep up, or anything else -- has no .done, and HOLDS its console
# on the machine code until a key nobody here presses: `timeout` ends it, and the row says so.
#
# THE TIME is start to exit. A clean run closes its console at once (satl-term's policy,
# console_launch.cpp), after the last flush has waited for the screen -- so it is how long the
# program and its console took together.
set -u
[ $# -ge 2 ] || { sed -n 2,22p "$0"; exit 1; }
root=$(cd "$(dirname "$0")/../.." && pwd)
old=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
new=$(cd "$(dirname "$2")" && pwd)/$(basename "$2")
rounds=${3:-5}
programs=${4:-lines numbers big}
limit=${CONSOLE_RACE_LIMIT:-240}
[ -x "$old" ] && [ -x "$new" ] || { echo "console_race.sh: both satl binaries must exist"; exit 1; }
[ -d "${XDG_RUNTIME_DIR:-}" ] || { echo "console_race.sh: XDG_RUNTIME_DIR must be the real one"; exit 1; }
work=$(mktemp -d)
[ -n "${KEEP_WORK:-}" ] || trap 'rm -rf "$work"' EXIT
mkdir -p "$work/home/.satl"

for p in $programs; do
    src="$root/SCRATCH.md/DISPLAY_THREADS/bench_$p.satl"
    [ -f "$src" ] || { echo "console_race.sh: no $src"; exit 1; }
    # The last line before satellite.return: the marker that the program reached its end.
    awk -v done="$p.done" '/satellite.return\(satellite\)/ { print "    satellite.file.new(\"" done "\").append(\"printed every line\")" } { print }' \
        "$src" > "$work/$p.satl"
done

cat > "$work/inside.sh" <<'INSIDE'
set -u
cd "$work"
# A socket left by a mutter that did not clean up would pass the wait below at once. The name is ours.
rm -f "$XDG_RUNTIME_DIR/satlconsolerace" "$XDG_RUNTIME_DIR/satlconsolerace.lock"
mutter --headless --virtual-monitor 1280x800 --wayland-display=satlconsolerace >"$work/mutter.log" 2>&1 &
mutter_pid=$!
tries=0
while [ ! -S "$XDG_RUNTIME_DIR/satlconsolerace" ] && [ $tries -lt 60 ]; do sleep 0.25; tries=$((tries + 1)); done
[ -S "$XDG_RUNTIME_DIR/satlconsolerace" ] || { echo "mutter never made its socket"; kill $mutter_pid; exit 1; }
one() { which=$1 satl=$2 p=$3
    rm -f "$work/$p.done"
    env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS -u XAUTHORITY -u SATL_NO_WINDOW HOME="$work/home" \
        WAYLAND_DISPLAY=satlconsolerace /usr/bin/time -f "%e" -o "$work/t" \
        timeout "$limit" "$satl" --console "$work/$p.satl" </dev/null >"$work/$p.$which.out" 2>&1
    status=$?
    reached=no; [ -f "$work/$p.done" ] && reached=yes
    printf '%s %s %s exit=%s reached_the_end=%s\n' "$p" "$which" "$(tail -1 "$work/t")" "$status" "$reached" >> "$work/times"
}
for p in $programs; do
    one old "$old" "$p"                      # once each, not counted: the fonts, the caches
    one new "$new" "$p"
    : > "$work/times.$p"
    r=0
    while [ $r -lt "$rounds" ]; do
        one old "$old" "$p"
        one new "$new" "$p"
        r=$((r + 1))
    done
done
kill $mutter_pid 2>/dev/null; wait $mutter_pid 2>/dev/null
INSIDE

: > "$work/times"
work="$work" old="$old" new="$new" programs="$programs" rounds="$rounds" limit="$limit" \
    dbus-run-session -- sh "$work/inside.sh" 2>/dev/null
# The warm-up rows are the first two of each program; the rest are counted.
awk '
    { key = $1 " " $2; seen[key]++; if (seen[key] == 1) next
      rows[key] = rows[key] " " $3; if ($4 != "exit=0" || $5 != "reached_the_end=yes") bad[key] = bad[key] " [" $3 " " $4 " " $5 "]"
      if (!(key in best) || $3 + 0 < best[key]) best[key] = $3 + 0 }
    END { for (k in best) printf "%-14s best %6.2f s   all:%s%s\n", k, best[k], rows[k], (k in bad) ? "   NOT CLEAN:" bad[k] : "" }
' "$work/times" | sort
