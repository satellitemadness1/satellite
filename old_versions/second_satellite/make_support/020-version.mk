# satellite -- the two version numbers, and what a build records about itself.
#
# AFTER 010-compiler.mk: VERSION_DEFS quotes $(CXX) and $(CXXFLAGS) into the
# binary, so it must be read once both are final. See src/system_facts/version.hpp.

# TWO NUMBERS, moving at different rates. VERSION is the language and changes
# rarely and deliberately; REVISION is this build of it and goes up as work
# lands. Overridable, so a packaging script can stamp its own without editing
# this file.
#
# 003 is the SECOND satellite. 001 was the version the first carried while its
# design was being written, 002 is what it became; the number moves here because
# the language is being rebuilt, not because this build is newer than that one.
# old_versions/first_satellite/ still answers 002 and always will.
SATELLITE_VERSION  ?= 003
SATELLITE_REVISION ?= 09

# When this build happened. SOURCE_DATE_EPOCH WINS WHENEVER IT IS SET, and that
# is not a nicety: dpkg exports it precisely so two builds of identical source
# produce identical binaries, and a stamp that read the wall clock instead would
# fail every reproducibility check a distribution runs. `date -u` because a
# timestamp without a zone means something different on each machine that reads
# it.
BUILD_STAMP := $(shell date -u $(if $(SOURCE_DATE_EPOCH),-d @$(SOURCE_DATE_EPOCH),) \
                   '+%Y-%m-%d %H:%M:%S UTC' 2>/dev/null || echo unrecorded)

# ON THE RECIPES THAT NEED THEM AND NEVER IN CXXFLAGS. CXXFLAGS is what
# .cxxflags-stamp records, and BUILD_STAMP changes every second -- so putting
# these there would rewrite the stamp on every invocation and recompile the
# whole tree, every time, forever, permanently silencing the one check that
# exists to catch a real flag change.
#
# $(CXX) is recorded as the path make INVOKED. What that path turned out to be
# is a separate fact, and version.hpp reads it from __VERSION__ instead. See
# 010-compiler.mk for the time those two disagreed and nothing said so.
#
# A FUNCTION, because satl is built twice and the two builds must not describe
# themselves identically. $(1) is whatever -march the caller compiled with, and
# it lands in the flags string, so `satl --version` on an installed binary says
# which of the two variants it is without anything having to record that
# separately. The install writes no manifest and needs none: the binary is the
# record. See make_support/045-microarchitecture.mk.
#
# $(strip) so the baseline call -- $(call version_defs,) with an empty argument
# -- does not bake a trailing space into the string a user reads.
#
# MARCH_HASWELL is defined in 045-microarchitecture.mk, which make reads after
# this file. That is fine and is the arrangement the top-level Makefile
# describes: these are recursively expanded, so the reference below is resolved
# when a recipe uses it, by which point every fragment has been read.

# WHICH MAKE RAN, for `arguments.build.make` `1 14 1 1 5 5`. MAKEFLAGS IS
# CLEARED for this one sub-invocation: $(MAKE) carries the current flags, and
# this value is compiled into an object as a string a program can read -- so
# anything that perturbs the sub-make's output gets baked in and stays there
# until that object is rebuilt. The first satellite observed a
# `make_version: # GNU Make 4.4.1` in a built binary on 2026-08-24 and a clean
# rebuild cleared it; the exact trigger was never reproduced, so this is
# defensive rather than a proven fix. $(MAKE) rather than a literal `make`, so
# a differently-named make still reports itself honestly.
SATELLITE_MAKE := $(shell MAKEFLAGS= $(MAKE) --version 2>/dev/null | head -1)

version_defs = -DSATELLITE_BUILD_MAKE='"$(SATELLITE_MAKE)"' \
               -DSATELLITE_VERSION='"$(SATELLITE_VERSION)"' \
               -DSATELLITE_REVISION='"$(SATELLITE_REVISION)"' \
               -DSATELLITE_BUILT='"$(BUILD_STAMP)"' \
               -DSATELLITE_BUILD_CXX='"$(CXX)"' \
               -DSATELLITE_BUILD_FLAGS='"$(strip $(CXXFLAGS) $(1))"'

VERSION_DEFS         = $(call version_defs,)
VERSION_DEFS_HASWELL = $(call version_defs,$(MARCH_HASWELL))
