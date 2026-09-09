#!/bin/sh
# install-satellite.sh -- install satellite on AlmaLinux 10 from a built package.
#
# THIS IS THE THIRD INSTALLER IN THIS TREE AND THE FIRST THAT DOES NOT BUILD
# ANYTHING. satellite_enterprise/install.sh and satellite_debian/install.sh both
# compile the sources on the machine they are run on, which is correct for a
# developer and is the wrong shape for the person this one is for: it needs a
# compiler, the pkg-config files for gtk4 and vte, and on Enterprise Linux the
# CRB repository turned on before vte291-gtk4-devel can even be found. That is a
# reasonable thing to ask of someone who is going to change satellite and an
# unreasonable thing to ask of someone who is going to learn to program with it.
#
# So this one installs PROGRAMS THAT ARE ALREADY BUILT, sitting in programs/
# beside this file. Nothing is compiled here, nothing is downloaded, and the
# only tools it needs are the ones an AlmaLinux desktop already has. The package
# is made on a build machine by ../make-package.sh, which is where every
# question about how the binaries were produced is answered.
#
# WHY THAT IS SAFE HERE AND WOULD NOT BE IN GENERAL. Shipping a binary is a
# promise that it runs, and the two facts that make the promise keepable are
# written into the package by make-package.sh and checked again by
# 040-machine.sh on the way in:
#
#     1. satl IS FULLY STATIC. `ldd` answers "not a dynamic executable"; it maps
#        no shared object at all, so there is no libstdc++, no libc and no
#        version of either that the target has to match. See
#        ../../make_support/048-static.mk, and note the RPATH trap it solves:
#        this build environment exports LD_RUN_PATH, and a dynamically linked
#        satl comes out pointing at a hand-built GCC in one particular user's
#        home directory. A static satl cannot.
#     2. satl-term CANNOT BE STATIC and does not try to be. GTK4 and VTE ship no
#        static libraries and declare shared_library() in their own build files,
#        so there is nothing to link; the window therefore uses the GTK4 and VTE
#        that the target machine has. That is why this package names one
#        operating system instead of claiming to be portable. AlmaLinux 10.2
#        built it and AlmaLinux 10.2 runs it, with the same vte291-gtk4 0.78
#        that ptyxis -- the default terminal -- already pulls in.
#
# WHAT IT INSTALLS, and install_support/060-install-tree.sh is the one place
# this is DECLARED. The list below is a copy of it in prose, and if the two ever
# disagree that file is right:
#
#     ~/.local/bin/satl                     the interpreter
#     ~/.local/bin/satl-term                the GTK4/VTE window
#     ~/.local/bin/satl-cpu-level           the detector, kept so the choice
#                                           this script made can be checked
#     ~/.local/share/applications/...       the launcher -- the apps-grid icon
#     ~/.local/share/icons/hicolor/...      the artwork, nine sizes, two icons
#     ~/.local/share/mime/packages/...      the .satl file type
#     ~/.local/share/satellite/example/     programs to read and run
#
# NOTHING IT INSTALLS NEEDS ROOT. Every one of those paths is under $HOME, a
# desktop reads all of them, and the install itself can be undone by the person
# who ran it. --prefix and its shorthand --system move the same tree somewhere
# else for a machine-wide install; that one needs a writable prefix, and this
# script CHECKS and prints the command rather than escalating into it.
#
# ONE STEP DOES ASK FOR A PASSWORD, AND IT IS NOT THE INSTALL. satl-term links
# against the machine's GTK4 and VTE, and on a machine that does not have them
# there is nothing this script can copy that would help. So it runs `sudo dnf
# install` for those packages -- 035-dependencies.sh -- and --no-deps declines.
#
# THAT REVERSES A RULE THE OTHER TWO INSTALLERS IN THIS TREE HOLD, at the
# author's instruction, and the rule it is replaced by is the half that was
# load-bearing: NOTHING IS ESCALATED SILENTLY. The package list is printed, the
# exact command is printed, sudo asks for the password itself so the prompt is
# visibly the system's, and a failure there does not fail the install -- the
# interpreter needs none of it. From the first satellite, which said the
# original best: "a program that silently escalates is a program you cannot
# audit by reading the command you typed." This one can still be audited by
# reading it, which is what that sentence was protecting.
#
# THE LAUNCHER IS REWRITTEN AS IT IS INSTALLED, and this is the one place this
# installer deliberately departs from the file it ships. share/applications/
# org.satellite.terminal.desktop carries `Exec=satl-term %f`, a bare command
# name, and its own comments explain why: the prefix is not known when that file
# is WRITTEN. It is known here. Measured on AlmaLinux 10.2 on 2026-09-08:
# `systemd-path search-binaries-default` is /usr/local/sbin:/usr/local/bin:
# /usr/sbin:/usr/bin, and neither /etc/profile nor anything in /etc/profile.d
# adds ~/.local/bin -- the stock ~/.bashrc does, at lines 9-10, and a desktop
# shell launching a .desktop entry does not read .bashrc. So a bare command name
# in the launcher is a bet on how the session happened to be started, and losing
# the bet means TryExec hides the entry and the apps grid shows nothing at all.
# 070-desktop.sh writes the absolute path instead. Nothing else in that file
# changes, and the copy under the prefix is the only copy that is edited.
#
# IT EDITS ~/.bashrc, AND ONLY BETWEEN TWO MARKERS. 075-shell-path.sh appends a
# block that puts the install directory on PATH, so that `satl` and `satl-term`
# are words that work in any terminal window. --no-path declines it.
#
# THIS REVERSES THE SECOND RULE the other two installers hold, again at the
# author's instruction, and again the objection is answered rather than dropped.
# Theirs was that editing a login file is a permanent change made by a program
# somebody ran once, which an uninstaller cannot reliably undo. The markers are
# the answer: --uninstall removes exactly the lines between them, a reinstall
# rewrites rather than appends so the file never grows a second copy, and the
# original is kept once at ~/.bashrc.satellite-backup before the first edit.
#
# The block is still needed even though AlmaLinux's stock ~/.bashrc already puts
# ~/.local/bin on PATH, because that is a fact about a file the person owns and
# may have changed, and because --prefix can put the programs somewhere else
# entirely. Writing it makes the guarantee this script's own rather than
# inherited. It is skipped for a prefix that is on the default PATH already,
# like /usr/local, where it would be noise.
#
# POSIX sh, not bash. An installer is the one program that has to run before
# anything is installed, so it may not assume a shell that might not be there.
#
# THIS FILE IS AN INDEX. The installer is the ten fragments under
# install_support/, sourced below in the order they are numbered -- the same
# arrangement as the Makefile in the tree above and as the two installers this
# one is a sibling of.
#
# A SOURCED FRAGMENT RUNS AS IT IS READ, unlike an included makefile, so the
# order below is not a convenience, it is the script. 010 defines where things
# go; 020 defines how things are said; 030 reads the command line; 035 installs
# the system packages; 040 looks at the machine; 050 checks the package and
# picks the variant; 060 installs or removes; 070 does the desktop work; 075
# does the PATH; 080 reports.
#
# TWO OF THOSE ARE NUMBERED IN FIVES because they belong BETWEEN fragments whose
# tens were already spent, and in both cases the position is load-bearing rather
# than tidy:
#
#     035 MUST RUN BEFORE 040, because 040 decides whether satl-term can run by
#         asking ldd whether its libraries resolve. Installed after that check,
#         the libraries would arrive too late to change the answer, and a
#         machine that was one dnf away from a working window would be told it
#         could not have one.
#     075 MUST RUN AFTER 070, because 070 is what computes whether $bindir is
#         already on PATH, and 075 is what does something about it.
#
# WHERE TO LOOK, by what you want to change:
#
#     where it installs to ............. 010-defaults.sh
#     an option, or how one is checked . 030-arguments.sh
#     the packages dnf installs ........ 035-dependencies.sh
#     the AlmaLinux and GTK checks ..... 040-machine.sh
#     how the variant is chosen ........ 050-payload.sh
#     WHAT gets installed .............. 060-install-tree.sh
#     the launcher, icons and indexes .. 070-desktop.sh
#     the ~/.bashrc block .............. 075-shell-path.sh
#     what it says afterwards .......... 080-report.sh

