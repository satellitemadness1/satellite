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

FORCE:

$(OBJS): $(HDRS) .cxxflags-stamp

.PHONY: FORCE
