# satellite -- the flags that are not CXXFLAGS: gtk, vte, pcg and the linker.
#
# Reads $(PKGS) from 010-packages.mk. Every variable here is recursively
# expanded on purpose, which is what keeps pkg-config out of a `make satl`.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# Recursively expanded, so pkg-config is run only by the recipes that use them
# -- window.o and the satl-term link -- and never by a `make satl` or a
# `make test`. Errors are dropped because MISSING_PKGS above has already asked
# the question properly and reported it; a second complaint from the same
# missing package, in pkg-config's words, at the top of an otherwise fine build,
# is the noise this replaces.
GTKFLAGS = $(shell $(PKG_CONFIG) --cflags $(PKGS) 2>/dev/null)
LDLIBS   = $(shell $(PKG_CONFIG) --libs $(PKGS) 2>/dev/null)

# pcg-cpp is header-only and vendored in the tree, so it adds nothing to the
# link -- `ldd satl` lists the same six shared objects it did before §18.
#
# -isystem, not -I, and that is not a style preference: pcg_extras.hpp:223
# raises -Wunused-but-set-parameter under this file's own -Wall -Wextra, and §9
# makes a silent from-scratch rebuild a property the build has to keep. A
# warning from a vendored header is one nobody here can fix without forking it.
PCGFLAGS = -isystem pcg-cpp-0.98/include

# LDFLAGS is set nowhere in this file on purpose, so that a distribution's
# link-time hardening -- -Wl,-z,relro,-z,now from dpkg-buildflags, and whatever
# the next one adds -- arrives from the environment or the command line and
# reaches both link rules below. A Makefile that never mentions the variable is
# a Makefile those flags cannot reach.
LDFLAGS ?=
