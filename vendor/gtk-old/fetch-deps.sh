#!/bin/sh
# vendor/gtk/fetch-deps.sh -- the libraries GTK is built ON, pinned beside it.
#
#     sh vendor/gtk/fetch-deps.sh          downloads, checks, and writes the wrap files
#     sh vendor/gtk/fetch-deps.sh --clean  removes the tarballs and the generated wraps
#
# WHY THIS EXISTS. GTK ships its own subprojects/*.wrap for the whole stack, and they
# cannot be used as they are, for two reasons found by running them (2026-09-19):
#
#   1. THEY DO NOT AGREE WITH EACH OTHER. pango.wrap has `revision = main` -- a moving
#      branch, not a release -- and main now wants glib >= 2.88, while glib.wrap pins
#      2.76.0. `meson setup --wrap-mode=forcefallback` dies on
#      "Dependency 'glib-2.0' ... found 2.76.0 but need: '>= 2.88'". Not in five years:
#      on the first configure.
#   2. THEY ARE OLD, AND STATIC MAKES THAT PERMANENT. The wraps pin zlib 1.2.11 (2017),
#      libpng 1.6.37 (2019), freetype 2.11.0 (2021). A shared library gets patched under
#      a program by the distribution; a STATIC one never does. Whatever is linked in is
#      what ships, for as long as the binary ships.
#
# SO: every version here is THE ONE THIS MACHINE RUNS, checked with pkg-config and rpm
# on AlmaLinux 10.2, the same rule fetch.sh keeps for GTK itself (4.16.7 = gtk4-4.16.7-4.el10).
# A static satl and the system's satl-term are then the same libraries, and a bug in
# one is a bug in both. The checksums are upstream's own, downloaded beside each tarball.
#
# NOTHING THIS SCRIPT PRODUCES IS IN GIT -- not the tarballs, not the unpacked trees, not
# the generated .wrap files, which land inside the gitignored gtk-*/ tree. This script is,
# so the whole stack is one command away on any machine.
set -eu

# name|version|series|url_dir  -- GNOME projects share a layout; the rest are named in full.
GNOME="glib|2.80.4|2.80
pango|1.54.0|1.54
gdk-pixbuf|2.42.12|2.42
graphene|1.10.6|1.10"

here=$(cd "$(dirname "$0")" && pwd)
cd "$here"
GTK_TREE="$here/gtk-4.16.7"
SUBP="$GTK_TREE/subprojects"
CACHE="$SUBP/packagecache"

