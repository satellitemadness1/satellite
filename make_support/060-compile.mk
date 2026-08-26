# satellite -- how a .cpp becomes a .o.
#
# The pattern rule covers every source in the tree. The exception below learns
# one thing the others do not: what this build is.

# Every translation unit spells its includes from the top of src/ --
# "system_facts/version.hpp" and never "../version.hpp" -- so the path a header
# is included by is a property of the header and not of whoever reached for it.
# That costs one -I, and this rule is where objects under src/ get it.
#
# -I$(SRC) is a LITERAL on the recipe line rather than part of CXXFLAGS, and
# that is the same argument VERSION_DEFS makes in 020-version.mk from the other
# direction: a command-line `make CXXFLAGS=...` replaces that variable WHOLE,
# and a distribution's rules file does precisely that. A flag the build cannot
# compile a single file without is not one an override may silently drop. The
# failure mode is every unit failing to find every header, in a package build,
# on a machine that is not this one.
$(SRC)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC) -c -o $@ $<

# main.o is the object that learns what this build IS. An explicit rule, so that
# a change to the version invalidates one object rather than the whole tree.
$(PROGRAMS)/main.o: $(PROGRAMS)/main.cpp .cxxflags-stamp $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $(VERSION_DEFS) -c -o $@ $(PROGRAMS)/main.cpp

# The same, and the reason is that the opening information names the version
# too. It reads version_line() rather than carrying a literal, so the banner and
# --version cannot drift apart -- which they had in the first satellite, where
# the prompt still said 0.1 long after the language said 002.
$(PROGRAMS)/opening.o: $(PROGRAMS)/opening.cpp .cxxflags-stamp $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $(VERSION_DEFS) -c -o $@ $(PROGRAMS)/opening.cpp

# The haswell half of the tree. A separate suffix rather than a separate
# directory, so that `clean` keeps naming what it removes and this rule stays
# one line like the baseline one above it.
#
# UNUSED TODAY and deliberately present: both of satl's two sources have
# explicit rules below, which outrank any pattern, so nothing currently reaches
# this. It is what makes adding a third source to SATL_SRCS a one-line edit in
# 040-sources.mk instead of a one-line edit plus a rule somebody has to notice
# is missing -- and the failure without it is a build looking for a
# main.haswell.cpp that was never meant to exist.
$(SRC)/%.haswell.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) $(MARCH_HASWELL) -I$(SRC) -c -o $@ $<

# The two objects that learn what this build is, again, for the other variant.
# VERSION_DEFS_HASWELL differs from VERSION_DEFS in exactly one string: the
# flags. That is what makes an installed binary able to say which of the two it
# is -- `satl --version` prints that line -- so the install needs no manifest
# and cannot have one that disagrees with the file it describes.
$(PROGRAMS)/main.haswell.o: $(PROGRAMS)/main.cpp .cxxflags-stamp-haswell $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) $(MARCH_HASWELL) -I$(SRC) $(VERSION_DEFS_HASWELL) -c -o $@ $(PROGRAMS)/main.cpp

$(PROGRAMS)/opening.haswell.o: $(PROGRAMS)/opening.cpp .cxxflags-stamp-haswell $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) $(MARCH_HASWELL) -I$(SRC) $(VERSION_DEFS_HASWELL) -c -o $@ $(PROGRAMS)/opening.cpp

# make invalidates a target when a PREREQUISITE changes, and CXXFLAGS is not a
# prerequisite of anything. So without this, `make OPT=-O3` over a tree built at
# -O2 recompiles NOTHING, and the result is a mixed binary that no output
# distinguishes from a clean one. Switching compilers has the same hole.
#
# CXX is in the stamp alongside the flags, because who compiled an object file
# is as much a property of it as what flags did.
.cxxflags-stamp: FORCE
	@printf '%s' '$(CXX) $(CXXFLAGS)' | cmp -s - $@ || \
	    printf '%s' '$(CXX) $(CXXFLAGS)' > $@

# The same guard for the haswell objects, and a SECOND stamp rather than one
# covering both. With one stamp, changing MARCH_HASWELL would rebuild the
# baseline objects too -- which is not wrong, merely a lie about what changed --
# and, worse, the two variants would share a file whose content could only
# describe one of them. Two stamps, each holding the exact command line its own
# objects were compiled with.
.cxxflags-stamp-haswell: FORCE
	@printf '%s' '$(CXX) $(CXXFLAGS) $(MARCH_HASWELL)' | cmp -s - $@ || \
	    printf '%s' '$(CXX) $(CXXFLAGS) $(MARCH_HASWELL)' > $@

FORCE:

$(OBJS): $(HDRS) .cxxflags-stamp
$(SATL_HASWELL_OBJS): .cxxflags-stamp-haswell

.PHONY: FORCE
