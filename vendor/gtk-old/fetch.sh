#!/bin/sh
# vendor/gtk/fetch.sh -- put GTK's own source beside satellite, so satl can be built
# with GTK INSIDE IT: one executable, on a machine that has no GTK installed.
#
#     sh vendor/gtk/fetch.sh            downloads, checks and unpacks GTK
#     sh vendor/gtk/fetch.sh --clean    removes the unpacked tree, keeps the tarball
#
# THE VERSION IS THE ONE THIS MACHINE RUNS (gtk4-4.16.7-4.el10), so a static satl and
# the system's own satl-term are the same GTK, and a bug in one is a bug in both.
# Change both lines together; the checksum is upstream's own, from
# https://download.gnome.org/sources/gtk/4.16/gtk-4.16.7.sha256sum.
#
# NOTHING HERE IS IN GIT (see .gitignore): 201 MB unpacked, with its own history
# upstream -- the same rule the repository already keeps for old_versions/first_satellite.
# This script and the checksum beside it ARE in git, so the tree can be made again
# exactly, on any machine, from two lines.
set -eu

VERSION=4.16.7
SERIES=4.16

here=$(cd "$(dirname "$0")" && pwd)
cd "$here"

if [ "${1:-}" = "--clean" ]; then
    rm -rf "gtk-$VERSION"
    echo "vendor/gtk: removed gtk-$VERSION (the tarball is kept)"
    exit 0
fi

if [ ! -f "gtk-$VERSION.tar.xz" ]; then
    echo "vendor/gtk: downloading gtk-$VERSION.tar.xz from download.gnome.org"
    curl -sS -O --max-time 600 "https://download.gnome.org/sources/gtk/$SERIES/gtk-$VERSION.tar.xz"
fi
if [ ! -f "gtk-$VERSION.sha256sum" ]; then
    curl -sS -O --max-time 60 "https://download.gnome.org/sources/gtk/$SERIES/gtk-$VERSION.sha256sum"
fi

# Upstream's file lists the .news beside the tarball; only the tarball is checked here.
grep "gtk-$VERSION.tar.xz\$" "gtk-$VERSION.sha256sum" | sha256sum -c -

if [ ! -d "gtk-$VERSION" ]; then
    echo "vendor/gtk: unpacking"
    tar xf "gtk-$VERSION.tar.xz"
fi
echo "vendor/gtk: gtk-$VERSION is ready ($(du -sh "gtk-$VERSION" | cut -f1))"