if [ "${1:-}" = "--clean" ]; then
    rm -f ./*.tar.xz ./*.sha256sum
    for n in glib pango gdk-pixbuf graphene xkbcommon; do rm -f "$SUBP/$n.wrap"; done
    rm -rf "$SUBP/glib" "$here/build-pkgconfig"
    echo "vendor/gtk: removed the dependency tarballs and their generated wraps"
    exit 0
fi

[ -d "$SUBP" ] || { echo "vendor/gtk: run fetch.sh first -- no $SUBP" >&2; exit 1; }
mkdir -p "$CACHE"

fetch_gnome() {
    name=$1 version=$2 series=$3
    tar="$name-$version.tar.xz"
    sum="$name-$version.sha256sum"
    base="https://download.gnome.org/sources/$name/$series"

    [ -f "$tar" ] || { echo "vendor/gtk: downloading $tar" >&2; curl -sS -O --max-time 900 "$base/$tar"; }
    [ -f "$sum" ] || curl -sS -O --max-time 60 "$base/$sum"

    # Upstream's file lists other artefacts beside the tarball; only the tarball is checked.
    # ...to stderr, because this function's STDOUT is the hash the caller captures.
    grep "$tar\$" "$sum" | sha256sum -c - >&2
    hash=$(grep "$tar\$" "$sum" | cut -d' ' -f1)

    cp -f "$tar" "$CACHE/$tar"
    echo "$hash"
}

# A cloned subproject DIRECTORY beats a .wrap file in meson, so GTK's git clones of the
# versions we are replacing have to go, or the wrap is silently ignored.
for d in pango gdk-pixbuf graphene; do
    [ -d "$SUBP/$d" ] && { rm -rf "$SUBP/$d"; echo "vendor/gtk: removed GTK's clone of $d"; }
done

# ---- glib ----------------------------------------------------------------
# GLIB IS UNPACKED AS A DIRECTORY, NOT LEFT AS A WRAP, and the directory has to be
# named exactly "glib". GTK's own subprojects/ has eight [wrap-redirect] files that
# point INTO its subprojects -- libffi.wrap, zlib.wrap and gvdb.wrap all say
# "filename = glib/subprojects/<name>.wrap". Meson resolves a redirect while it is
# reading subprojects/*.wrap, BEFORE it downloads or extracts anything, so the file
# has to be on disk already or setup dies with
#     ERROR: wrap-redirect filename 'glib/subprojects/gvdb.wrap' does not exist
# GTK's [wrap-git] made that directory by cloning; we make it by unpacking. (cairo and
# fontconfig are still GTK's own clones, so their four redirects already resolve.)
h=$(fetch_gnome glib 2.80.4 2.80)
if [ ! -d "$SUBP/glib" ]; then
    echo "vendor/gtk: unpacking glib-2.80.4 as subprojects/glib" >&2
    tar xf "glib-2.80.4.tar.xz" -C "$SUBP"
    mv "$SUBP/glib-2.80.4" "$SUBP/glib"
fi
# The three redirect targets glib must carry, checked rather than hoped for.
for r in gvdb libffi zlib; do
    [ -f "$SUBP/glib/subprojects/$r.wrap" ] || {
        echo "vendor/gtk: glib-2.80.4 has no subprojects/$r.wrap -- GTK's redirect will fail" >&2
        exit 1
    }
done
# AND A WRAP BESIDE THE DIRECTORY, not instead of it. The directory alone is not
# enough: a wrap's [provide] block is what tells meson which subproject satisfies the
# NAME "glib-2.0", and with GTK's glib.wrap deleted that mapping came from cairo's own
# subprojects/glib.wrap -- which pins glib 2.74.0. Cairo is configured first, so
# glib-2.0 was resolved through cairo before GTK reached meson.build:386, and our real
# glib subproject then died trying to claim a name that was already taken:
#     ERROR: Tried to override dependency 'glib-2.0' which has already been resolved
# No source_url or source_filename here on purpose -- the directory is already on disk
# and meson must not try to fetch over it. Re-making the tree is this script's job, not
# the wrap's, which is why the wrap can be this small.
cat > "$SUBP/glib.wrap" <<EOF
# GENERATED by vendor/gtk/fetch-deps.sh. Names subprojects/glib (unpacked above from
# glib-2.80.4.tar.xz) as the provider of glib-2.0 and friends, ahead of cairo's
# subprojects/glib.wrap, which pins 2.74.0.
[wrap-file]
directory = glib

[provide]
dependency_names = gthread-2.0, gobject-2.0, gmodule-no-export-2.0, gmodule-export-2.0, gmodule-2.0, glib-2.0, gio-2.0, gio-windows-2.0, gio-unix-2.0
program_names = glib-genmarshal, glib-mkenums, glib-compile-schemas, glib-compile-resources, gio-querymodules, gdbus-codegen
EOF

# ---- pango ---------------------------------------------------------------
h=$(fetch_gnome pango 1.54.0 1.54)
cat > "$SUBP/pango.wrap" <<EOF
# GENERATED by vendor/gtk/fetch-deps.sh. Replaces GTK's [wrap-git] pango at revision =
# main, a MOVING BRANCH that now requires glib >= 2.88, with the release this machine
# runs, pango-1.54.0-3.el10.
[wrap-file]
directory = pango-1.54.0
source_url = https://download.gnome.org/sources/pango/1.54/pango-1.54.0.tar.xz
source_filename = pango-1.54.0.tar.xz
source_hash = $h

[provide]
pango = libpango_dep
pangoft2 = libpangoft2_dep
pangoxft = libpangoxft_dep
pangowin32 = libpangowin32_dep
pangocairo = libpangocairo_dep
EOF

# ---- gdk-pixbuf ----------------------------------------------------------
h=$(fetch_gnome gdk-pixbuf 2.42.12 2.42)
cat > "$SUBP/gdk-pixbuf.wrap" <<EOF
# GENERATED by vendor/gtk/fetch-deps.sh. gdk-pixbuf-2.42.12-2.el10 is what this machine
# runs. Its loaders are normally dlopen()ed modules; a static satl needs them BUILT IN
# (-Dgdk-pixbuf:builtin_loaders=...), which is the whole reason this one is pinned here.
[wrap-file]
directory = gdk-pixbuf-2.42.12
source_url = https://download.gnome.org/sources/gdk-pixbuf/2.42/gdk-pixbuf-2.42.12.tar.xz
source_filename = gdk-pixbuf-2.42.12.tar.xz
source_hash = $h

[provide]
# VERBATIM FROM GTK'S OWN gdk-pixbuf.wrap, program_names included -- the build runs
# gdk-pixbuf-query-loaders and gdk-pixbuf-csource, and a wrap that names no programs
# sends meson looking for them on PATH.
dependency_names = gdk-pixbuf-2.0
program_names = gdk-pixbuf-query-loaders, gdk-pixbuf-pixdata, gdk-pixbuf-csource, gdk-pixbuf-thumbnailer
EOF

# ---- graphene ------------------------------------------------------------
h=$(fetch_gnome graphene 1.10.6 1.10)
cat > "$SUBP/graphene.wrap" <<EOF
# GENERATED by vendor/gtk/fetch-deps.sh. graphene-1.10.6-9.el10.
[wrap-file]
directory = graphene-1.10.6
source_url = https://download.gnome.org/sources/graphene/1.10/graphene-1.10.6.tar.xz
source_filename = graphene-1.10.6.tar.xz
source_hash = $h

[provide]
# VERBATIM FROM GTK'S OWN graphene.wrap. Hand-writing this block is how the system copy
# got linked instead: GTK asks for graphene-gobject-1.0 (meson.build), and a [provide]
# naming only graphene-1.0 does not match, so meson quietly resolved it from /usr/lib64
# with no warning at any stage. Checked against the block inside gtk-4.16.7.tar.xz.
# BOTH NAMES MAPPED TO THE VARIABLE, not just listed. GTK's own wrap lists them under
# dependency_names, which only tells meson WHICH subproject to enter; the subproject is
# then expected to override each name itself, and graphene 1.10.6 overrides neither --
# it only declares graphene_dep (src/meson.build:128). Listing the names alone gives
#     WARNING: Subproject 'graphene' did not override 'graphene-gobject-1.0' ... found: NO
# The name = variable form points both at that declaration explicitly.
graphene-1.0 = graphene_dep
graphene-gobject-1.0 = graphene_dep
EOF

# PROXY-LIBINTL IS REMOVED, NOT CONFIGURED AWAY. glib's own comment (meson.build:2222)
# describes the intended order:
#     # First check in libc, fallback to libintl, and as last chance build
#     # proxy-libintl subproject.
#     libintl = dependency('intl', required: false)
# --wrap-mode=forcefallback skips the first two and goes straight to "last chance".
# proxy-libintl exists for platforms whose libc has no gettext -- Windows, older macOS.
# glibc has it, so on this machine the fallback is simply wrong, and it collides:
#     multiple definition of `_nl_msg_cat_cntr'
#         libc.a(loadmsgcat.o)  vs  proxy-libintl/libintl.a
# Dropping libintl.a from the link instead does NOT work: proxy-libintl renames the
# whole API through its header, so glib is compiled against g_libintl_dgettext and
# friends and the link then fails on those. The fix has to be at configure time, and
# with no wrap to find, dependency('intl') falls through to glibc as glib intended.
# -Dglib:nls=disabled does NOT do this -- glib only consults `nls` when looking for
# xgettext to build .mo files (meson.build:2579), never to decide whether to link intl.
for w in "$SUBP/proxy-libintl.wrap" "$SUBP/glib/subprojects/proxy-libintl.wrap"; do
    [ -f "$w" ] && { rm -f "$w"; echo "vendor/gtk: removed $(basename "$(dirname "$w")")/proxy-libintl.wrap" >&2; }
done
rm -rf "$SUBP/proxy-libintl"

# WAYLAND-CLIENT MUST NOT BE STATIC, AND THIS IS THE SHARPEST LESSON IN THE FOLDER.
# Built as a subproject it links into the binary, and then there are TWO COPIES of
# libwayland-client in one process: ours, and the one the GPU driver dlopens for itself.
# GDK makes its wl_display with ours and hands the pointer to EGL, which passes it to
# its copy, whose internal lists were never initialised for that object. Measured:
#
#     #0  wl_list_insert ()          from /opt/amdgpu/lib64/libwayland-client.so.0
#     #1  wl_proxy_create_wrapper () from /opt/amdgpu/lib64/libwayland-client.so.0
#     #2  dri2_initialize_wayland () from /opt/amdgpu/lib64/libEGL_mesa.so.0
#     #5  gdk_display_init_egl       at gdk/gdkdisplay.c:1836
#
# SIGSEGV, in both the -static and the dynamic build -- so this is not the static-glibc
# hazard, it is duplicate-library state. A connection to a compositor is shared with
# whatever draws on it, so exactly one copy of the protocol library can exist.
#
# Removing the wrap makes meson use the system libwayland-client.so.0 (1.24.0 here).
# That costs nothing real: a machine with no libwayland-client has no Wayland compositor
# and therefore cannot show a window at all, so it was never a target.
#
# wayland-protocols STAYS a subproject -- it is XML read at build time, never linked,
# so there is no second copy of anything to collide.
for w in "$SUBP/wayland.wrap"; do
    [ -f "$w" ] && { rm -f "$w"; echo "vendor/gtk: removed wayland.wrap -- the driver and GDK must share one libwayland-client" >&2; }
done
rm -rf "$SUBP/wayland"

# ---- libxkbcommon: the one GTK needs and does not wrap -------------------
# THIS IS THE LAST UNDEFINED SYMBOL BETWEEN US AND A LINKED BINARY. With every other
# dependency resolved, the static link still failed on nothing but xkb_*:
#     undefined reference to `xkb_state_key_get_layout'   gdkseat-wayland.c:1312
#     undefined reference to `xkb_keymap_num_layouts'     gdkseat-wayland.c:1067
# GTK asks for it at gtk-4.16.7/meson.build:422 and ships NO subprojects/xkbcommon.wrap,
# so --wrap-mode=forcefallback cannot help -- meson only falls back where a wrap exists --
# and /usr/lib64 has only libxkbcommon.so. Writing the wrap is the whole fix.
#
# 1.7.0 is what this machine runs (libxkbcommon-1.7.0-4.el10), same rule as the rest.
# meson.build:274 does meson.override_dependency('xkbcommon', ...), so naming the
# dependency is enough -- no variable mapping needed, unlike graphene.
#
# THE CHECKSUM IS WEAKER THAN THE OTHERS AND THAT IS WORTH KNOWING. The GNOME tarballs
# above are checked against upstream's own published .sha256sum. xkbcommon.org publishes
# none (.sha256sum, .sha256 and .sha256sums are all 404), so this hash was taken from
# our own download on 2026-09-19. It pins the file against silent change from here on;
# it does NOT prove the first download was authentic. If that matters, compare it
# against the distribution's source package before shipping anything built with it.
#
# DATA IS A SEPARATE PROBLEM FROM CODE, and this library is the sharpest case of it in
# the whole stack: linking it in does NOT bring xkeyboard-config with it, and GDK's
# gdkkeymap-wayland.c:478-486 null-checks nothing, so a machine without that data gets a
# SEGFAULT INSIDE gtk_init(). See vendor/gtk/hello/hello-static.c, which probes for it
# before gtk_init so the failure is a printed line instead of a silent death.
XKB_VERSION=1.7.0
xkbtar="libxkbcommon-$XKB_VERSION.tar.xz"
XKB_SHA256=65782f0a10a4b455af9c6baab7040e2f537520caa2ec2092805cdfd36863b247
[ -f "$xkbtar" ] || {
    echo "vendor/gtk: downloading $xkbtar" >&2
    curl -sS -O --max-time 900 "https://xkbcommon.org/download/$xkbtar"
}
echo "$XKB_SHA256  $xkbtar" | sha256sum -c - >&2
cp -f "$xkbtar" "$CACHE/$xkbtar"
cat > "$SUBP/xkbcommon.wrap" <<EOF
# GENERATED by vendor/gtk/fetch-deps.sh. GTK ships no wrap for this; see the script.
[wrap-file]
directory = libxkbcommon-$XKB_VERSION
source_url = https://xkbcommon.org/download/$xkbtar
source_filename = $xkbtar
source_hash = $XKB_SHA256

[provide]
dependency_names = xkbcommon
EOF

# ---- libdrm: a header, not a library ---------------------------------------
# GTK's wayland backend requires libdrm on Linux and then immediately throws the
# library away (gtk-4.16.7/meson.build:644-647):
#
#     libdrm_dep = dependency('libdrm', required: os_linux)
#     # We only care about drm_fourcc.h for all the fourccs,
#     # but not about linking to libdrm
#     libdrm_dep = libdrm_dep.partial_dependency(includes: true, compile_args: true)
#
# So it wants ONE HEADER of pixel-format constants and never links a thing -- nothing
# of libdrm reaches the static binary. libdrm has no wrap, and libdrm-devel is not
# installed here, but drm_fourcc.h IS: kernel-headers puts the kernel's own copy at
# /usr/include/drm/drm_fourcc.h, which is where libdrm's copy comes from in the first
# place. GTK includes it as <drm_fourcc.h>, so an include path is the whole requirement.
#
# This .pc therefore has Cflags and NO Libs -- if some future GTK starts actually
# linking libdrm, this file will fail loudly at link time rather than quietly pulling
# a shared object into a binary that is supposed to have none.
PCDIR="$here/build-pkgconfig"
DRM_HEADER=/usr/include/drm/drm_fourcc.h
if [ ! -f "$DRM_HEADER" ]; then
    echo "vendor/gtk: $DRM_HEADER is missing -- install kernel-headers or libdrm-devel" >&2
    exit 1
fi
mkdir -p "$PCDIR"
cat > "$PCDIR/libdrm.pc" <<EOF
# GENERATED by vendor/gtk/fetch-deps.sh. GTK wants drm_fourcc.h and does not link
# libdrm (gtk-4.16.7/meson.build:644-647), so this describes the header only.
# The header is the kernel's, from kernel-headers, which is what libdrm ships a copy of.
Name: libdrm
Description: drm_fourcc.h only -- no library, on purpose
Version: $(rpm -q --qf '%{VERSION}' libdrm 2>/dev/null || echo 2.4.128)
Cflags: -I/usr/include/drm
EOF
echo "vendor/gtk: wrote $PCDIR/libdrm.pc (headers only, no Libs)" >&2

echo "vendor/gtk: glib 2.80.4, pango 1.54.0, gdk-pixbuf 2.42.12, graphene 1.10.6 pinned"
