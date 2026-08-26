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
SATELLITE_REVISION ?= 01

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
VERSION_DEFS = -DSATELLITE_VERSION='"$(SATELLITE_VERSION)"' \
               -DSATELLITE_REVISION='"$(SATELLITE_REVISION)"' \
               -DSATELLITE_BUILT='"$(BUILD_STAMP)"' \
               -DSATELLITE_BUILD_CXX='"$(CXX)"' \
               -DSATELLITE_BUILD_FLAGS='"$(CXXFLAGS)"'
