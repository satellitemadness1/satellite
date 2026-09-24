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
# A REVISION RESTARTS THE COUNT (the author, 2026-09-19). Raise arguments.revision by
# hand and the next make sets arguments.build back to 1 rather than raising it, so BUILD
# means "the Nth build of THIS revision" and not of satellite since the beginning:
#
#     satellite: REVISION 05 -- BUILD restarts at 0001
#
# The stamp carries the revision as a third field for exactly this -- `1 <fingerprint> 5`
# -- because the reset is the only case allowed to write a build number BELOW the one the
# stamp holds, which every other time means an editor saved a stale config and is refused.
# A stamp written before this rule has no third field; that reads as "unknown" and resets
# nothing, so upgrading never throws a count away. Nothing needs to change in this file
# when a revision is raised: edit the row, run make.
#
# satl shows the three numbers (satellite/version/) and carries the rows it was
# compiled with.
BUILD_STAMP = .satellite_build

# The build machine's operating system for the title's third line ("CLANG++ 24
# ALMALINUX 10.2"): NAME and VERSION_ID from /etc/os-release, upper case, letters,
# digits, spaces, dots and dashes only.
BUILD_OS := $(shell (. /etc/os-release 2>/dev/null && echo "$$NAME $$VERSION_ID" || uname -sr) | tr a-z A-Z | tr -cd 'A-Z0-9 .-')
OS_DEFINE = -DSATELLITE_BUILD_OS='"$(BUILD_OS)"'

# What the fingerprint is told about the compiler and the links. EVERY FLAG THAT
# CHANGES A BINARY IS IN IT (review of M0.5): LDFLAGS relinked satl and every
# library under the same number before. Which GTK is linked and whether the
# console is in it say what the window half of satl is.
# CXX_VERSION is the compiler's own first line, because clang-current is a symlink
# repointed at each rebuilt clang -- the same CXX string, a different compiler.
CXX_VERSION := $(shell $(CXX) --version 2>/dev/null | head -n 1)
# WITHOUT ONE PROCESSOR'S -march (010-compiler.mk's CPU): every processor's build of BUILD
# N is made from exactly what the ordinary BUILD N was, and says so with its fingerprint.
# AND HOW IT WAS OPTIMISED (045-optimise.mk) -- the word, never the training build's flags,
# so the training build describes itself as the build it trains for.
BUILD_DESCRIPTION = $(CXX) [$(CXX_VERSION)] $(filter-out -march=$(CPU),$(CXXFLAGS)) $(BUILD_OS) | $(LDFLAGS) | $(GTK_KIND) | $(CONSOLE_DEFINE) | $(or $(OPTIMISE_KIND),plain)
