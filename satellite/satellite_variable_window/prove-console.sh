#!/bin/sh
# satellite/satellite_variable_window/prove-console.sh -- A CONSOLE satl LAUNCHES
# FOR ITSELF, AND A CONSOLE A PROGRAM MAKES, on a compositor of their own.
# GTK_AND_NO_DEPENDENCIES.md GTK-17, built 2026-09-22.
#
# THIS IS NOT IN check.sh AND CANNOT BE, for press-a-button.sh's reason: every
# window row there runs with no display on purpose. This opens consoles on a
# headless mutter, types into them with real keys through mutter's own
# RemoteDesktop, and checks what satl left behind AFTER it exited. Run it by
# hand; it takes about a minute.
#
#     XDG_RUNTIME_DIR=/run/user/1000 sh satellite/satellite_variable_window/prove-console.sh
#     KEEP_WORK=1 sh ...    keeps the work folder
#
# WHAT IT PROVES, in four stages -- and what none of them can: nobody here can
# read the screen, so what is asserted is what went THROUGH the pty and came
# out the other side as a file, an exit status, or a line on satl's stdout.
#
#   prompt   `satl --console`: the prompt runs in a window of satl's own. Real
#            keys type a line that writes a file and Return; the file appears,
#            so the keys went VTE -> pty -> satl's stdin -> the session. `exit`
#            ends the session, the console HOLDS with its message, one more key
#            closes it, and satl exits 0.
#   stopped  `satl --console fail.satl`: a program prints a line and then stops
#            on a refused line. The console holds with the machine code on it;
#            a key closes it; satl exits with THAT code -- the exit status
#            survives the window, which is the thing WIN-9 said a handover
#            loses and the reason this is one process.
#   clean    `satl --console clean.satl`: a program writes a file and finishes.
#            The console closes ITSELF, no key pressed, and satl exits 0 within
#            a few seconds -- satl-term's policy for a file that ran.
#   piece    a PROGRAM's console: satellite.console.new, .display into it,
#            .typed(when_typed); real keys type "abc" and Return; the capsule
#            reads .typed and .columns and closes the window. The kernel did
#            the echo and the line, the desk read it, the interpreter ran it.
#            AND IT STARTS IN arguments.directory.default (2026-09-22): satl
#            runs here with a home of its own whose config.ini sets
#            `directory.default` to $work/where, and the typed line's file must
#            land THERE -- which also keeps this proof out of the author's
#            real default folder.
#   bare     PLAIN `satl`, NO FLAG, started the way a launcher starts it: no
#            controlling terminal (setsid), stdin and stdout on /dev/null. It
#            must take you to its prompt in a console of its own -- WIN-9, the
#            author, 2026-09-22 -- and it is typed at, exits 0 and holds.
#   loud     THE DEADLOCK A FRESH READER FOUND (2026-09-22): 3000 lines of
#            4000 characters printed into satl's own console while GDK_DEBUG
#            makes the desk print a line on every frame. Before the fix the
#            desk blocked on the full pty that only it drains, and satl hung
#            with nothing printed; now the desk's lines land where satl was
#            started (loud.out) and satl exits 0 by itself.
#
# THE HOLD IS PROVED BY LIVENESS: after each stage's pause and BEFORE the closing
# key is sent, satl must still be alive (a build that did not hold would have
# exited already and the key would land on nothing). stopped.out must be empty
# too: the report went to the pty, not to where satl was started.
#
# THE TRAPS ARE press-a-button.sh's and prove-canvas-tabs-menus.sh's; two are
# load-bearing here: the first key of a RemoteDesktop session is swallowed, so
# every sequence starts with a harmless Shift; and `wait $pid` on a process
# started inside $(...) answers 127, so satl is started in the current shell.
#
# satl IS NOT A PROCESS GROUP LEADER HERE -- a non-interactive sh has no job
# control -- so setsid() succeeds and the pty becomes satl's controlling
# terminal: this is the launcher's path (console_launch.cpp). The shell path,
# where satl already leads a group and Ctrl-C is bridged by hand, is not driven
# here; it needs an interactive shell to start from.
set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
satl="$root/build/satl"
work=$(mktemp -d)
keep=${KEEP_WORK:-}
[ -n "$keep" ] || trap 'rm -rf "$work"' EXIT

[ -x "$satl" ] || { echo "prove-console.sh: no $satl -- run make first"; exit 1; }
command -v mutter >/dev/null || { echo "prove-console.sh: mutter is not installed"; exit 1; }
command -v dbus-run-session >/dev/null || { echo "prove-console.sh: dbus-run-session is not installed"; exit 1; }
[ -d "${XDG_RUNTIME_DIR:-}" ] || { echo "prove-console.sh: XDG_RUNTIME_DIR must be the real one"; exit 1; }

