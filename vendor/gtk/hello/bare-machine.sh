#!/bin/sh
# vendor/gtk/hello/bare-machine.sh -- run the experiment on a machine that has no GTK.
#
#     sh bare-machine.sh ./hello-static            nothing GTK-ish is visible
#     sh bare-machine.sh --with-xkb ./hello-static the same, but xkeyboard-config IS there
#
# WHY bwrap AND NOT `env -i`. Clearing the environment proves nothing about files: with
# XDG_DATA_DIRS unset, glib still falls back to its compiled-in default of
# /usr/local/share:/usr/share, so the developer machine's schemas, icon themes and MIME
# database are all still THERE and still found. A binary can pass `env -i` and die on a
# real enterprise box. bwrap gives it an actual empty root, so absence is absence.
#
# WHAT IS BOUND IN, AND WHY EACH ONE IS FAIR. The target is a headless-ish enterprise
# Linux box with a graphical session -- not a machine with no hardware:
#   /dev/dri            the GPU. A user's graphics card is theirs, not something we ship.
#   $XDG_RUNTIME_DIR    the wayland socket. Without a compositor there is no window at all.
#   /sys, /proc         mesa reads both to identify the card.
#   /usr/lib64, /lib64  ONLY when the binary is dynamic (see below), plus the GL driver
#                       stack, which is dlopen()ed by design and can never be linked in.
#
# WHAT IS DELIBERATELY ABSENT -- the whole point:
#   /usr/share/glib-2.0/schemas   GSettings. g_settings_new() on a missing schema calls
#                                 g_error(), which is fatal and cannot be caught.
#   /usr/share/icons, /usr/share/themes
#   /usr/lib64/gdk-pixbuf-2.0, /usr/lib64/gio/modules
#   /usr/share/mime               what gio_sniffing=false is supposed to make unnecessary
#   /etc/fonts, /usr/share/fonts  text should degrade to tofu, not crash
#   /usr/share/X11/xkb            UNLESS --with-xkb. This is the one expected to SEGFAULT
#                                 inside gtk_init(), at gdkkeymap-wayland.c:486. Run it
#                                 both ways: the difference is the proof.
set -eu

WITH_XKB=0
WITH_FONTS=0
while : ; do
    case "${1:-}" in
        --with-xkb)   WITH_XKB=1;   shift ;;
        --with-fonts) WITH_FONTS=1; shift ;;
        *) break ;;
    esac
