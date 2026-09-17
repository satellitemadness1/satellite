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

# What the fingerprint is told about the compiler and the links. EVERY FLAG THAT
# CHANGES A BINARY IS IN IT (review of M0.5): LDFLAGS and the window's pkg-config
# flags relinked satl, satl-term and every library under the same number before.
# CXX_VERSION is the compiler's own first line, because clang-current is a symlink
# repointed at each rebuilt clang -- the same CXX string, a different compiler.
CXX_VERSION := $(shell $(CXX) --version 2>/dev/null | head -n 1)
BUILD_DESCRIPTION = $(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(BUILD_OS) | $(LDFLAGS) | $(WINDOW_CFLAGS) | $(WINDOW_LIBS)
