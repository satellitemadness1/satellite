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

# THE STEM MAY CONTAIN A SLASH, which is why the rule above still covers
# src/programs/satl-term/ without an edit: make matches % against any part of the
# path, separators included. 070-clean.mk is the place that does NOT get this
# for free -- a shell glob is not a stem, and `src/*/*.o` never reaches a
# directory deeper.

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

# THE WINDOW OBJECTS, which need $(WINDOW_CFLAGS) and are therefore the one
# part of the tree the pattern rule above cannot compile: gtk4's headers are not
# under src/ and no -I this build knows about reaches them. 047-window.mk is
# where those flags come from and where the module name is argued.
#
# window.o also takes $(VERSION_DEFS), because `satl-term --version` prints the
# same version_text() satl does. The others do not -- they name no version, and
# giving them the defines would rebuild them on every version bump for nothing.
$(TERM_DIR)/window.o: $(TERM_DIR)/window.cpp .cxxflags-stamp $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $(WINDOW_CFLAGS) $(VERSION_DEFS) -c -o $@ $(TERM_DIR)/window.cpp

# AND ONE PATTERN RULE FOR ALL THE REST OF THEM, which is a change from the
# three explicit rules this held until File > New tab made them six. Six copies
# of one command line is a list somebody adds a file to and forgets, and what
# they get for forgetting is a gtk header not found, forty lines from the file
# they added.
#
# THE SHORTEST STEM WINS, and that is what makes this beat the generic
# $(SRC)/%.o above rather than tying with it: both patterns match
# src/programs/satl-term/keys.o, the generic one with a stem of
# `programs/satl-term/keys` and this one with `keys`, and make considers
# matching pattern rules in order of stem length, shortest first. Checked with
# `make -n $(TERM_DIR)/keys.o`, which must print a command carrying
# $(WINDOW_CFLAGS); if it ever does not, this rule has stopped being reached and
# every object here is being compiled without gtk on its include path. That
# failure is LOUD -- a header that cannot be found -- which is why the subtlety
# is affordable.
#
# window.o is above and not here because an explicit rule outranks any pattern,
# which is how it keeps its $(VERSION_DEFS) without opting out of anything.
$(TERM_DIR)/%.o: $(TERM_DIR)/%.cpp .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) $(WINDOW_CFLAGS) -c -o $@ $<

# satellite.random, which is the ONE object that sees a third-party header.
#
# -isystem AND NOT -I, and the first satellite found this the hard way:
# pcg_extras.hpp:223 warns under -Wall -Wextra, which this build turns on for
# everything. -isystem suppresses warnings from a header this project does not
# own and cannot fix without forking it. pcg/README.md carries the rest --
# which single type is used, and why 512-bit was investigated and refused.
$(RANDOM)/random.o: $(RANDOM)/random.cpp .cxxflags-stamp $(RANDOM)/random.hpp
	$(CXX) $(CXXFLAGS) -I$(SRC) -isystem pcg/include -c -o $@ $(RANDOM)/random.cpp

# The haswell twin, needed since M13 put random.cpp in SATL_SRCS: without it
# the generic %.haswell.o pattern below would compile the one PCG-naming
# object with -I semantics and the vendored header's warning would become
# ours -- exactly what -isystem exists to prevent, in the one variant nobody
# rebuilds by hand.
$(RANDOM)/random.haswell.o: $(RANDOM)/random.cpp .cxxflags-stamp-haswell $(RANDOM)/random.hpp
	$(CXX) $(CXXFLAGS) $(MARCH_HASWELL) -I$(SRC) -isystem pcg/include -c -o $@ $(RANDOM)/random.cpp

# The haswell half of the tree. A separate suffix rather than a separate
# directory, so that `clean` keeps naming what it removes and this rule stays
# one line like the baseline one above it.
#
# REACHED SINCE M2, and the prediction this comment used to make came true in
# the milestone that followed it. It read "UNUSED TODAY and deliberately
# present: both of satl's two sources have explicit rules below, which outrank
# any pattern, so nothing currently reaches this" -- then M2 added
# $(WORDS)/dump.cpp to SATL_SRCS, and `make -n satl.haswell` now compiles
# dump.haswell.o through this rule and no other.
#
# Kept as written because it is the reason adding that third source was a
# one-line edit in 040-sources.mk rather than a one-line edit plus a rule
# somebody had to notice was missing. The failure without it would have been a
# build looking for a dump.haswell.cpp that was never meant to exist.
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
