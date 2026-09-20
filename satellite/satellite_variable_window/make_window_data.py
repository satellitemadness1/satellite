#!/usr/bin/env python3
# satellite/satellite_variable_window/make_window_data.py -- everything satl must
# carry so that gtk_init() does not SIGSEGV on a machine with no GTK installed.
# SATELLITE_WINDOW.md WIN-1.
#
# CODE LINKS; DATA DOES NOT. That one sentence is the whole milestone. The static
# build carries GTK, glib, cairo, pango, freetype and fontconfig as machine code,
# and every one of them then goes looking on disk for files that are not there:
#
#   xkeyboard-config   gdk/wayland/gdkkeymap-wayland.c calls xkb_keymap_new_from_names()
#                      at seat creation and null-checks nothing -> SIGSEGV inside
#                      gtk_init(), before a window exists and before anything prints.
#   a fontconfig config  measured 2026-09-20: "Cannot load default config file"
#                      then SIGSEGV. Our fontconfig does not read /etc/fonts -- it
#                      reads where OUR build was configured, which exists nowhere else.
#   at least one font  no font is FATAL rather than degraded (measured, n=4).
#   GSettings schemas  g_settings_new() on a missing schema calls g_error(), which
#                      is fatal and cannot be caught. The emoji chooser is in the
#                      default right-click menu of every editable text widget.
#
# THEY GO IN AS A GResource -- one blob in .rodata, compressed -- and are spilled
# to a writable directory before gtk_init runs (window_spill.cpp). GResource and
# not a C array: glib is already linked, the blob is compressed for free, and a
# 2.8 MB byte array is 17 MB of source that clang has to parse every build.
#
# Run from the repo root, or let make run it:
#   python3 satellite/satellite_variable_window/make_window_data.py
import os
import subprocess
import sys
import xml.sax.saxutils as sax

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(ROOT, "build", "generated")
PREFIX = "/org/satellite/window"

# WHAT GOES IN, and where it comes from. Each is (resource path, file on disk).
# A source that is missing is an ERROR and never a silent skip: a resource that
# is quietly short produces exactly the SIGSEGV this file exists to prevent.
def gather():
    items = []

    xkb = os.path.join(ROOT, "vendor", "xkb", "xkb-data")
    if not os.path.isdir(xkb):
        sys.exit("make_window_data.py: no vendor/xkb/xkb-data -- run sh vendor/xkb/fetch.sh")
    for folder, _, names in os.walk(xkb):
        for name in sorted(names):
            full = os.path.join(folder, name)
            items.append(("xkb/" + os.path.relpath(full, xkb), full))

    # THE WHOLE IBM PLEX MONO FAMILY, not only Regular. The author's ceiling for
    # the binary makes 1.9 MB free, and carrying every face is what lets a window
    # use bold or italic at all -- WIN-3's "11px or 12px, never bold or italic"
    # ruling is then a CHOICE rather than a limit of what was shipped.
    fonts = os.path.join(ROOT, "vendor", "fonts", "ibm-plex-mono")
    faces = sorted(n for n in os.listdir(fonts) if n.endswith(".ttf")) if os.path.isdir(fonts) else []
    if not faces:
        sys.exit("make_window_data.py: no .ttf in vendor/fonts/ibm-plex-mono -- run sh vendor/fonts/fetch.sh")
    for name in faces:
        items.append(("fonts/" + name, os.path.join(fonts, name)))
    # THE OFL TRAVELS WITH THE FONT, and that is a licence obligation rather than
    # a courtesy: vendor/fonts/fetch.sh says "whatever satl ships, ships OFL.txt".
    ofl = os.path.join(fonts, "OFL.txt")
    if os.path.isfile(ofl):
        items.append(("fonts/OFL.txt", ofl))

    items.append(("fonts.conf.in", os.path.join(HERE, "fonts.conf")))

    schemas = os.path.join(ROOT, "vendor", "gtk", "build-static", "gtk", "gschemas.compiled")
    if os.path.isfile(schemas):
        items.append(("schemas/gschemas.compiled", schemas))
    else:
        print("make_window_data.py: no gschemas.compiled -- GTK's emoji chooser will abort "
              "(build vendor/gtk first)", file=sys.stderr)
    return items


def main():
    items = gather()
    os.makedirs(OUT, exist_ok=True)
    xml_path = os.path.join(OUT, "window_data.gresource.xml")

    # COMPRESSED, EXCEPT THE FONTS. xkb data is text and compresses to about a
    # fifth; a .ttf is already compressed and zlib makes it bigger while costing
    # a decompress on every spill.
    with open(xml_path, "w", encoding="utf-8") as x:
        x.write('<?xml version="1.0" encoding="UTF-8"?>\n<gresources>\n')
        x.write('  <gresource prefix="%s">\n' % PREFIX)
        for name, full in items:
            squeeze = "false" if name.endswith(".ttf") else "true"
            x.write('    <file alias="%s" compressed="%s">%s</file>\n'
                    % (sax.escape(name), squeeze, sax.escape(full)))
        x.write("  </gresource>\n</gresources>\n")

    # THE VENDORED COMPILER FIRST. A GResource written by the system's glib and
    # read by ours is two versions of one format meeting inside satl.
    vendored = os.path.join(ROOT, "vendor", "gtk", "build-static", "subprojects",
                            "glib", "gio", "glib-compile-resources")
    tool = vendored if os.access(vendored, os.X_OK) else "glib-compile-resources"

    source = os.path.join(OUT, "window_data.c")
    run = [tool, "--target", source, "--generate-source",
           "--c-name", "satl_window_data", "--sourcedir", "/", xml_path]
    made = subprocess.run(run, capture_output=True, text=True)
    if made.returncode != 0:
        sys.exit("make_window_data.py: %s failed:\n%s" % (tool, made.stderr.strip()))

    bytes_in = sum(os.path.getsize(f) for _, f in items)
    print("window_data.c: %d files, %.1f MB in, %.1f MB of generated source (%s)"
          % (len(items), bytes_in / 1048576, os.path.getsize(source) / 1048576,
             "vendored glib" if tool == vendored else "system glib"))


if __name__ == "__main__":
    main()
