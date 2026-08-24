# satellite -- where an install goes, and who decides.
#
# prefix is compiled INTO the binary (see 110-compile.mk's .libdir-stamp);
# DESTDIR is staging and is compiled into nothing. 090-autoinstall.mk reads both.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# `prefix` is baked into the binary; DESTDIR is staging and is baked into
# NOTHING. The distinction is the whole contract with a packaging system: a
# .deb is built with prefix=/usr into a DESTDIR chroot, and a binary that had
# learned the chroot's name would look for its library there on the user's
# machine, where that directory does not exist.
#
# The DEFAULT follows who is running make, for the reason install.sh already
# gives at length (install.sh:40-58) and now for a second one: `all` below ends
# in an install, and an install a normal user cannot perform is not a default,
# it is an error message. /usr/local is the right place for an install from
# source and it needs root; $HOME/.local needs none, is already on PATH, and is
# in the XDG data search path on any current desktop.
#
# id -u rather than $(HOME) alone: `sudo make` runs with HOME=/root, and a
# default of $(HOME)/.local would drop a system build into /root/.local, which
# is on nobody's PATH -- the same trap the note on LLVM_BIN above is about. A
# HOME that is unset entirely falls back to /usr/local, where the writability
# check in `all` refuses the install rather than inventing /.local.
#
# ONE prefix per invocation, and that is not a style point. prefix is compiled
# into system.o through .libdir-stamp below, so a build at one prefix followed
# by an install at another recompiles system.o and relinks satl -- in BOTH
# directions, on every invocation, so `make` never reaches a fixed point. That
# is why the auto-install below overrides nothing: it installs at the prefix
# this build already used.
SATELLITE_UID := $(shell id -u 2>/dev/null)
ifeq ($(SATELLITE_UID),0)
  DEFAULT_PREFIX = /usr/local
else ifeq ($(strip $(HOME)),)
  DEFAULT_PREFIX = /usr/local
else
  DEFAULT_PREFIX = $(HOME)/.local
endif

prefix  ?= $(DEFAULT_PREFIX)
DESTDIR ?=
bindir   = $(prefix)/bin
datadir  = $(prefix)/share
mandir   = $(datadir)/man
docdir   = $(datadir)/doc/satellite
