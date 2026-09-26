# satellite -- the GUI packages, and what to type when they are missing.
#
# First of sixteen fragments the Makefile at the top of this tree includes, in
# the order they are numbered. Nothing above this; MISSING_PKGS is a := and is
# therefore measured here, once, and read later by 040-flags.mk (GTKFLAGS),
# 080-sources.mk (GUI_TARGET) and two recipes.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

PKGS     = gtk4 vte-2.91-gtk4

# pkg-config, and whether this machine has what satl-term needs.
#
# `make` used to die here, and the way it died is the point: PKGS is only ever
# needed by ONE object, window.o, but `all` asked for satl-term unconditionally,
# so a machine without the vte development package got pkg-config's complaint
# followed by `fatal error: gtk/gtk.h: No such file or directory` -- and no
# satl either, because make stops at the first failed recipe. The interpreter
# needs neither library (see the note on CXXFLAGS below) and there was no reason
# for it to go down with the window.
#
# So the packages are LOOKED FOR rather than assumed. Present, `make` builds
# both, exactly as before. Absent, it builds the interpreter, says which package
# is missing and how to install it, and exits 0 -- and `make satl-term` still
# fails, loudly and with the same instruction, because someone who asked for the
# window by name wants to know why they did not get it.
#
# --exists rather than --cflags: it answers yes or no and prints nothing either
# way, so this is the one place the question is asked and the noise stops here.
# := so it is asked once per make rather than once per reference.
PKG_CONFIG ?= pkg-config
MISSING_PKGS := $(strip $(foreach p,$(PKGS),     $(if $(shell $(PKG_CONFIG) --exists $(p) 2>/dev/null && echo yes),,$(p))))

# What to type to fix it, in the words of whichever package manager is here.
# A message that says "install the vte development package" and leaves the
# reader to find out what it is called on their distribution is half a message.
ifneq ($(shell command -v dnf 2>/dev/null),)
  # The vte gtk4 bindings live in CodeReady Builder on RHEL and its rebuilds,
  # which is disabled by default -- so the repository has to be named or dnf
  # reports "No match for argument" on a package that is sitting right there.
  DEPS_CMD = sudo dnf install --enablerepo=crb gtk4-devel vte291-gtk4-devel
else ifneq ($(shell command -v apt-get 2>/dev/null),)
  # The same names debian/control Build-Depends on, so the two cannot drift.
  DEPS_CMD = sudo apt-get install libgtk-4-dev libvte-2.91-gtk4-dev pkg-config
else ifneq ($(shell command -v pacman 2>/dev/null),)
  DEPS_CMD = sudo pacman -S --needed gtk4 vte4
else
  DEPS_CMD =
endif

# The message itself, once, because three recipes print it.
ifeq ($(DEPS_CMD),)
  DEPS_ADVICE = install the gtk4 and vte-2.91-gtk4 development packages
else
  DEPS_ADVICE = run:  make deps    (which is: $(DEPS_CMD))
endif
