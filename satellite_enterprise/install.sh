#!/bin/sh
# install.sh -- install satellite into $HOME/.satl on Enterprise Linux.
#
# Written and tested on AlmaLinux 10.2 (Lavender Lion), 2026-08-26. It is aimed
# at the RHEL family -- AlmaLinux, Rocky, CentOS Stream, RHEL, Oracle -- and it
# says so and carries on if it finds itself somewhere else, because a machine
# that can build satellite can install it and refusing on the strength of a name
# in /etc/os-release would be a policy, not a check.
#
# NO ROOT, EVER. Everything this script writes is under $HOME, so there is no
# sudo branch, no privilege to drop, and no root-owned object file left in a
# source tree afterwards. That is a real simplification over the first
# satellite's installer, which had to compile as the human and copy as root; the
# fixed per-user root is what buys it.
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
# FOUR FILES ARE BUILT AND THREE PROGRAMS ARE INSTALLED, which is not an
# omission. On x86-64 the build produces satl and satl.haswell -- the same
# sources compiled against the x86-64 baseline and against x86-64-v3, the
# instruction set Haswell introduced in 2013 -- plus satl-cpu-level, which asks
# the CPU which of them it can run, plus satl-term. This script runs the
# detector and installs its answer UNDER THE NAME satl. The installed binary
# then says which one it is for the rest of its life: `satl --version` prints
# the flags its objects were compiled with, so the install needs no manifest and
# cannot have one that disagrees with the file it describes -- which is exactly
# what a second interpreter sitting beside it would create.
# See make_support/045-microarchitecture.mk and PLAN.md sec 4.2.
#
# satl-term MUST LAND BESIDE satl AND DOES. src/programs/terminal.cpp reads
# /proc/self/exe and spawns the `satl` next to itself rather than the one PATH
# finds, so the two share one directory here by requirement and not by tidiness.
# The fixed root gives that for free.
#
# THE WINDOW IS CONDITIONAL AND THE INTERPRETER IS NOT. 047-window.mk asks
# pkg-config for gtk4 and vte-2.91-gtk4; without them 050-build.mk drops
# satl-term from `all` with a note, and this script installs three files less
# and says so. A machine with no desktop libraries gets a correct install, not
# a failed one.
#
# NOTHING HERE EDITS .profile, .bashrc, .zshrc OR ANY OTHER FILE THE USER OWNS,
# and nothing tells the user to export a variable. Inherited from the first
# satellite's installer, along with the reasons, of which the third is the one
# that matters: a child process cannot change its parent's environment, so an
# installer that "exports PATH" exports it into a shell that exits one line
# later; editing a login file is a permanent change made by a program someone
# ran once, and --uninstall cannot reliably undo it; and an install that only
# works once a variable is set is not an install. What this script does instead
# is offer a SYMLINK, which is a file, in a directory that is already on PATH by
# XDG convention, and which --uninstall can therefore actually remove.
#
# POSIX sh, not bash. An installer is the one program that has to run before
# anything is installed, so it may not assume a shell that might not be there.
#
# THIS FILE IS AN INDEX. The installer is the eight fragments under
# install_support/, sourced below in the order they are numbered. The first
# satellite arrived at this arrangement by splitting a 634-line install.sh after
# the fact, and PLAN_ONE.md sec 6a makes the same argument about C++ files:
# writing to a ceiling changes a file's shape, splitting to one only preserves
# it. So this starts here, the way the Makefile did at M1.
#
# A SOURCED FRAGMENT RUNS AS IT IS READ, unlike an included makefile, so the
# order below is not a convenience -- it is the script. 010 and 020 define; 030
# reads the command line; 040 looks at the machine; 050 builds and chooses;
# 060 installs or removes; 070 does the optional work outside the root; 080
# reports.
#
# WHERE TO LOOK, by what you want to change:
#
#     where it installs to ............. 010-defaults.sh
#     an option, or how one is checked . 030-arguments.sh
#     the Enterprise Linux checks ...... 040-machine.sh
#     how the variant is chosen ........ 050-building.sh
#     WHAT gets installed .............. 060-install-tree.sh
#     the PATH link and the icons ...... 070-desktop.sh
#     what it says afterwards .......... 080-report.sh

set -eu

self=$0

# The tree this script was unpacked with is the one it installs, so everything
# is found relative to THIS FILE and not to the caller's directory: `sh
# /mnt/satellite/satellite_enterprise/install.sh` run from $HOME must still
# build /mnt/satellite.
here=$(dirname -- "$self")
here=$(cd -- "$here" && pwd)

# One level up, because this script lives in satellite_enterprise/ rather than
# at the top of the tree. That is the one structural difference from the first
# satellite's installer, and it is why repo and here are two variables: the
# sources and the Makefile are up there, the icons and these fragments are down
# here, and a single variable would have to be right about both.
repo=$(cd -- "$here/.." && pwd)

# die() is not defined yet -- it arrives in 020-saying-things.sh -- so this one
# message is spelled out. It is the only duplicated line in the split, and it is
# duplicated so that the failure it reports can be reported at all.
support=$here/install_support
if [ ! -d "$support" ]; then
    printf 'install.sh: %s\n' "install_support/ is not beside $self.
       This script is an index: the installer is eight files in that folder,
       sourced in order. A missing install_support/ means this file was copied
       out on its own. Copy the whole satellite_enterprise/ directory instead." >&2
    exit 1
fi

# IN THIS ORDER, and the order is the script. Named one by one rather than
# globbed: a glob sorts asciibetically, which is fine for eight files numbered
# in tens and stops being fine at a hundredth, and it would also source an
# editor's backup copy of a fragment on top of the fragment.
. "$support/010-defaults.sh"
. "$support/020-saying-things.sh"
. "$support/030-arguments.sh"
. "$support/040-machine.sh"
. "$support/050-building.sh"
. "$support/060-install-tree.sh"
. "$support/070-desktop.sh"
. "$support/080-report.sh"
