#!/bin/sh
# satellite/satellite_variable_window/prove-bare-machine.sh -- RUN THE REAL satl
# ON A MACHINE THAT HAS NOTHING, and prove that a window, a capsule, a font and
# a PNG all came out of the binary. GTK_AND_NO_DEPENDENCIES.md DEP-3.
#
#     XDG_RUNTIME_DIR=/run/user/1000 sh satellite/satellite_variable_window/prove-bare-machine.sh
#     KEEP_WORK=1 sh ...    keeps the work folder: every stage's output, stderr,
#                           PNG, and binds.txt -- the list of what was bound in
#
# THIS IS NOT IN check.sh AND CANNOT BE, for press-a-button.sh's reason: every
# window row there runs with no display on purpose. This opens windows on a
# headless mutter of its own. Run it by hand; it takes about thirty seconds.
#
# WHY bwrap AND NOT `env -i`, from the hello experiment's bare-machine.sh
# (vendor/gtk-old/hello/, 2026-09-19): clearing the environment proves nothing
# about FILES. With XDG_DATA_DIRS unset glib still falls back to its compiled-in
# /usr/local/share:/usr/share, so this machine's icons, schemas, MIME database
# and fonts are all still there and still found. A binary can pass `env -i` and
# die on a real enterprise box. bwrap gives it an empty root: absence is absence.
#
# WHAT IS BOUND IN, AND WHY EACH ONE IS FAIR. The target is an enterprise Linux
# box with a graphical session and nothing installed on it -- not a machine
# with no hardware and no libc:
#   the binary          /satl/satl, and satellite-numbers/ beside it, because
#                       satl loads its words from /proc/self/exe's folder
#   glibc               libc, libm, ld-linux, /etc/ld.so.cache -- every Linux
#                       has these. libresolv was NEEDED until 2026-09-22 and
#                       bound nothing (DEP-4); it is not here
#   libstdc++, libgcc_s THE DISTRIBUTION'S, from /lib64 -- never this user's
#                       ~/opt/gcc-*. ldd is run with LD_LIBRARY_PATH unset so
#                       the list names what a machine with no such variable
#                       would load. This is the check that a satl built by a
#                       newer compiler still runs on the distro's runtime.
#   libwayland-client, libwayland-egl, and libffi (libwayland's own NEEDED) --
#                       the machine's, because two copies in one process
#                       segfault (Part 1); they cannot be carried
#   the GPU driver      the machine's, by design: libepoxy dlopens it. Found
#                       the way the loader finds it -- glvnd's egl_vendor.d
#                       names the vendor library, ldconfig resolves it, ldd
#                       gives its closure, and mesa's gallium/dri neighbours
#                       and drirc.d come with it. Bound one path at a time, so
#                       the list IS the dependency list and nothing else from
#                       /usr/lib64 is there. Nothing here names AMD -- but what
#                       it covers is a stock mesa behind glvnd: a vendor file
#                       naming an absolute path (NVIDIA's does) is honoured,
#                       and a driver that dlopens libraries no ldd names is
#                       NOT in the closure. That stage then finds no EGL and
#                       the report FAILS, loudly, rather than passing driverless.
#   /dev/dri, /sys      the card, and what mesa reads to identify it
#   the Wayland socket  alone, in a fresh tmpfs XDG_RUNTIME_DIR -- which is
#                       also where satl spills what it carries (WIN-1)
#
# WHAT IS DELIBERATELY ABSENT -- the whole point:
#   /usr/share/icons, /usr/share/themes, /usr/share/glib-2.0/schemas,
#   /usr/share/mime, /usr/share/fonts, /etc/fonts, /usr/share/X11/xkb,
#   /usr/share/locale, /etc/passwd -- and vendor/stage/, whose paths are
#   compiled into fontconfig, gdk-pixbuf and GTK and exist on no other machine.
#   Of /usr/share only the driver's own data folders are bound, and only in
#   the driver stage: drirc.d, glvnd's vendor file, libdrm's ids.
#   D-Bus: no session bus and no accessibility bus (the portal is off since
#          Q-WIN-11a; the a11y bus is Q-WIN-11c, and this shows what it costs)
#   the network: --unshare-net
#
# THE FOUR STAGES, on one headless mutter:
#   outside    the same program on this machine as it is, outside any sandbox
#              -- the reference, and its bare.png is what the sandboxes' PNGs
#              are compared against byte for byte. It is NOT bus-less: with
#              DBUS_SESSION_BUS_ADDRESS unset GLib falls back to the real
#              $XDG_RUNTIME_DIR/bus (press-a-button.sh, trap 2), so this stage
#              reaches the user's real session bus and a11y registry, and that
#              is why it alone prints no a11y warning
#   driver     the bare root above WITH the GPU driver: a window, a label, a
#              canvas written in the carried IBM Plex Mono and saved, a
#              button the program presses itself, the capsule closing the
#              window, the .closed capsule, exit 0
#   no_driver  the same root with NO GPU driver, no /dev/dri and no /sys:
#              what GTK does on a machine whose graphics stack is missing
#   no_display the same root with no Wayland socket at all: S730 NO_DISPLAY,
#              exit 50, a refusal and not a crash
#
# TRAPS, beyond press-a-button.sh's four:
#   - the compositor is named satlbare, not satlwin, so this can run while
#     prove-canvas-tabs-menus.sh runs in another session on the same machine
#   - `ldd` here is run with LD_LIBRARY_PATH unset (see above); with it set,
#     libstdc++ resolves into ~/opt and the proof would bind a compiler's
#     runtime and call it the machine's
#   - bwrap's --unshare-net does not cut the Wayland socket: it is a unix
#     socket bound into the root, and those cross network namespaces
set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
satl="$root/build/satl"
numbers="$root/build/satellite-numbers"
work=$(mktemp -d)
keep=${KEEP_WORK:-}
[ -n "$keep" ] || trap 'rm -rf "$work"' EXIT

