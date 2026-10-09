# vendor/hello-vte/build-and-run.sh -- compile hello-vte against the vendored stack, check its NEEDED,
# run it on a headless mutter of its own (satlbare), and show what it printed.
set -u
root=$(cd "$(dirname "$0")/../.." && pwd)
here=$(cd "$(dirname "$0")" && pwd)
stage=$root/vendor/stage; gtkb=$root/vendor/build/gtk
export PKG_CONFIG_LIBDIR=$gtkb/meson-uninstalled:$stage/lib/pkgconfig:$stage/lib64/pkgconfig:$stage/share/pkgconfig:$stage/pkgconfig-system
cflags=$(env -u PKG_CONFIG_PATH pkg-config --cflags gtk4 vte-2.91-gtk4) || { echo "pkg-config failed"; exit 1; }
cxx=$HOME/opt/clang-current/bin/clang++
cc=$HOME/opt/clang-current/bin/clang
cd "$here"
$cc -std=gnu11 -O1 $cflags -c hello-vte.c -o hello-vte.o || exit 1
env -u LD_RUN_PATH $cxx -fuse-ld=lld hello-vte.o -o hello-vte \
    -ldl $stage/lib/libvte-2.91-gtk4.a \
    -Wl,--start-group $gtkb/gtk/libgtk.a $gtkb/gdk/libgdk.a $gtkb/gdk/wayland/libgdk-wayland.a \
    $gtkb/gsk/libgsk.a $gtkb/gsk/libgsk_f16c.a $gtkb/gtk/css/libgtk_css.a $gtkb/gtk/svg/libgtk_svg.a \
    $stage/lib64/*.a $stage/lib/*.a -Wl,--end-group \
    -lm -lpthread -lrt -lwayland-client -lwayland-egl || exit 1
ls -la hello-vte; echo "NEEDED:"; readelf -d hello-vte | grep NEEDED | sed 's/.*\[\(.*\)\]/  \1/' | tr '\n' ' '; echo
cat > inside.sh <<'INSIDE'
set -u
mutter --headless --virtual-monitor 1280x800 --wayland-display=satlbare >"$here/mutter.log" 2>&1 &
mutter_pid=$!
tries=0; while [ ! -S "$XDG_RUNTIME_DIR/satlbare" ] && [ $tries -lt 60 ]; do sleep 0.25; tries=$((tries + 1)); done
[ -S "$XDG_RUNTIME_DIR/satlbare" ] || { echo "mutter never made its socket"; kill $mutter_pid; exit 1; }
# WHAT satl SPILLS BEFORE gtk_init (WIN-1), pointed at the stage instead: the vendored GTK
# looks for xkb data at /nonexistent on purpose, so a program that does not say where it is
# SIGSEGVs inside gtk_init -- measured here first, exit 139, five xkbcommon errors.
env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS WAYLAND_DISPLAY=satlbare \
    XKB_CONFIG_ROOT="$stage/share/X11/xkb" GSETTINGS_SCHEMA_DIR="$gtkb/gtk" FONTCONFIG_FILE="$stage/etc/fonts/fonts.conf" \
    timeout 40 "$here/hello-vte" > "$here/run.out" 2> "$here/run.err"
echo "exit $?" > "$here/run.exit"
kill $mutter_pid 2>/dev/null; wait $mutter_pid 2>/dev/null
INSIDE
here=$here stage=$stage gtkb=$gtkb XDG_RUNTIME_DIR=/run/user/1000 dbus-run-session -- sh inside.sh 2>&1 | grep -v 'dbus-daemon\|fusermount\|connection to the bus'
echo "=== hello-vte $(cat run.exit)"; cat run.out; echo "=== stderr"; cat run.err
