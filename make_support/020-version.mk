# satellite 004 -- the build number, and what a build records about itself.
#
# AFTER 010-compiler.mk: the build fingerprint quotes $(CXX) and $(CXXFLAGS).
#
# VERSION, REVISION AND BUILD ARE ROWS, not make variables (the author,
# 2026-09-15): arguments.version, arguments.revision and arguments.build in
# satellite/config/satellite_config.hpp. A make whose inputs changed -- the files
# in BUILD_INPUTS (040-sources.mk), every library source, or the compiler and
# flags -- raises arguments.build by one BEFORE anything compiles; any other make
# leaves it alone. satellite/config/build_number.py says exactly when. Its stamp,
# .satellite_build (not in git), holds the last build's number and fingerprint.
#
# satl and satl-term both show the three numbers (satellite/version/), and each
# carries the rows it was compiled with.
BUILD_STAMP = .satellite_build

# The build machine's operating system for the title's third line ("CLANG++ 24
# ALMALINUX 10.2"): NAME and VERSION_ID from /etc/os-release, upper case, letters,
# digits, spaces, dots and dashes only.
BUILD_OS := $(shell (. /etc/os-release 2>/dev/null && echo "$$NAME $$VERSION_ID" || uname -sr) | tr a-z A-Z | tr -cd 'A-Z0-9 .-')
OS_DEFINE = -DSATELLITE_BUILD_OS='"$(BUILD_OS)"'

# What the fingerprint is told about the compiler: the same string the old
# Makefile passed, so moving to these fragments raised the number once (the
# Makefile itself is an input) and not for a different compiler description.
BUILD_DESCRIPTION = $(CXX) $(CXXFLAGS) $(BUILD_OS)