[ -x "$satl" ] || { echo "prove-bare-machine.sh: no $satl -- run make first"; exit 1; }
[ -d "$numbers" ] || { echo "prove-bare-machine.sh: no $numbers beside satl"; exit 1; }
for tool in bwrap mutter dbus-run-session ldd ldconfig realpath cmp; do
    command -v $tool >/dev/null || { echo "prove-bare-machine.sh: $tool is not installed"; exit 1; }
done
[ -d "${XDG_RUNTIME_DIR:-}" ] || { echo "prove-bare-machine.sh: XDG_RUNTIME_DIR must be the real one"; exit 1; }
readelf -d "$satl" | grep -q 'libwayland-client' ||
    { echo "prove-bare-machine.sh: $satl does not carry GTK (make GTK=system?) -- nothing to prove"; exit 1; }

cat > "$work/bare.satl" <<'SATL'
satellite.include(satellite)

satellite.capsule when_pressed(satellite.variable.window the_button, satellite.variable.window its_window)
{
    satellite.console.display("the button was pressed on " + its_window.title)
    its_window.close()
}

satellite.capsule when_closed(satellite.variable.window the_window, satellite.variable.window its_window)
{
    satellite.console.display("the window closed")
}

satellite.capsule satellite.main()
{
    satellite.variable.window w = satellite.window.new("bare machine", 640, 480)
    satellite.variable.window l = satellite.window.label("drawn with the carried font")
    satellite.variable.window b = satellite.window.button("press me")
    satellite.variable.window c = satellite.window.canvas(300, 100)
    c.background("#202830")
    c.colour("#40c8ff")
    c.font("IBM Plex Mono", 18)
    c.write(10, 40, "carried IBM Plex Mono")
    c.box(10, 60, 120, 20)
    w.append(l, 320, 60)
    w.append(c, 320, 200)
    w.append(b, 320, 400)
    c.save("bare.png")
    satellite.console.display("saved bare.png, and the canvas measures")
    satellite.console.display(c.width)
    satellite.console.display(c.height)
    b.pressed(when_pressed)
    w.closed(when_closed)
    satellite.console.display("the window is up, and the program presses its own button")
    b.press()
    satellite.return(satellite)
}
SATL

# ---------------------------------------------------------------------------
# WHAT GETS BOUND. Two lists, each a file of paths one per line: `runtime`
# is what satl itself names, `driver` is the machine's graphics stack.
# ---------------------------------------------------------------------------

# satl's own NEEDED, resolved as a machine with no LD_LIBRARY_PATH resolves
# them, and each one's own closure (libwayland-client brings libffi).
env -u LD_LIBRARY_PATH ldd "$satl" | awk '/=> \//{print $3} /^[[:space:]]*\/lib64\/ld-linux/{print $1}' \
    | sort -u > "$work/runtime.list"