cat > "$work/fail.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.console.display("before the stop")
    satellite.console.display(1 / 0)
    satellite.return(satellite)
}
SATL

cat > "$work/clean.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.file.new("clean.se").append("the program ran in a console and finished")
    satellite.console.display("done")
    satellite.return(satellite)
}
SATL

# 3000 LINES OF 4000 CHARACTERS: twelve megabytes through a pty that holds
# twelve kilobytes, so the interpreter is waiting for room most of the run.
line=$(printf 'x%.0s' $(seq 1 4000))
cat > "$work/loud.satl" <<SATL
satellite.include(satellite)

satellite.capsule satellite.main()
{
    satellite.variable.string s = "$line"
    satellite.statement.for(satellite.variable.number i = 1; i < 3001; i + 1)
    {
        satellite.console.display(s)
    }
    satellite.file.new("loud.se").append("printed every line")
    satellite.return(satellite)
}
SATL

cat > "$work/piece.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_typed(satellite.variable.window the_console)
{
    satellite.console.display("typed: " + the_console.typed)
    satellite.console.display(the_console.columns)
    satellite.console.display(the_console.rows)
    the_console.close()
}

satellite.capsule satellite.main()
{
    satellite.variable.window c = satellite.console.new("a program's console", 700, 400)
    c.display("type a line and press Enter")
    c.typed(when_typed)
    satellite.console.display("main is finished, and the console is waiting")
    satellite.return(satellite)
}
SATL

# THE DRIVER: prove-canvas-tabs-menus.sh's, one process and one RemoteDesktop
# session. "type:some text" types it a character at a time; any other word is a
# key by name; "x,y" is a click.
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
KEYS = {'escape': 0xff1b, 'return': 0xff0d, 'space': 0x020, 'shift': 0xffe1}

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

for step in sys.argv[1:]:
    if step.startswith('type:'):
        for ch in step[5:]:
            session.NotifyKeyboardKeysym('(ub)', ord(ch), True)
            time.sleep(0.03)
            session.NotifyKeyboardKeysym('(ub)', ord(ch), False)
            time.sleep(0.05)
        time.sleep(0.3)
    elif step.startswith('wait:'):
        time.sleep(float(step[5:]))
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
mutter --headless --virtual-monitor 1280x800 --wayland-display=satlcon >"$work/mutter.log" 2>&1 &
mutter_pid=$!
tries=0
while [ ! -S "$XDG_RUNTIME_DIR/satlcon" ] && [ $tries -lt 60 ]; do sleep 0.25; tries=$((tries + 1)); done
[ -S "$XDG_RUNTIME_DIR/satlcon" ] || { echo "mutter never made its socket"; kill $mutter_pid 2>/dev/null; exit 1; }

# NO SESSION BUS FOR satl, NO DISPLAY EITHER -- press-a-button.sh's traps 1 and 2.
# stdin IS /dev/null ON PURPOSE: that is exactly what a launcher gives, and the
# console must not need anything else. `$@` after the name is the command line.
# A HOME OF satl'S OWN, whose config.ini names the folder the prompt starts in.
mkdir -p "$work/home/.satl" "$work/where"
printf 'directory.default = %s\n' "$work/where" > "$work/home/.satl/config.ini"
start() { name=$1; shift
    env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS HOME="$work/home" WAYLAND_DISPLAY=satlcon "$satl" "$@" </dev/null >"$work/$name.out" 2>&1 & pid=$!; }
alive() { kill -0 $pid 2>/dev/null && echo alive > "$work/$1.alive"; }
finish() { waited=0
    while kill -0 $pid 2>/dev/null && [ $waited -lt 60 ]; do sleep 0.5; waited=$((waited + 1)); done
    if kill -0 $pid 2>/dev/null; then echo "$1: satl did not exit -- the console was never closed" >> "$work/$1.out"; kill $pid 2>/dev/null; fi
    wait $pid 2>/dev/null; echo "exit $?" > "$work/$1.exit"; }

# THE PROMPT. A Shift first (swallowed), a line that writes a file, Return,
# `exit`, Return, a pause for the hold to be up, and a Space to close it.
start prompt --console; sleep 5
/usr/bin/python3 "$work/drive.py" shift 'type:satellite.file.new("typed.se").append("hello from the console")' return \
    wait:2 'type:exit' return wait:3 >"$work/prompt.drive" 2>&1