done
[ $# -ge 1 ] || { echo "usage: $0 [--with-xkb] ./hello-static" >&2; exit 2; }

bin=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
[ -x "$bin" ] || { echo "$bin is not executable" >&2; exit 1; }
command -v bwrap >/dev/null || { echo "bwrap is not installed" >&2; exit 1; }

RUNTIME=${XDG_RUNTIME_DIR:-/run/user/$(id -u)}
WL=${WAYLAND_DISPLAY:-wayland-0}
scratch=$(mktemp -d)
trap 'rm -rf "$scratch"' EXIT

set -- bwrap \
    --unshare-user --unshare-pid --unshare-ipc --unshare-uts \
    --die-with-parent \
    --tmpfs / \
    --proc /proc \
    --dev /dev \
    --dev-bind /dev/dri /dev/dri \
    --ro-bind /sys /sys \
    --tmpfs /tmp \
    --bind "$scratch" /scratch \
    --ro-bind "$bin" /hello-static \
    --setenv XDG_RUNTIME_DIR /run/user-scratch \
    --tmpfs /run/user-scratch \
    --ro-bind "$RUNTIME/$WL" "/run/user-scratch/$WL" \
    --setenv WAYLAND_DISPLAY "$WL" \
    --setenv HOME /scratch \
    --setenv TMPDIR /tmp \
    --setenv SATL_HELLO_SECONDS "${SATL_HELLO_SECONDS:-3}"

# A dynamic binary still needs its loader and its libraries; a static one needs neither,
# and binding them anyway would make a static run look better than it is. The GL driver
# is bound in BOTH cases, because epoxy dlopen()s it whatever we link.
# BIND ONLY WHAT THE BINARY ACTUALLY NAMES, resolved with ldd -- not all of /usr/lib64.
# Binding the whole directory would leave libgtk-4.so, libglib-2.0.so and the rest
# sitting there, and "it ran on a machine with no GTK" would be a claim about a machine
# that still had GTK on it. Each library is bound at its own path, one at a time, so the
# list below IS the dependency list and anything missing from it fails loudly.
if file -b "$bin" | grep -q "dynamically linked"; then
    echo "note: $bin is DYNAMIC -- binding ONLY the libraries ldd names, individually"
    for lib in $(ldd "$bin" | awk '/=> \//{print $3} /^\s*\/lib64\/ld-linux/{print $1}'); do
        [ -e "$lib" ] && set -- "$@" --ro-bind "$lib" "$lib"
    done
    # The loader needs its cache to find a SONAME it was not given a path for.
    [ -e /etc/ld.so.cache ] && set -- "$@" --ro-bind /etc/ld.so.cache /etc/ld.so.cache
else
    echo "note: $bin is STATIC -- binding nothing from /usr except the GL driver"
fi

# THE GL DRIVER IS BOUND IN BOTH CASES. libepoxy resolves every GL entry point by
# dlopen()+dlsym() by design, so no link model can absorb it, and a user's graphics
# driver is theirs -- shipping one was never on the table. Its own dependencies come
# with it, which is why the whole driver directory is bound rather than picked apart.
for d in /usr/lib64/dri /opt/amdgpu/lib64 \
         /usr/lib64/libEGL.so.1 /usr/lib64/libEGL_mesa.so.0 /usr/lib64/libGL.so.1 \
         /usr/lib64/libGLdispatch.so.0 /usr/lib64/libGLX.so.0 /usr/lib64/libglapi.so.0 \
         /usr/lib64/libgbm.so.1 /usr/lib64/libdrm.so.2 /usr/lib64/libdrm_amdgpu.so.1 \
         /usr/lib64/libexpat.so.1 /usr/lib64/libzstd.so.1 /usr/lib64/libz.so.1 \
         /usr/lib64/libelf.so.1 /usr/lib64/libstdc++.so.6 /usr/lib64/libgcc_s.so.1 \
         /usr/share/glvnd; do
    [ -e "$d" ] && set -- "$@" --ro-bind "$d" "$d"
done

if [ "$WITH_XKB" = 1 ]; then
    echo "note: xkeyboard-config IS bound in (/usr/share/X11/xkb)"
    set -- "$@" --ro-bind /usr/share/X11/xkb /usr/share/X11/xkb \
                --setenv XKB_CONFIG_ROOT /usr/share/X11/xkb
else
    echo "note: xkeyboard-config is ABSENT -- expect a segfault inside gtk_init()"
fi

# Fonts are their own axis. The claim under test is that a machine with no fonts gets
# tofu rather than a crash -- DEGRADED, not fatal. Run it both ways to find out; do not
# take either answer on trust, because "it still drew something" and "it still ran" are
# different claims and only one of them is about correctness.
if [ "$WITH_FONTS" = 1 ]; then
    echo "note: fonts ARE bound in (/usr/share/fonts, /etc/fonts)"
    for f in /usr/share/fonts /etc/fonts /usr/share/fontconfig /var/cache/fontconfig; do
        [ -e "$f" ] && set -- "$@" --ro-bind "$f" "$f"
    done
else
    echo "note: NO fonts at all -- text should be tofu, and that should not be fatal"
fi

echo "--- running with no GTK installed ---"
set +e
"$@" /hello-static
status=$?
set -e
echo "--- exit status $status ---"
case $status in
  0)   echo "clean" ;;
  139) if grep -q "xkb: NO " /dev/null 2>&1; then :; fi
       echo "SIGSEGV. If the last line before it was the xkb probe, it is"
       echo "gdkkeymap-wayland.c:486. If the window was already presented, it is NOT --"
       echo "read the output above rather than trusting this line." ;;
  *)   echo "see the output above" ;;
esac
exit $status
