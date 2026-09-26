#!/bin/sh
# install.sh — install satellite from a source tarball or from the prebuilt
# download folder.
#
# For people who have a tarball rather than a .deb. It is deliberately thin: the
# install tree is declared exactly once, in the Makefile's `install` target, and
# this script only decides WHERE that tree goes and then calls it. Two lists of
# files to install is how an install tree rots — the day someone adds a data
# file to one of them, the other is silently wrong, and the failure surfaces as
# a missing file on a user's machine rather than as a build error here.
#
# The prebuilt download folder has no Makefile to call, and that used to be the
# end of the story: the script exited with "no Makefile beside ./install.sh"
# and installed nothing. It carries install_tree/ instead — the output of that
# same one declaration, staged prefix-relative by `make bundle` — and this
# script copies it. Still one list, still in the Makefile; the bundle ships
# what running it produced rather than a second copy of it. See the mode
# detection further down.
#
# POSIX sh, not bash. On a minimal Ubuntu image /bin/sh is dash, and an
# installer is the one program that has to run before anything is installed.
#
# Nothing here writes to .profile, .bashrc, .zshrc or any other file the user
# owns, and nothing tells the user to set an environment variable. Three
# reasons, of which the third is the one that matters:
#
#   - A child process cannot change its parent's environment. An installer that
#     "exports PATH" exports it into a shell that exits one line later, so the
#     gesture is theatre even when the user wanted it.
#   - Editing a login file is a permanent change to something the user owns,
#     made by a program they ran once. `--uninstall` cannot reliably undo it,
#     which is how a stale PATH entry outlives three uninstalls.
#   - An install that only works once SATELLITE_PATH is set is not an install.
#     The interpreter resolves its own library directory in three tiers —
#     $SATELLITE_PATH, then ../share/satellite/lib relative to /proc/self/exe,
#     then the compiled-in SATELLITE_LIB_DIR baked from `prefix` — and that
#     ordering exists precisely so tiers 2 and 3 cover every installed case.
#     If satellite needs SATELLITE_PATH to find its library, tier 2 or tier 3 is
#     broken, and exporting the variable hides the bug instead of fixing it.
#     See DESIGN.md §9 and satl(1) under ENVIRONMENT.
#
# So the most this script does about PATH is SAY that $prefix/bin is not on it,
# and print the exact line the user may add themselves if they want it.

set -eu

# THIS FILE IS AN INDEX. The installer itself is nine fragments under
# install_support/, sourced below in the order they are numbered, and that
# order is the order they appeared in when this was one 634-line file. Nothing
# was rewritten to get here: every line of the old file is in one of the
# fragments, in its original order, and every transcript this script prints is
# byte for byte the one it printed before -- verified across both modes,
# --help, --uninstall, --destdir, a bad option and an unwritable prefix.
#
# A SOURCED FRAGMENT RUNS AS IT IS READ, unlike an included makefile, so the
# order below is not a convenience: it is the script. 010 and 020 define; 030
# reads the command line; 040 decides which of the two modes this is; and
# 050 through 090 do the work and report it, in that sequence.
#
# WHY THIS IS SAFE FOR SOMETHING THAT SHIPS. This script has never been a file
# that works on its own: it installs either a source tree, which needs the
# Makefile beside it, or the download folder, which needs install_tree/ beside
# it, and 040-mode.sh dies with those words when neither is there. Both of the
# ways anybody actually receives it -- a clone or source tarball, and the
# download folder that `make bundle` writes -- are directories, and
# install_support/ travels inside them the same way install_tree/ already does.
# The check below is what turns "somebody copied one file out" into a sentence
# instead of a half-finished install.
#
# WHERE TO LOOK, by what you want to change:
#
#     the default prefix ............... 010-defaults.sh
#     an option, or how one is checked . 030-arguments.sh
#     source tree vs download folder ... 040-mode.sh
#     what a source install builds ..... 050-building.sh
#     what a bundle install copies ..... 060-bundle-tree.sh
#     what the script says afterwards .. 080-report.sh
#
# Split out of the single file on 2026-08-24. See plans/restructure.txt.

self=$0

# The tarball this script was unpacked with is the one it installs, so the
# repository is found relative to the script and not to the caller's directory:
# `sh /mnt/satellite/install.sh` from $HOME must still build /mnt/satellite.
repo=$(dirname -- "$self")
repo=$(cd -- "$repo" && pwd)

# And it is where install_support/ is, for the same reason and by the same
# measurement -- symmetric with SATELLITE_TREE in the Makefile at the top of a
# source tree, which does this once above its own includes and for the same
# cause: a file that is about to read others has to know where it is itself.
#
# die() is not defined yet -- it arrives in 020-saying-things.sh -- so this one
# message is spelled out. It is the only duplicated line in the split, and it
# is duplicated so that the failure it reports can be reported at all.
support=$repo/install_support
if [ ! -d "$support" ]; then
    printf 'install.sh: %s\n' "install_support/ is not beside $self.
       This script is an index: the installer is nine files in that folder,
       sourced in order. It travels with the source tree and inside the
       download folder, so a missing install_support/ means this file was
       copied out on its own. Copy the whole directory instead." >&2
    exit 1
fi

# IN THIS ORDER, and the order is the script. Named one by one rather than
# globbed: a glob sorts asciibetically, which is fine for nine files numbered
# in tens and stops being fine the moment there is a hundredth, and it would
# also source an editor's backup copy of a fragment on top of the fragment.
. "$support/010-defaults.sh"
. "$support/020-saying-things.sh"
. "$support/030-arguments.sh"
. "$support/040-mode.sh"
. "$support/050-building.sh"
. "$support/060-bundle-tree.sh"
. "$support/070-desktop-indexes.sh"
. "$support/080-report.sh"
. "$support/090-desktop-advice.sh"