[ -s "$work/runtime.list" ] || { echo "prove-bare-machine.sh: ldd named nothing for $satl"; exit 1; }
if grep -q "$HOME/" "$work/runtime.list"; then
    echo "prove-bare-machine.sh: satl resolves a library inside $HOME even with LD_LIBRARY_PATH unset:"
    grep "$HOME/" "$work/runtime.list"
    echo "  that is an RPATH (LD_RUN_PATH bakes one) and the proof would be lying -- stopping"
    exit 1
fi

# The GPU driver, found the way the loader finds it. glvnd's dispatcher
# libraries first; then every vendor file names a library by SONAME, which
# ldconfig turns into a path; then that library's closure, and mesa's
# gallium/dri neighbours and its two data folders beside it.
driver_roots() {
    for p in /lib64/libEGL.so.1 /lib64/libGLdispatch.so.0 /lib64/libGLESv2.so.2 \
             /usr/share/glvnd /etc/glvnd; do
        [ -e "$p" ] && echo "$p"
    done
    for json in /usr/share/glvnd/egl_vendor.d/*.json /etc/glvnd/egl_vendor.d/*.json; do
        [ -e "$json" ] || continue
        name=$(sed -n 's/.*"library_path" *: *"\([^"]*\)".*/\1/p' "$json")
        # a SONAME is looked up the way the loader does it; an absolute path
        # (NVIDIA's vendor file gives one) is taken as it is
        case "$name" in
            /*) lib=$name ;;
            *)  lib=$(ldconfig -p | awk -v n="$name" '$1 == n { print $NF; exit }') ;;
        esac
        [ -n "$lib" ] && [ -e "$lib" ] || continue
        echo "$lib"
        dir=$(dirname "$lib")
        for p in "$dir"/libgallium*.so "$dir"/dri "$dir"/libgbm.so.1 "$dir"/gbm \
                 "$dir"/../share/drirc.d "$dir"/../share/libdrm /usr/share/drirc.d /usr/share/libdrm; do
            [ -e "$p" ] && realpath -m "$p"
        done
    done
}
driver_roots | sort -u > "$work/driver.roots"
# ...and the closure of every library in it, one ldd each. ONE PASS IS THE
# FIXED POINT: ldd already prints the whole transitive closure, and a loop that
# reads a file while appending to it reads its own appends too (measured).
cp "$work/driver.roots" "$work/driver.list"
while read -r p; do
    case "$p" in *.so*) [ -f "$p" ] || continue ;; *) continue ;; esac
    env -u LD_LIBRARY_PATH ldd "$p" 2>/dev/null | awk '/=> \//{print $3}'
done < "$work/driver.list" >> "$work/driver.list"
sort -u "$work/driver.list" -o "$work/driver.list"
# mesa dlopens the gallium driver out of its dri folder, whose members are
# not in any ldd: their closure is taken too.
if [ -d "$(grep '/dri$' "$work/driver.list" | head -1)" ]; then
    for so in "$(grep '/dri$' "$work/driver.list" | head -1)"/*.so; do
        [ -f "$so" ] && env -u LD_LIBRARY_PATH ldd "$so" 2>/dev/null | awk '/=> \//{print $3}'
    done >> "$work/driver.list"
    sort -u "$work/driver.list" -o "$work/driver.list"
fi
# What satl's own runtime already binds is not the driver's to bind again.
grep -vxF -f "$work/runtime.list" "$work/driver.list" > "$work/driver.only" || true

binds_from() {   # binds_from <list> -> a sh fragment that appends --ro-bind pairs to $@
    while read -r p; do
        [ -e "$p" ] || continue
        q=$(printf '%s' "$p" | sed "s/'/'\\\\''/g")          # a path is quoted, however it is spelled
        printf 'set -- "$@" --ro-bind '\''%s'\'' '\''%s'\''\n' "$q" "$q"
    done < "$1"
}
binds_from "$work/runtime.list" > "$work/binds-runtime.sh"
binds_from "$work/driver.only" > "$work/binds-driver.sh"
{
    echo "# what prove-bare-machine.sh bound into the empty root, $(date '+%Y-%m-%d %H:%M')"
    echo "# satl's runtime ($(wc -l < "$work/runtime.list") paths):"; sed 's/^/    /' "$work/runtime.list"
    echo "# the GPU driver ($(wc -l < "$work/driver.only") paths):"; sed 's/^/    /' "$work/driver.only"
    echo "# and always: /satl/satl, /satl/satellite-numbers, /etc/ld.so.cache, /proc, /dev, /sys (driver stage), a tmpfs /tmp and runtime dir"
} > "$work/binds.txt"

cat > "$work/inside.sh" <<'INSIDE'
set -u
mutter --headless --virtual-monitor 1280x800 --wayland-display=satlbare >"$work/mutter.log" 2>&1 &
mutter_pid=$!
tries=0
while [ ! -S "$XDG_RUNTIME_DIR/satlbare" ] && [ $tries -lt 60 ]; do sleep 0.25; tries=$((tries + 1)); done
[ -S "$XDG_RUNTIME_DIR/satlbare" ] || { echo "mutter never made its socket"; kill $mutter_pid 2>/dev/null; exit 1; }

# THE REFERENCE: this machine as it is, on the headless compositor, with a HOME
# of its own. DBUS_SESSION_BUS_ADDRESS is unset as every other proof does, and
# that does NOT make it bus-less: GLib falls back to the real $XDG_RUNTIME_DIR/bus
# (press-a-button.sh, trap 2). The sandboxes below are the bus-less runs.
mkdir -p "$work/outside.home/.satl"; cp "$work/bare.satl" "$work/outside.home/"
( cd "$work/outside.home" && HOME="$work/outside.home" "$satl" --rebuild >/dev/null 2>&1
  cd "$work/outside.home" && env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS HOME="$work/outside.home" WAYLAND_DISPLAY=satlbare \
      timeout 60 "$satl" bare.satl > "$work/outside.out" 2> "$work/outside.err"; echo "exit $?" > "$work/outside.exit" )

# ONE SANDBOX, THREE WAYS. run_stage <name> <driver: yes|no> <display: yes|no>
run_stage() {
    name=$1; driver=$2; display=$3
    mkdir -p "$work/$name.home/.satl"; cp "$work/bare.satl" "$work/$name.home/"
    set -- bwrap --unshare-user --unshare-pid --unshare-ipc --unshare-uts --unshare-net --die-with-parent \
        --tmpfs / --proc /proc --dev /dev --tmpfs /tmp \
        --bind "$work/$name.home" /scratch \
        --ro-bind "$satl" /satl/satl --ro-bind "$numbers" /satl/satellite-numbers \
        --ro-bind /etc/ld.so.cache /etc/ld.so.cache \
        --tmpfs /run/user-scratch \
        --clearenv --setenv HOME /scratch --setenv TMPDIR /tmp --setenv PATH /usr/bin \
        --setenv XDG_RUNTIME_DIR /run/user-scratch --chdir /scratch \
        --setenv GSK_DEBUG renderer --setenv GDK_DEBUG opengl
    . "$work/binds-runtime.sh"
    if [ "$driver" = yes ]; then
        . "$work/binds-driver.sh"
        set -- "$@" --dev-bind /dev/dri /dev/dri --ro-bind /sys /sys
    fi
    if [ "$display" = yes ]; then
        set -- "$@" --ro-bind "$XDG_RUNTIME_DIR/satlbare" /run/user-scratch/satlbare --setenv WAYLAND_DISPLAY satlbare
    fi
    "$@" /satl/satl --rebuild > "$work/$name.rebuild" 2>&1
    timeout 60 "$@" /satl/satl /scratch/bare.satl > "$work/$name.out" 2> "$work/$name.err"
    echo "exit $?" > "$work/$name.exit"
}
run_stage driver     yes yes
run_stage no_driver  no  yes
run_stage no_display no  no

kill $mutter_pid 2>/dev/null; wait $mutter_pid 2>/dev/null
INSIDE

work="$work" satl="$satl" numbers="$numbers" dbus-run-session -- sh "$work/inside.sh"

# ---------------------------------------------------------------------------
# THE REPORT. Every line satl was expected to print, counted; the PNG compared
# byte for byte with the reference; the exit status of each stage.
# ---------------------------------------------------------------------------
want_lines='saved bare.png, and the canvas measures|300|100|the window is up, and the program presses its own button|the button was pressed on bare machine|the window closed|'
lines_of() { grep -E '^(saved bare.png, and the canvas measures|300|100|the window is up, and the program presses its own button|the button was pressed on bare machine|the window closed)$' "$work/$1.out" 2>/dev/null | tr '\n' '|'; }
status_of() { cat "$work/$1.exit" 2>/dev/null || echo "exit ?"; }
png_of() { if [ -f "$work/$1.home/bare.png" ] && cmp -s "$work/outside.home/bare.png" "$work/$1.home/bare.png"; then echo identical; elif [ -f "$work/$1.home/bare.png" ]; then echo DIFFERENT; else echo missing; fi; }
criticals_of() { grep -c 'CRITICAL\|WARNING' "$work/$1.err" 2>/dev/null; }
renderer_of() { sed -n "s/^Using renderer '\([A-Za-z]*\)' for surface.*/\1/p" "$work/$1.err" 2>/dev/null | head -1; }
egl_of() { grep -c '^EGL API version' "$work/$1.err" 2>/dev/null; }

outside_status=$(status_of outside); outside_lines=$(lines_of outside)
driver_status=$(status_of driver); driver_lines=$(lines_of driver); driver_png=$(png_of driver)
driver_egl=$(egl_of driver); driver_renderer=$(renderer_of driver)
nodriver_status=$(status_of no_driver); nodriver_lines=$(lines_of no_driver); nodriver_png=$(png_of no_driver)
nodriver_renderer=$(renderer_of no_driver); nodriver_no_egl=$(grep -c '^Not using GL: libEGL not available' "$work/no_driver.err" 2>/dev/null)
nodisplay_status=$(status_of no_display)
nodisplay_refused=$(grep -c 'S730: NO_DISPLAY' "$work/no_display.err" 2>/dev/null)
runtime_n=$(wc -l < "$work/runtime.list"); driver_n=$(wc -l < "$work/driver.only")

echo "---------------------------------------------------------------"
echo "THIS MACHINE AS IT IS, on a headless compositor (the reference)"
echo "  satl $outside_status"
echo "  $outside_lines"
echo "AN EMPTY ROOT: satl, glibc, the distro's libstdc++, libwayland, and the GPU driver"
echo "  bound: $runtime_n runtime paths and $driver_n driver paths -- binds.txt names every one"
echo "  satl $driver_status        (0 -- the capsule closed the window)"
echo "  $driver_lines"
echo "  bare.png against the reference:     $driver_png   (want identical -- the carried font, found)"
echo "  EGL found through the driver:       $driver_egl   (want 1), and GTK drew with $driver_renderer"
echo "  GLib criticals and warnings on stderr: $(criticals_of driver)   ($work/driver.err)"
echo "THE SAME ROOT WITH NO GPU DRIVER, no /dev/dri and no /sys"
echo "  satl $nodriver_status"
echo "  $nodriver_lines"
echo "  bare.png against the reference:     $nodriver_png"
echo "  GTK said libEGL was not available:  $nodriver_no_egl   (want 1), and drew with $nodriver_renderer   (want GskCairoRenderer)"
echo "  GLib criticals and warnings on stderr: $(criticals_of no_driver)   ($work/no_driver.err)"
echo "THE SAME ROOT WITH NO DISPLAY AT ALL"
echo "  satl $nodisplay_status        (50 -- S730 NO_DISPLAY, a refusal and not a crash)"
echo "  refused with S730:                  $nodisplay_refused   (want 1)"
[ -n "$keep" ] && echo "the work folder is kept: $work"
echo "---------------------------------------------------------------"

[ "$outside_status" = "exit 0" ] && [ "$outside_lines" = "$want_lines" ] &&
[ "$driver_status" = "exit 0" ] && [ "$driver_lines" = "$want_lines" ] && [ "$driver_png" = identical ] && [ "$driver_egl" = 1 ] &&
[ "$nodriver_status" = "exit 0" ] && [ "$nodriver_lines" = "$want_lines" ] && [ "$nodriver_png" = identical ] &&
[ "$nodriver_no_egl" = 1 ] && [ "$nodriver_renderer" = GskCairoRenderer ] &&
[ "$nodisplay_status" = "exit 50" ] && [ "$nodisplay_refused" = 1 ] || {
    echo "prove-bare-machine.sh: FAILED"; exit 1; }
echo "prove-bare-machine.sh: satl drew a window, ran a capsule, found its carried font and wrote a PNG in a root that had nothing but glibc, libwayland and the GPU driver -- and again with no driver, and refused cleanly with no display"
