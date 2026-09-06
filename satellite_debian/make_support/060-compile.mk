# satellite -- how a .cpp becomes a .o, out of tree.
#
# THE PARENT'S 060-compile.mk WITH THE OUTPUT MOVED. Every rule below is the
# parent's rule with $(OBJ)/ in front of the target instead of the object
# sitting beside its source, and every reason is the parent's reason.
#
# AFTER 030-output.mk, which defines the map that put $(OBJ) in those names.

# THE OBJECT DIRECTORIES, created before any object is written into them.
#
# ORDER-ONLY, after the bar, for the reason 050-build.mk gives about build/: a
# directory's timestamp moves every time a file lands in it, so an ordinary
# prerequisite would make every object in a directory out of date as soon as the
# next one was compiled beside it, and `make` twice would rebuild the tree.
#
# $(OBJS) IS THE PARENT'S VARIABLE, unchanged and still recursive, resolving
# through the five lists 030-output.mk overrode. So this line covers every
# object this build can make, including a module directory added to the language
# tomorrow, and 030 derives $(OBJ_DIRS) from the same set.
$(OBJS): | $(OBJ_DIRS)

$(OBJ_DIRS):
	mkdir -p $@

# Every translation unit spells its includes from the top of src/ --
# "system_facts/version.hpp" and never "../version.hpp" -- so the path a header
# is included by is a property of the header and not of whoever reached for it.
# That costs one -I, and this rule is where it is spent. Out of tree it is
# -I../src rather than -Isrc, which is the same statement about the same
# directory.
#
# -I$(SRC) IS A LITERAL ON THE RECIPE LINE rather than part of CXXFLAGS, for the
# parent's reason: a command-line `make CXXFLAGS=...` replaces that variable
# WHOLE, and a distribution's rules file does precisely that. A flag the build
# cannot compile a single file without is not one an override may silently drop.
$(OBJ)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC) -c -o $@ $<

# main.o is the object that learns what this build IS. An explicit rule, so that
# a change to the version invalidates one object rather than the whole tree.
$(OBJ)/programs/main.o: $(SRC)/programs/main.cpp $(CXXFLAGS_STAMP) $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $(VERSION_DEFS) -c -o $@ $(SRC)/programs/main.cpp

# The same, because the opening information names the version too. It reads
# version_line() rather than carrying a literal, so the banner and --version
# cannot drift apart -- which they had in the first satellite, where the prompt
# still said 0.1 long after the language said 002.
$(OBJ)/programs/opening.o: $(SRC)/programs/opening.cpp $(CXXFLAGS_STAMP) $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $(VERSION_DEFS) -c -o $@ $(SRC)/programs/opening.cpp

# THE TWO WINDOW OBJECTS, which need $(WINDOW_CFLAGS) and are therefore the one
# part of the tree the pattern rule above cannot compile: gtk4's headers are not
# under src/ and no -I this build knows about reaches them.
#
# window.o also takes $(VERSION_DEFS), because `satl-term --version` prints the
# same version_text() satl does. terminal.o does not -- it names no version, and
# giving it the defines would rebuild it on every version bump for nothing.
# THE PATHS ARE THE PARENT'S, one directory deeper since satl-term's sources
# moved into src/programs/satl-term/ on 2026-09-06. $(TERM_DIR) is inherited and
# resolves through $(SRC), so the source side needs no spelling here; the target
# side is $(OBJ)/programs/satl-term/, which 030-output.mk's $(OBJ_DIRS) already
# creates because it derives the directory set from $(OBJS).
$(OBJ)/programs/satl-term/window.o: $(TERM_DIR)/window.cpp $(CXXFLAGS_STAMP) $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $(WINDOW_CFLAGS) $(VERSION_DEFS) -c -o $@ $(TERM_DIR)/window.cpp

$(OBJ)/programs/satl-term/terminal.o: $(TERM_DIR)/terminal.cpp $(CXXFLAGS_STAMP)
	$(CXX) $(CXXFLAGS) -I$(SRC) $(WINDOW_CFLAGS) -c -o $@ $(TERM_DIR)/terminal.cpp

$(OBJ)/programs/satl-term/keys.o: $(TERM_DIR)/keys.cpp $(CXXFLAGS_STAMP)
	$(CXX) $(CXXFLAGS) -I$(SRC) $(WINDOW_CFLAGS) -c -o $@ $(TERM_DIR)/keys.cpp