set -eu

self=$0

# The package this script was unpacked with is the one it installs, so
# everything is found relative to THIS FILE and not to the caller's directory:
# `sh /run/media/mom/USB/satellite/install-satellite.sh` run from $HOME must
# still install the programs sitting on the USB stick.
here=$(dirname -- "$self")
here=$(cd -- "$here" && pwd)

# die() is not defined yet -- it arrives in 020-saying-things.sh -- so this one
# message is spelled out. It is the only duplicated line in the split, and it is
# duplicated so that the failure it reports can be reported at all.
support=$here/install_support
if [ ! -d "$support" ]; then
    printf 'install-satellite.sh: %s\n' "install_support/ is not beside $self.
       This script is an index: the installer is ten files in that folder,
       sourced in order, and the programs and artwork it installs are in
       programs/ and share/ beside them. A missing install_support/ means this
       file was copied out on its own. Unpack the whole package directory and
       run this script from inside it." >&2
    exit 1
fi

# IN THIS ORDER, and the order is the script. Named one by one rather than
# globbed: a glob sorts asciibetically, which is fine for eight files numbered
# in tens and stops being fine at a hundredth, and it would also source an
# editor's backup copy of a fragment on top of the fragment.
. "$support/010-defaults.sh"
. "$support/020-saying-things.sh"
. "$support/030-arguments.sh"
. "$support/035-dependencies.sh"
. "$support/040-machine.sh"
. "$support/050-payload.sh"
. "$support/060-install-tree.sh"
. "$support/070-desktop.sh"
. "$support/075-shell-path.sh"
. "$support/080-report.sh"