alive prompt
# A SECOND SESSION, SO A SECOND SWALLOWED FIRST KEY: Shift before the Space.
/usr/bin/python3 "$work/drive.py" shift space >"$work/prompt.drive2" 2>&1
finish prompt

# PLAIN satl, AS A LAUNCHER STARTS IT: no flag, no controlling terminal, stdin
# and stdout on /dev/null, SATL_NO_WINDOW unset. setsid -w so that a terminal
# this proof was started from is not satl's, and so $! is satl's exit.
env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS -u SATL_NO_WINDOW HOME="$work/home" WAYLAND_DISPLAY=satlcon \
    setsid -w "$satl" </dev/null >/dev/null 2>"$work/bare.err" & pid=$!
sleep 5
/usr/bin/python3 "$work/drive.py" shift 'type:satellite.file.new("bare.se").append("a launcher reached the prompt")' return \
    wait:2 'type:exit' return wait:3 >"$work/bare.drive" 2>&1
alive bare
/usr/bin/python3 "$work/drive.py" shift space >"$work/bare.drive2" 2>&1
finish bare

# A PROGRAM THAT STOPS. Nothing to type until it has: the console holds with
# the code on it, and one key closes it.
start stopped --console "$work/fail.satl"; sleep 5
alive stopped
/usr/bin/python3 "$work/drive.py" shift space >"$work/stopped.drive" 2>&1
finish stopped

# A PROGRAM THAT FINISHES. No keys at all: the console closes itself.
start clean --console "$work/clean.satl"
finish clean

# A PROGRAM'S OWN CONSOLE, typed into. The window has the focus, being the only
# one; Shift first, then the line and Return.
start piece "$work/piece.satl"; sleep 5
/usr/bin/python3 "$work/drive.py" shift 'type:abc' return >"$work/piece.drive" 2>&1
finish piece

# THE LOUD PROGRAM, WITH THE DESK MADE TO TALK. No keys: it closes itself, or it
# never does. G_ENABLE_DEBUG is on in the vendored GTK, so GDK_DEBUG=frames
# prints from the desk on every frame; a release GTK prints nothing and the
# stage still proves the run ends.
loud_start() { env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS HOME="$work/home" WAYLAND_DISPLAY=satlcon GDK_DEBUG=frames "$satl" --console "$work/loud.satl" </dev/null >"$work/loud.out" 2>&1 & pid=$!; }
loud_start
finish loud

kill $mutter_pid 2>/dev/null; wait $mutter_pid 2>/dev/null
INSIDE

work="$work" satl="$satl" dbus-run-session -- sh "$work/inside.sh"

prompt_status=$(cat "$work/prompt.exit" 2>/dev/null || echo "exit ?")
prompt_typed=$(cat "$work/where/typed.se" 2>/dev/null | tr '\n' '|')
prompt_hung=$(grep -c 'satl did not exit' "$work/prompt.out" 2>/dev/null); [ -n "$prompt_hung" ] || prompt_hung=0
prompt_stdout=$(wc -c < "$work/prompt.out" 2>/dev/null || echo ?)
stopped_status=$(cat "$work/stopped.exit" 2>/dev/null || echo "exit ?")
stopped_hung=$(grep -c 'satl did not exit' "$work/stopped.out" 2>/dev/null); [ -n "$stopped_hung" ] || stopped_hung=0
clean_status=$(cat "$work/clean.exit" 2>/dev/null || echo "exit ?")
clean_wrote=$(cat "$work/clean.se" 2>/dev/null | tr '\n' '|')
clean_hung=$(grep -c 'satl did not exit' "$work/clean.out" 2>/dev/null); [ -n "$clean_hung" ] || clean_hung=0
prompt_alive=$(cat "$work/prompt.alive" 2>/dev/null || echo dead)
stopped_alive=$(cat "$work/stopped.alive" 2>/dev/null || echo dead)
stopped_stdout=$(wc -c < "$work/stopped.out" 2>/dev/null || echo ?)
bare_status=$(cat "$work/bare.exit" 2>/dev/null || echo "exit ?")
bare_wrote=$(cat "$work/where/bare.se" 2>/dev/null | tr '\n' '|')
bare_alive=$(cat "$work/bare.alive" 2>/dev/null || echo dead)
loud_status=$(cat "$work/loud.exit" 2>/dev/null || echo "exit ?")
loud_wrote=$(cat "$work/loud.se" 2>/dev/null | tr '\n' '|')
loud_desk_lines=$(grep -c 'Gdk-Message' "$work/loud.out" 2>/dev/null); [ -n "$loud_desk_lines" ] || loud_desk_lines=0
loud_hung=$(grep -c 'satl did not exit' "$work/loud.out" 2>/dev/null); [ -n "$loud_hung" ] || loud_hung=0
piece_status=$(cat "$work/piece.exit" 2>/dev/null || echo "exit ?")
piece_lines=$(grep -E '^(main is finished, and the console is waiting|typed: abc)$' "$work/piece.out" 2>/dev/null | tr '\n' '|')
piece_cells=$(grep -A2 '^typed: abc$' "$work/piece.out" 2>/dev/null | tail -2 | tr '\n' ' ')

