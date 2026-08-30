#!/bin/sh
# install.sh -- install satellite into $HOME/.satl on Debian and Ubuntu.
#
# Written and tested on Debian 13 (trixie), 2026-08-28, with g++ 14.2, vte
# 0.80.1 and gtk4 4.18.6. It is aimed at the Debian family -- Debian, Ubuntu,
# and the derivatives that report one of them in ID_LIKE -- and it says so and
# carries on if it finds itself somewhere else, for the reason
# satellite_enterprise/install.sh gives: a machine that can build satellite can
# install it, and refusing on the strength of a name in /etc/os-release would be
# a policy dressed up as a check.
#
# THE SIBLING OF satellite_enterprise/install.sh, AND DELIBERATELY NOT A COPY OF
# IT THAT DRIFTED. Every decision that script made about layouts, symlinks,
# ownership, dry runs and never calling sudo is made the same way here, because
# they were argued once and the arguments do not change at a distribution
# boundary. THREE things genuinely differ, and they are the reason this is a
# second script rather than a flag on the first:
#
#     1. WHERE THE BINARIES COME FROM. That installer builds the tree root and
#        takes satl from beside the Makefile. This one builds satellite_debian/,
#        which is an out-of-tree build, and takes them from build/. See
#        install_support/050-building.sh.
#     2. THE PACKAGE NAMES IN EVERY MESSAGE. dnf and CRB there, apt here -- and
#        not as a translation: on this family both static libraries arrive with
#        build-essential, so the advice is shorter as well as different.
#     3. THE ICON CACHE TOOL IS PACKAGED UNDER A DIFFERENT NAME, which is the
#        one that would have been got wrong silently. See 070-desktop.sh.
#
# THE ARTWORK IS SHARED WITH THE ENTERPRISE FOLDER AND IS NOT COPIED HERE.
# 060-install-tree.sh reads ../satellite_enterprise/icons -- the launcher, the
# mime packet and eighteen PNGs. That is a seam and it is worth naming: the
# artwork belongs to the PROGRAM, not to Enterprise Linux, so the folder it
# currently sits in is a fact about which installer was written first and not
# about what the files are. Twenty binary files copied into a second directory
# would be twenty files that can drift, which is the failure this project
# refuses everywhere else; a path across is the smaller wrong. If the artwork
# ever moves to a shared directory of its own, 060-install-tree.sh is the one
# line that changes.
#
# TWO LAYOUTS, AND ONLY THE FIRST IS THE DEFAULT.
#
#     (no flags)  $HOME/.satl. Everything this script writes is under $HOME,
#                 there is no privilege to drop and no root-owned object file
#                 left in a build tree afterwards.
#     --system    /usr/local, in the bin/ and share/ shape an operating system
#                 expects, INSTALLING OVER whatever satl is already there.
#
# THIS SCRIPT NEVER CALLS sudo. --system checks that the prefix is writable and
# stops with the command to run otherwise; it does not escalate on your behalf.
# From the first satellite, which said it best -- "a program that silently
# escalates is a program you cannot audit by reading the command you typed."
#
# WHAT IT INSTALLS. install_support/060-install-tree.sh is the one place the
# tree is DECLARED -- this is a copy of it in prose, and if the two ever
# disagree that file is right:
#
#     $HOME/.satl/satl                        the interpreter
#     $HOME/.satl/satl-cpu-level              the detector, kept so the choice
#                                             below can be checked afterwards
#     $HOME/.satl/satl-term                   the GTK4/VTE window, when the
#                                             libraries to build it were there
#     $HOME/.satl/share/applications/...      the satl-term launcher, with it
#     $HOME/.satl/share/mime/packages/...     the .satl file type
#     $HOME/.satl/share/icons/hicolor/...     the artwork, nine sizes
#
# Under --system the same tree lands in the prefix, with the three programs in
# bin/ and share/ where it already was -- plus share/icons/hicolor/index.theme,
# which is not artwork and is what decides whether any of the artwork is ever
# drawn. 075-system.sh is entirely about that file.
#
# FOUR FILES ARE BUILT AND THREE PROGRAMS ARE INSTALLED, which is not an
# omission. On x86-64 the build produces satl and satl.haswell -- the same
# sources compiled against the x86-64 baseline and against x86-64-v3, the
# instruction set Haswell introduced in 2013 -- plus satl-cpu-level, which asks
# the CPU which of them it can run, plus satl-term. This script runs the
# detector and installs its answer UNDER THE NAME satl. The installed binary
# then says which one it is for the rest of its life: `satl --version` prints
# the flags its objects were compiled with, so the install needs no manifest and
# cannot have one that disagrees with the file it describes.
# See ../make_support/045-microarchitecture.mk and PLAN.md sec 4.2.
#
# satl-term MUST LAND BESIDE satl AND DOES. ../src/programs/terminal.cpp reads
# /proc/self/exe and spawns the `satl` next to itself rather than the one PATH
# finds, and ../src/programs/window_handover.cpp does the mirror image, so the
# two share one directory here by requirement and not by tidiness.
#
# THE WINDOW IS CONDITIONAL AND THE INTERPRETER IS NOT. 047-window.mk asks
# pkg-config for vte-2.91-gtk4; without it the Debian build drops satl-term with
# a note, and this script installs three files less and says so. A machine with
# no desktop libraries gets a correct install, not a failed one.
#
# NOTHING HERE EDITS .profile, .bashrc, .zshrc OR ANY OTHER FILE THE USER OWNS,
# and nothing tells the user to export a variable. A child process cannot change
# its parent's environment, so an installer that "exports PATH" exports it into
# a shell that exits one line later; editing a login file is a permanent change
# made by a program someone ran once, and --uninstall cannot reliably undo it.
# What this script does instead is offer a SYMLINK, which is a file, in a
# directory that is already on PATH by convention, and which --uninstall can
# therefore actually remove.
#
# POSIX sh, not bash. An installer is the one program that has to run before
# anything is installed, so it may not assume a shell that might not be there.
#
# THIS FILE IS AN INDEX. The installer is the nine fragments under
# install_support/, sourced below in the order they are numbered -- the same
# arrangement as the Makefile beside it and as the enterprise installer.
#
# A SOURCED FRAGMENT RUNS AS IT IS READ, unlike an included makefile, so the
# order below is not a convenience -- it is the script. 010 and 020 define; 030
# reads the command line; 040 looks at the machine; 050 builds and chooses;
# 060 installs or removes; 070 does the optional work outside the root; 075
# does the work that only a system install has; 080 reports.
#
# 075 is numbered in fives rather than taking the next ten because it belongs
# BETWEEN those two and the tens were already spent. It reads
# rebuild_data_indexes() out of 070, so it cannot move above it.
#
# WHERE TO LOOK, by what you want to change:
#
#     where it installs to ............. 010-defaults.sh
#     an option, or how one is checked . 030-arguments.sh
#     the Debian family checks ......... 040-machine.sh
#     how the variant is chosen ........ 050-building.sh
#     WHAT gets installed .............. 060-install-tree.sh
#     the PATH link and the icons ...... 070-desktop.sh
#     the --system takeover ............ 075-system.sh
#     what it says afterwards .......... 080-report.sh

