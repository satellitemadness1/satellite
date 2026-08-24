# satellite -- why `make` at the top of this tree also installs.
#
# Reads prefix and DESTDIR from 060-prefix.mk, and SATELLITE_TREE from the
# Makefile at the top of this tree -- the one line of this policy that cannot
# live in this folder, for the reason given over HERE below.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte, with one
# line changed: HERE.

# `make` at the top of this tree BUILDS and then INSTALLS, and the second half
# of that sentence is unusual enough to earn a paragraph.
#
# A build tree is not an install, and at a shell prompt the two are
# indistinguishable: ./satl and the satl on PATH are different files, and which
# one answers to `satl` is whichever the shell finds first. So a fix compiled
# here was routinely being tried against the copy installed last week, and the
# artwork in dist/icons was routinely not the artwork the desktop was drawing.
# No test catches either: the tests link the objects and never go near either
# binary. Ending the build where the last install ended is what stops the two
# from drifting. The icon walk in `install` below is an unconditional copy, so
# every build overwrites the installed icons with what is in dist/icons right
# now -- which is the point of doing it every time rather than when something
# looks stale.
#
# It fires only when make was invoked with THIS directory as its working
# directory: $(CURDIR) against the directory this file was read from. That is
# necessary and it is NOT sufficient. `make -C <tree>` sets CURDIR to <tree>, so
# it is indistinguishable from `cd <tree> && make` -- same CURDIR, same PWD,
# same MAKELEVEL, measured -- and both of this repository's own callers invoke
# make exactly that way: install.sh builds with `make -C "$repo"` before running
# its own install, and debian/rules arrives through dh_auto_build with
# prefix=/usr and no DESTDIR, where an install would write into the build
# machine's live /usr instead of debian/tmp and slip past dh_missing entirely.
# Neither is guessed at. Both pass SATELLITE_AUTOINSTALL=0 on the make command
# line -- the one level that outranks the assignment below, for the reason
# debian/rules:10-13 already spells out -- and this comment is what the comments
# there point back to.
#
# And it never runs sudo. The prefix is the one chosen above, a directory the
# caller already owns; where it is not writable the install is REFUSED, in
# words, with the command that would have worked. `deps` above refuses to
# escalate for the same reason, one target down. Nothing here can fail the
# build, either: every path through the recipe exits 0, because a tarball built
# by an rpm spec or a Nix derivation reaches this line with an unwritable
# prefix and no way to pass the opt-out, and a printed note is the right
# outcome there.
# A NAMED prefix is the second half of the answer, and it is the half that
# needs no cooperation from anybody. Every caller that must not auto-install --
# debian/rules through dh_auto_build, install.sh's two build steps, and the rpm
# spec or Nix derivation or PKGBUILD that no edit in this tree can reach --
# passes prefix on the make command line, because all of them have somewhere
# specific to put the files. So `$(origin prefix)` answers the question that
# CURDIR cannot: a build that was told where the tree goes is a build whose
# install somebody else is performing. The two callers in this repository pass
# SATELLITE_AUTOINSTALL=0 as well, and the redundancy is deliberate -- it says
# in their own files what they mean, rather than leaving it to be inferred from
# a variable they pass for another reason entirely.
#
# lastword rather than firstword: the MAKEFILES environment variable prepends
# to MAKEFILE_LIST, so firstword can name a file nobody here wrote. realpath
# rather than abspath: CURDIR arrives from getcwd() with symlinks already
# resolved, while a -f path does not, so abspath compares a resolved path
# against an unresolved one and answers no to a tree reached through a symlink.
#
# AND THE MEASUREMENT IS TAKEN IN THE MAKEFILE AT THE TOP OF THIS TREE, not
# here. That is the whole of what the 2026-08-24 split changed in this file.
# MAKEFILE_LIST GROWS WITH EVERY INCLUDE, so read from inside this fragment its
# lastword is this fragment and $(dir ...) is make_support/ -- the comparison
# against CURDIR below would then answer no on every machine, and the
# auto-install would switch itself off in silence, which is the worst way for
# it to go. The root Makefile is the one file whose own position answers the
# question the comparison is asking, so it takes the measurement there and
# hands it here as SATELLITE_TREE. Nothing else about the policy moved.
SATELLITE_AUTOINSTALL ?= 1
HERE := $(SATELLITE_TREE)
AUTOINSTALL := $(strip $(if $(filter-out 0,$(SATELLITE_AUTOINSTALL)),\
                   $(if $(DESTDIR),,\
                     $(if $(filter file undefined,$(origin prefix)),\
                       $(if $(subst $(realpath $(CURDIR)),,$(HERE)),,\
                         $(if $(filter 0,$(MAKELEVEL)),yes))))))

# --silent for that sub-make, but only when this make is not a dry run: `make -n`
# has to SHOW the install it would do, and -s suppresses exactly that printing.
# A recipe line mentioning $(MAKE) is run even under -n -- that is how a dry run
# recurses at all -- so the sub-make is where the question has to be asked. The
# first word of MAKEFLAGS is the bundle of single-letter flags; long options
# arrive later and start with a dash, which is what the filter drops.
MAKE_SHORT_FLAGS := $(filter-out -%,$(firstword $(MAKEFLAGS)))
INSTALL_QUIET    := $(if $(findstring n,$(MAKE_SHORT_FLAGS)),,--silent)

# Three more words to that sub-make, each buying one thing:
#
#   -o satl -o satl-term   `make -B` means rebuild everything, and it reaches
#                          the sub-make through MAKEFLAGS, where `install`
#                          lists both binaries as prerequisites -- so a -B
#                          build would compile the whole tree, install it, and
#                          then compile the whole tree a second time. --old-file
#                          on the two binaries `all` has just finished building
#                          is exactly the statement that they are current, and
#                          it beats -B (measured).
#   GUI_TARGET=            the GUI question was already asked and answered by
#                          the make that got here. Left to ask it again, the
#                          sub-make prints the gui-skipped banner a second time
#                          on every build on a machine without gtk.
#
# And where the advice below names a command for the caller to run as root, it
# names install.sh and not `sudo make`. `make` inside the tree writes .o files,
# relinks satl and rewrites .libdir-stamp, all as root, in a directory the user
# owns -- after which their next ordinary `make` cannot write the stamp and the
# build fails with a permission error nobody connects to the sudo they typed
# an hour earlier. install.sh:263-273 already solves this: it compiles as the
# human and uses root for the copy alone.
# The folder https://satellite.foundation/ hands out when somebody clicks
# download: the two binaries this machine just built, and the installer beside
# them. Enterprise Linux is what it is because that is what this machine is --
# an EL 10 build links EL 10's libstdc++ and runs on EL 9/10 and its rebuilds,
# and on nothing older. A Debian bundle has to be built on Debian, which is why
# this variable names the distribution rather than the word "linux".
#
# Under the same condition as the auto-install below, and not on `every make`
# literally: debian/rules reaches `all` too, and a package build that dropped a
# directory of binaries into the unpacked source would have dpkg-source
# complaining about a tree that changed while it was being built.
#
# gitignored, because it holds build products. Nothing in it is a source file
# and every one of them is rewritten by the next make.
DOWNLOAD_DIR = enterprise_download
DOWNLOAD_TAR = satellite_rhel.tar.xz

# The data half of the bundle: the whole install tree, staged prefix-relative,
# so install.sh can put it somewhere without a Makefile, a compiler or a copy of
# the file list. Written by the `bundle` rule below, which explains all three.
# Under DOWNLOAD_DIR so that it travels inside the tarball with the binaries.
BUNDLE_TREE = $(DOWNLOAD_DIR)/install_tree