echo "---------------------------------------------------------------"
echo "THE PROMPT, IN A CONSOLE OF satl'S OWN, TYPED AT WITH REAL KEYS"
echo "  satl $prompt_status        (0 -- exit ended the session, a key closed the hold)"
echo "  the typed line wrote, in the default folder: $prompt_typed  (want hello from the console|)"
echo "  bytes on satl's ORIGINAL stdout:    $prompt_stdout   (want 0 -- everything went to the pty)"
echo "  still alive when the key was sent:  $prompt_alive   (want alive -- the hold was up)"
echo "  satl hung:                          $prompt_hung   (want 0)"
echo "PLAIN satl, STARTED AS A LAUNCHER STARTS IT: IT TAKES YOU TO ITS PROMPT"
echo "  satl $bare_status        (0 -- exit ended the session, a key closed the hold)"
echo "  the typed line wrote:               $bare_wrote  (want a launcher reached the prompt|)"
echo "  still alive when the key was sent:  $bare_alive   (want alive)"
echo "A PROGRAM THAT STOPPED, IN A CONSOLE: THE CODE SURVIVES THE WINDOW"
echo "  satl $stopped_status        (22 -- the refused line's own code, through the window)"
echo "  still alive when the key was sent:  $stopped_alive   (want alive -- the hold was up)"
echo "  bytes on satl's ORIGINAL stdout:    $stopped_stdout   (want 0 -- the report went to the pty)"
echo "  satl hung:                          $stopped_hung   (want 0 -- one key closed the hold)"
echo "A PROGRAM THAT FINISHED, IN A CONSOLE: IT CLOSES ITSELF"
echo "  satl $clean_status        (0 -- no key was pressed)"
echo "  the program wrote:                  $clean_wrote"
echo "  satl hung:                          $clean_hung   (want 0)"
echo "A PROGRAM'S OWN CONSOLE, A LINE TYPED INTO IT"
echo "  satl $piece_status        (0 -- the capsule closed the window)"
echo "  $piece_lines  (want main is finished..., then typed: abc)"
echo "  .columns and .rows in the capsule:  $piece_cells  (want two numbers, not 80 24 by luck)"
echo "TWELVE MEGABYTES INTO satl'S OWN CONSOLE WHILE THE DESK TALKS ON EVERY FRAME"
echo "  satl $loud_status        (0 -- it finished and closed itself; before the fix it hung)"
echo "  the program wrote:                  $loud_wrote"
echo "  the desk's lines, where satl started: $loud_desk_lines   (more than 0 on this GTK; none on a release GTK)"
echo "  satl hung:                          $loud_hung   (want 0)"
[ -n "$keep" ] && echo "the work folder is kept: $work"
echo "---------------------------------------------------------------"

piece_cells_ok=0; echo "$piece_cells" | grep -qE '^[0-9]+ [0-9]+ $' && piece_cells_ok=1
[ "$prompt_status" = "exit 0" ] && [ "$prompt_typed" = "hello from the console|" ] && [ "$prompt_stdout" = "0" ] &&
[ "$prompt_alive" = "alive" ] && [ "$prompt_hung" = "0" ] &&
[ "$stopped_status" = "exit 22" ] && [ "$stopped_hung" = "0" ] && [ "$stopped_alive" = "alive" ] && [ "$stopped_stdout" = "0" ] &&
[ "$clean_status" = "exit 0" ] && [ "$clean_wrote" = "the program ran in a console and finished|" ] && [ "$clean_hung" = "0" ] &&
[ "$piece_status" = "exit 0" ] && [ "$piece_lines" = "main is finished, and the console is waiting|typed: abc|" ] && [ "$piece_cells_ok" = "1" ] &&
[ "$loud_status" = "exit 0" ] && [ "$loud_wrote" = "printed every line|" ] && [ "$loud_hung" = "0" ] &&
[ "$bare_status" = "exit 0" ] && [ "$bare_wrote" = "a launcher reached the prompt|" ] && [ "$bare_alive" = "alive" ] &&
{ echo "prove-console.sh: PASS"; exit 0; }
echo "prove-console.sh: FAIL"
exit 1
