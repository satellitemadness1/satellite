#!/bin/sh
# vendor/gtk/hello/build.sh -- link the experiment against the STATIC GTK.
#
#     sh build.sh          link with -static: one file, no dynamic sections
#     STATIC=0 sh build.sh link dynamically against the same .a files
#
# WHY THE LINK LINE IS BUILT BY HAND, and not from pkg-config.
#
# GTK's gtk4.pc -- and the gtk4-uninstalled.pc meson writes beside a build -- describe
# libgtk_dep, which is the SHARED libgtk-4.so (gtk-4.16.7/meson.build:883). Checked here
# on 2026-09-19, in a build configured --default-library=static:
#
#     $ pkg-config --libs gtk4
#     -L.../build-static/gtk -lgtk-4 ...
#
# That is the shared object, in a static build, with no warning. A binary linked from
# that line passes every casual check and proves the opposite of what this experiment
# is for. The static library is `libgtk_static` (gtk/meson.build:1116) and it is never
# installed and never named in a .pc: on disk it is build-static/gtk/libgtk.a.
#
# So: CFLAGS from pkg-config, which are right, and the libraries gathered as FILES.
# This is also the shape satl itself needs -- satl is built by make, not meson
# (make_support/048-link.mk), so a make rule will have to name these .a files the same
# way. The experiment rehearses the real link rather than a meson-only convenience.
#
# --start-group, because static archives are order-sensitive and this graph has cycles
# (harfbuzz <-> freetype, glib <-> gio); a group lets the linker re-scan until it settles.
set -eu

here=$(cd "$(dirname "$0")" && pwd)
BUILD="$here/../build-static"
CC=${CC:-/usr/bin/gcc}
STATIC=${STATIC:-1}

[ -d "$BUILD" ] || { echo "hello: no $BUILD -- configure and build GTK first" >&2; exit 1; }
[ -f "$BUILD/gtk/libgtk.a" ] || { echo "hello: $BUILD/gtk/libgtk.a is missing -- ninja has not finished" >&2; exit 1; }

CFLAGS=$(PKG_CONFIG_PATH="$BUILD/meson-uninstalled" pkg-config --cflags gtk4)

# Every archive the build produced. libgtk.a goes FIRST so its undefined symbols drive
# the rest; the group sorts out the remainder. Sorted for a reproducible command line.
# NOT EVERY .a IN THE TREE IS A LIBRARY, AND THE TEST IS THE NAME, NOT THE DIRECTORY.
# cairo keeps three things under util/ that must never be linked into a program:
#   libmalloc-stats.a  DEFINES malloc/realloc/free ->
#       multiple definition of `realloc'; .../malloc-stats.c:216: first defined here
#   libcairo-trace.a, libcairo-fdr.a  LD_PRELOAD interposers that redefine cairo_*
# and pixman keeps demos/libdemo.a. But cairo ALSO keeps two REAL libraries under the
# same util/ -- libcairo-gobject.a and libcairo-script-interpreter.a -- and GSK needs
# both (gsk/gskrendernodeparser.c wants cairo_gobject_antialias_get_type and
# cairo_script_interpreter_destroy). Excluding util/ wholesale, which is the obvious
# thing to do after the malloc-stats collision, breaks the link a second way.
# libintl.a is proxy-libintl, a STUB gettext that meson falls back to when it finds no
# system libintl. glibc has gettext built in, so on this platform it is not merely
# redundant, it collides:
#     multiple definition of `_nl_msg_cat_cntr'; .../proxy-libintl/libintl.a
# against glibc's own libc.a(loadmsgcat.o). Dropping it loses nothing -- the real
# gettext symbols resolve straight out of libc.
excluded='libmalloc-stats.a libcairo-trace.a libcairo-fdr.a libdemo.a libintl.a'
archives=""
for a in $(find "$BUILD" -name '*.a' ! -name 'libgtk.a' | sort); do
    case " $excluded " in
        *" $(basename "$a") "*) continue ;;
    esac
    archives="$archives $a"
done

# What is NOT in the group, and why. -lm, -lpthread and -ldl are glibc's own. -ldl in
# particular is the honest part: libepoxy calls dlopen() to find libGL/libEGL at run
# time BY DESIGN, so even a fully static binary keeps a dynamic loader path for the
# graphics driver. That is not a defect we can link away -- the GL driver belongs to
# the machine's graphics card, not to satellite.
# -lwayland-client IS SHARED ON PURPOSE AND IS WHY STATIC=1 CANNOT ALSO HAVE GL.
# Built into the binary it produced a second copy of the protocol library, and the GPU
# driver dlopens its own; GDK's wl_display then reached wl_list_insert() in a copy that
# had never initialised it. Measured, SIGSEGV, in BOTH link modes -- see fetch-deps.sh.
# A compositor connection is shared with whatever draws on it, so there is exactly one.
syslibs="-lm -lpthread -ldl -lrt -lresolv -lwayland-client -lwayland-egl"

out="$here/hello-static"
[ "$STATIC" = "1" ] && { link_mode="-static"; out="$here/hello-static"; } || { link_mode=""; out="$here/hello-dynamic"; }

echo "hello: compiling and linking ${link_mode:-dynamically}"
# shellcheck disable=SC2086
$CC -O2 -g -o "$out" "$here/hello-static.c" \
    $CFLAGS \
    $link_mode \
    -Wl,--start-group "$BUILD/gtk/libgtk.a" $archives -Wl,--end-group \
    $syslibs 2>&1 | tail -40

echo
echo "=== $out ==="
ls -lh "$out" | awk '{print "size:      " $5}'
file -b "$out"
echo "--- dynamic dependencies ---"
if ldd "$out" 2>&1 | grep -q "not a dynamic executable"; then
    echo "NONE -- statically linked"
else
    ldd "$out" 2>&1
fi
echo "--- dlopen() call sites left in the binary ---"
strings -a "$out" | grep -E '^lib(GL|EGL|GLX|glapi)\.so' | sort -u || true
