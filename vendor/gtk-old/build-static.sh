#!/bin/sh
# vendor/gtk/build-static.sh -- build the configured static GTK.
#
#     sh vendor/gtk/configure-static.sh && sh vendor/gtk/build-static.sh
#
# THE PATH IS THE WHOLE POINT OF THIS SCRIPT. glib's gdbus-codegen -- which GTK runs to
# generate its ATSPI accessibility sources -- starts with `#!/usr/bin/env python3`, and
# on this machine `python3` is PyPy 3.11 at ~/opt/pypy3, which has no `packaging` module:
#
#     File ".../gdbus-2.0/codegen/utils.py", line 22, in <module>
#         import packaging.version
#     ModuleNotFoundError: No module named 'packaging'
#
# meson found the right interpreter at configure time and said so, but a #!/usr/bin/env
# shebang ignores that and re-resolves through PATH when ninja runs it. Configuring in
# one shell and building in another is therefore enough to break the build -- which is
# exactly how it broke on 2026-09-19, 2235 targets in.
#
# So the venv goes first on PATH here, not only in configure-static.sh.
#
# niced, because an un-niced parallel build of ~3200 targets takes every core and the
# desktop stops repainting -- the same argument satl-term/child.cpp:36 makes for the
# interpreter it spawns.
set -eu

here=$(cd "$(dirname "$0")" && pwd)
BUILD="$here/build-static"
JOBS=${JOBS:-16}

[ -d "$BUILD" ] || { echo "vendor/gtk: no $BUILD -- run configure-static.sh first" >&2; exit 1; }
[ -x "$here/.mesonvenv/bin/python3" ] || { echo "vendor/gtk: no venv -- see configure-static.sh" >&2; exit 1; }

PATH="$here/.mesonvenv/bin:$PATH"
export PATH

echo "vendor/gtk: building with python3 = $(command -v python3)"
nice -n 19 ninja -C "$BUILD" -j "$JOBS"

echo
if [ -f "$BUILD/gtk/libgtk.a" ]; then
    echo "vendor/gtk: libgtk.a  $(ls -lh "$BUILD/gtk/libgtk.a" | awk '{print $5}')"
    echo "vendor/gtk: $(find "$BUILD" -name '*.a' | wc -l) static archives built"
else
    echo "vendor/gtk: libgtk.a was NOT produced" >&2
    exit 1
fi
