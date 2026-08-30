# satellite -- where it goes, and what a run can be told.
#
# Sourced first. Sets the variables every fragment after this one reads; runs
# nothing.
#
# THE DEFAULTS ARE THE ENTERPRISE INSTALLER'S DEFAULTS, deliberately and to the
# letter. Two installers for one language that disagree about where it lands,
# or about whether `satl` ends up on PATH, would make the answer to "where is
# satellite installed" depend on which folder somebody happened to run. The
# arguments behind each of these were made once, in
# ../satellite_enterprise/install_support/010-defaults.sh, and several were paid
# for with somebody's evening; they are summarised here and not re-litigated.

# THE ROOT IS FIXED AND IT IS NOT A PREFIX. Everything satellite owns lives
# under one directory named after the language, which is why the binary is
# $HOME/.satl/satl rather than $HOME/.satl/bin/satl: a prefix layout exists so
# that many packages can share bin/, lib/ and share/, and nothing shares this
# directory. One name to remember, one directory to delete.
#
# THE SAME $HOME/.satl AS THE ENTERPRISE INSTALLER, and that is on purpose
# rather than by inheritance. A machine could have both folders in one checkout
# -- this one does -- and if the two installed to different roots, running the
# other one would leave two satellites in the home directory and neither
# --uninstall would find the other's. One root means the second install
# REPLACES the first, which is what a person who ran it meant.
#
# share/ underneath it is not a contradiction -- the icons and the mime packet
# keep their XDG-relative paths so that publishing them to the desktop in
# 070-desktop.sh is a link with the same relative path on both sides, rather
# than a translation table between two layouts.
#
# --root is offered because a fixed destination is untestable otherwise: every
# rehearsal of this script would have to write into the home directory it is
# rehearsing for. It is not advertised as a way to install somewhere else.
root=${HOME:?HOME is not set, so there is no home directory to install into}/.satl

action=install
dry_run=no

# --link IS ON. An install that leaves `satl` unfindable by name has copied
# files rather than installed a language, and the alternative -- telling the
# user to edit a login file -- is the exact thing install.sh's header refuses to
# ship.
#
# ON-BY-DEFAULT CANNOT OVERWRITE ANYTHING, which is what makes it safe rather
# than merely convenient. ours() and the occupied-refusal in 070-desktop.sh both
# run UNCONDITIONALLY: a path holding anything this script did not create is
# refused, collected, and printed by 080-report.sh, whether linking was asked
# for or assumed. Off-by-default was never the guard; it was a quiet way to not
# find out.
link_bin=yes

# Whether the command line said so, as opposed to this file. 030-arguments.sh
# withdraws the default above for a --root rehearsal and must be able to tell
# "the user asked for links" from "nobody mentioned links".
link_explicit=no
root_given=no

# --desktop STAYS OFF, and it is not an oversight that these two differ. --link
# creates two symlinks in a bin directory. --desktop writes into
# ~/.local/share AND rebuilds three indexes covering EVERY application on the
# machine. That is a reasonable thing to ask for and an unreasonable thing to
# assume from `./install.sh` with no arguments, and it is not needed for the
# word `satl` to work. The launcher does not depend on it either --
# 060-install-tree.sh writes an absolute Exec -- so --desktop and --link are
# independent.
#
# --system is unaffected by both: a prefix install writes bin/ and share/ inside
# the prefix, which is already where PATH and the desktop look.
desktop=no

# WHERE THIS INSTALL LIVES.
#
#   layout=root    $HOME/.satl, everything at the top of one directory named
#                  after the language. The default.
#   layout=prefix  a normal Unix prefix -- bin/ and share/ under $root -- for
#                  installing INTO AN OPERATING SYSTEM rather than beside it.
#
# System mode requires that the prefix be writable by whoever ran it and says so
# otherwise -- 040-machine.sh -- so the escalation is visible in the command the
# user typed, which is the only place it can be audited. THIS SCRIPT NEVER CALLS
# sudo.
layout=root

# Prepended to the three program names in 060-install-tree.sh, and empty in the
# default layout. That is the WHOLE difference between the two trees: share/
# already has its XDG-relative shape under $root, deliberately, so publishing it
# to a prefix is the same relative path on both sides and needs no translation
# table.
bin_rel=

# --system with no --prefix. /usr/local and not /usr: /usr belongs to the
# package manager, and a file this script writes there is a file dpkg does not
# know about sitting where dpkg believes it is authoritative.
system_prefix=/usr/local

# Set by 075-system.sh, read by 080-report.sh, and empty in the home layout --
# declared here because `set -u` is on and 080 reports unconditionally, so a
# variable that only one layout assigns is an unbound-variable exit in the
# other. Same reason have_term starts as `unknown` below rather than unset.
#
#   superseded         files removed because this install replaces the program
#                      they described, one path per line
#   theme_index_state  present | installed | missing -- whether the prefix has
#                      the index.theme without which no icon in it is ever
#                      drawn. 075-system.sh explains why that file decides it.
superseded=
theme_index_state=present

# WHETHER TO LINK THE C++ RUNTIME IN. auto | yes | no, resolved by
# 050-building.sh into a STATIC= for make.
#
#   auto  the best this machine can link, in BOTH layouts, and `no` when it can
#         link neither -- which is not an error, just the build it has always
#         got.
#   yes   STATIC=full, and make stops with a package name if it cannot.
#   no    link as before.
#
# auto REACHES `full` ON A STOCK MACHINE OF THIS FAMILY, which is the one place
# this differs in EFFECT rather than in wording from the enterprise installer.
# There, libstdc++.a and libc.a are two separate packages and one of them lives
# in a repository that is off by default, so `auto` routinely lands on 1 or no.
# Here both arrive with build-essential, so a machine that can compile satellite
# can almost always link it statically too. Measured on Debian 13, 2026-08-28:
# `make -s static-available` answered full with nothing installed beyond
# build-essential. See ../make_support/040-static.mk.
static=auto

: "${MAKE:=make}"

# Which satl was installed. Set by 050-building.sh, read by 080-report.sh, and
# empty on an uninstall, which builds nothing and chooses nothing.
variant=

# WHETHER THE WINDOW WAS BUILT, which is a question about this machine's
# libraries and not about its instruction set. 047-window.mk asks pkg-config for
# vte-2.91-gtk4 and the build drops satl-term when it is missing, with a note
# rather than an error -- so an install on a headless box is a correct install
# of three programs rather than a failure. Set by 050-building.sh; read by
# 060-install-tree.sh, 070-desktop.sh and 080-report.sh.
#
# It starts as `unknown` rather than `no` so that the uninstall can tell the two
# apart: an uninstall builds nothing, so it never learns the answer, and it
# removes satl-term and the launcher by name regardless. Removing a file that
# was never installed costs an rm -f that finds nothing; leaving one behind
# because this run could not prove it was there is how a tree rots.
have_term=unknown