set -eu

self=$0

# The tree this script was unpacked with is the one it installs, so everything
# is found relative to THIS FILE and not to the caller's directory: `sh
# /mnt/satellite/satellite_debian/install.sh` run from $HOME must still build
# /mnt/satellite/satellite_debian.
here=$(dirname -- "$self")
here=$(cd -- "$here" && pwd)

# One level up. The sources, pcg and the shared make_support are up there; the
# Makefile that builds them, these fragments and the build output are down here.
# Two variables because a single one would have to be right about both.
repo=$(cd -- "$here/.." && pwd)

# WHERE THE BINARIES ARE, and this is the line that differs most from the
# enterprise installer. That one builds the tree root and finds satl beside the
# Makefile; satellite_debian is an out-of-tree build whose Makefile puts every
# binary under build/, so the four files are here and not up there. Declared
# once, beside the two directories it is derived from, rather than spelled as
# "$here/build" in the four fragments that need it.
build=$here/build

# die() is not defined yet -- it arrives in 020-saying-things.sh -- so this one
# message is spelled out. It is the only duplicated line in the split, and it is
# duplicated so that the failure it reports can be reported at all.
support=$here/install_support
if [ ! -d "$support" ]; then
    printf 'install.sh: %s\n' "install_support/ is not beside $self.
       This script is an index: the installer is nine files in that folder,
       sourced in order. A missing install_support/ means this file was copied
       out on its own. Copy the whole satellite_debian/ directory instead --
       and note that it is a build OF the tree above it and needs that too." >&2
    exit 1
fi

# IN THIS ORDER, and the order is the script. Named one by one rather than
# globbed: a glob sorts asciibetically, which is fine for nine files numbered in
# tens and stops being fine at a hundredth, and it would also source an editor's
# backup copy of a fragment on top of the fragment.
. "$support/010-defaults.sh"
. "$support/020-saying-things.sh"
. "$support/030-arguments.sh"
. "$support/040-machine.sh"
. "$support/050-building.sh"
. "$support/060-install-tree.sh"
. "$support/070-desktop.sh"
. "$support/075-system.sh"
. "$support/080-report.sh"
