# satellite -- the two version numbers, and what a build records about itself.
#
# AFTER 020-compiler.mk: VERSION_DEFS quotes $(CXX) and $(CXXFLAGS) into the
# binary, so it must be read once both are final. See version.hpp.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# TWO NUMBERS, moving at different rates. VERSION is the language and changes
# rarely; REVISION is this build of it and goes up as work lands. Overridable,
# so a packaging script can stamp its own without editing this file.
SATELLITE_VERSION  ?= 002
SATELLITE_REVISION ?= 01

# When this build happened. SOURCE_DATE_EPOCH WINS WHENEVER IT IS SET, and that
# is not a nicety: dpkg exports it precisely so that two builds of identical
# source produce identical binaries, and a stamp that read the wall clock
# instead would fail every reproducibility check Debian runs -- on packages
# this repo actually ships, and marked Architecture: any, so on every port.
# `date -u` because a timestamp without a zone is a timestamp that means
# something different on each machine that reads it.
BUILD_STAMP := $(shell date -u $(if $(SOURCE_DATE_EPOCH),-d @$(SOURCE_DATE_EPOCH),)                    '+%Y-%m-%d %H:%M:%S UTC' 2>/dev/null || echo unrecorded)

# On TWO recipes and never in CXXFLAGS. CXXFLAGS is what .cxxflags-stamp
# records, and BUILD_STAMP changes every second -- so putting these there would
# rewrite the stamp on every invocation and rebuild all forty-three objects,
# every time, forever, permanently silencing the one check that exists to catch
# a real flag change. This is the same reasoning that keeps -DSATELLITE_LIB_DIR
# on the system.o recipe alone.
#
# $(CXX) is recorded as the path make INVOKED. What that path turned out to be
# is a separate fact and version.hpp reads it from __VERSION__, because this
# project has already been bitten once by the two disagreeing: LLVM_BIN pointed
# at a directory that did not exist, `c++` answered instead, and every figure
# attributed to clang was GCC's with nothing anywhere saying so.
# The make that drove this build, first line of its --version. It joins
# VERSION_DEFS rather than CXXFLAGS for the reason version.hpp:12 gives about
# the build stamp: a define in CXXFLAGS rewrites .cxxflags-stamp, and a value
# that can change would rebuild all 43 objects on every make, forever. Here it
# reaches exactly the three recipes that need it.
#
# := so the sub-shell runs once per make and not once per recipe, and the
# stderr redirect so a make that has no --version leaves the fallback in
# system.cpp to answer `unrecorded` rather than putting an error message in a
# string a program can read.
# MAKEFLAGS is CLEARED for this one sub-invocation. $(MAKE) carries the current
# flags, and this value is baked into system.o as a string a program can read --
# so anything that perturbs the sub-make's output gets compiled in and stays
# there until that object is rebuilt. A `make_version: # GNU Make 4.4.1` was
# observed in a built binary on 2026-08-24 and a clean rebuild of system.o
# cleared it; the exact trigger was NOT reproduced, so this is defensive rather
# than a proven fix. $(MAKE) is kept rather than a literal `make` so that a
# differently-named make still reports itself honestly.
SATELLITE_MAKE_VERSION := $(shell MAKEFLAGS= $(MAKE) --version 2>/dev/null | head -1)

VERSION_DEFS = -DSATELLITE_BUILD_MAKE='"$(SATELLITE_MAKE_VERSION)"' \
               -DSATELLITE_VERSION='"$(SATELLITE_VERSION)"' \
               -DSATELLITE_REVISION='"$(SATELLITE_REVISION)"' \
               -DSATELLITE_BUILT='"$(BUILD_STAMP)"' \
               -DSATELLITE_BUILD_CXX='"$(CXX)"' \
               -DSATELLITE_BUILD_FLAGS='"$(CXXFLAGS)"'