# satellite.random, which is the ONE object that sees a third-party header.
#
# -isystem AND NOT -I, and the first satellite found this the hard way:
# pcg_extras.hpp:223 warns under -Wall -Wextra, which this build turns on for
# everything. -isystem suppresses warnings from a header this project does not
# own and cannot fix without forking it.
#
# $(PCG) AND NOT `pcg/include`, which is the parent's spelling and is relative
# to the parent's working directory. 010-tree.mk holds the path because where
# pcg is, is a fact about the tree.
$(OBJ)/satellite_random/random.o: $(SRC)/satellite_random/random.cpp $(CXXFLAGS_STAMP) $(RANDOM)/random.hpp
	$(CXX) $(CXXFLAGS) -I$(SRC) -isystem $(PCG) -c -o $@ $(SRC)/satellite_random/random.cpp

# The haswell half of the tree. A separate suffix rather than a separate
# directory, so this rule stays one line like the baseline one above it and
# 070-clean.mk keeps naming what it removes.
#
# THE TWO PATTERNS BOTH MATCH A .haswell.o AND THE SHORTER STEM WINS, which is
# what makes this work rather than a coincidence. build/objects/programs/
# main.haswell.o matches `$(OBJ)/%.o` with the stem `programs/main.haswell` and
# `$(OBJ)/%.haswell.o` with the stem `programs/main`; GNU make prefers the rule
# whose stem is shorter, so the haswell rule is chosen. The baseline rule could
# not have built it anyway -- it would want a main.haswell.cpp that was never
# meant to exist -- but the choice is made on stem length before that is ever
# discovered, which is worth knowing before adding a third suffix.
$(OBJ)/%.haswell.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) $(MARCH_HASWELL) -I$(SRC) -c -o $@ $<

# The two objects that learn what this build is, again, for the other variant.
# VERSION_DEFS_HASWELL differs from VERSION_DEFS in exactly one string: the
# flags. That is what makes an installed binary able to say which of the two it
# is -- `satl --version` prints that line -- so an install needs no manifest and
# cannot have one that disagrees with the file it describes.
$(OBJ)/programs/main.haswell.o: $(SRC)/programs/main.cpp $(CXXFLAGS_STAMP_HASWELL) $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) $(MARCH_HASWELL) -I$(SRC) $(VERSION_DEFS_HASWELL) -c -o $@ $(SRC)/programs/main.cpp

$(OBJ)/programs/opening.haswell.o: $(SRC)/programs/opening.cpp $(CXXFLAGS_STAMP_HASWELL) $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) $(MARCH_HASWELL) -I$(SRC) $(VERSION_DEFS_HASWELL) -c -o $@ $(SRC)/programs/opening.cpp

# make invalidates a target when a PREREQUISITE changes, and CXXFLAGS is not a
# prerequisite of anything. So without this, `make OPT=-O3` over a tree built at
# -O2 recompiles NOTHING, and the result is a mixed binary that no output
# distinguishes from a clean one. Switching compilers has the same hole.
#
# CXX is in the stamp alongside the flags, because who compiled an object file
# is as much a property of it as what flags did.
#
# | $(BUILD), because these live in build/ and are written by a recipe that
# would otherwise be the first thing to run in an empty directory -- before
# anything else has had cause to create it.
$(CXXFLAGS_STAMP): FORCE | $(BUILD)
	@printf '%s' '$(CXX) $(CXXFLAGS)' | cmp -s - $@ || \
	    printf '%s' '$(CXX) $(CXXFLAGS)' > $@

# The same guard for the haswell objects, and a SECOND stamp rather than one
# covering both. With one stamp, changing MARCH_HASWELL would rebuild the
# baseline objects too -- which is not wrong, merely a lie about what changed --
# and, worse, the two variants would share a file whose content could only
# describe one of them.
$(CXXFLAGS_STAMP_HASWELL): FORCE | $(BUILD)
	@printf '%s' '$(CXX) $(CXXFLAGS) $(MARCH_HASWELL)' | cmp -s - $@ || \
	    printf '%s' '$(CXX) $(CXXFLAGS) $(MARCH_HASWELL)' > $@

FORCE:

# $(HDRS) IS THE PARENT'S LIST, resolving to ../src/... through the SRC override
# in 020-inherited.mk. Every object depends on every header, which is a blunt
# instrument the parent chose on purpose over -MMD: generated .d files would make
# a from-scratch build depend on files a clean has removed. words.def is in that
# list and is not a header -- it is the file that actually changes when the
# language gains a word.
$(OBJS): $(HDRS) $(CXXFLAGS_STAMP)
$(SATL_HASWELL_OBJS): $(CXXFLAGS_STAMP_HASWELL)

.PHONY: FORCE
