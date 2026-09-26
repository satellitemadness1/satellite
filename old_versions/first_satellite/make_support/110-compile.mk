# satellite -- how a .cpp becomes a .o, and the four objects that need more.
#
# The pattern rule covers every source in the tree. The exceptions below each
# learn something the others do not: what this build is, where gtk lives, where
# the vendored PCG headers are, and where the library will be installed.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# Every translation unit spells its includes from the top of src/ --
# "evaluator/eval.hpp" and never "../eval.hpp" -- so the path a header is
# included by is a property of the header and not of whoever reached for it.
# That costs one -I, and this rule is where the objects under src/ get it.
#
# -I$(SRC) is a LITERAL on every recipe line rather than part of CXXFLAGS, for
# exactly the reason -DSATELLITE_LIB_DIR is one on the system.o rule below: a
# command-line `make CXXFLAGS=...` replaces that variable WHOLE, and
# debian/rules does precisely that -- it restates -std=c++20 -Wall -Wextra
# ahead of dpkg's hardening flags, and would have to learn to restate a -I as
# well. A flag the build cannot compile a single file without is not one an
# override may silently drop; the failure mode is every unit failing to find
# every header, in a package build, on a machine that is not this one.
# -I. arrived with Satellite Orbit, which is the one module directory outside
# src/: methods_containers.cpp includes "satellite_orbit_search/orbit.hpp" to
# reach .orbit(), and that path is root-relative for exactly the reason every
# path under src/ is src-relative -- a header's include path is a property of
# the header and not of whoever reached for it.
$(SRC)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I. -c -o $@ $<

# The same rule for the one module that is not under src/. It needs TWO -I:
# -I$(SRC) so it can include "evaluator/search.hpp" like everything else, and
# -I. so its own headers are reached as "satellite_orbit_search/orbit.hpp" --
# root-relative, which is the same property the rule above buys for src/. A
# header's include path stays a property of the header and not of who reached
# for it, which is the whole point of the note above.
$(ORBIT)/%.o: $(ORBIT)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I. -c -o $@ $<

# main.o is one of the two objects that learn what this build IS, the same way
# system.o is the one that learns where it will live. Both are explicit rules
# for the same reason: the define belongs to one translation unit, so a change
# to it invalidates one object rather than the whole tree.
$(PROGRAMS)/main.o: $(PROGRAMS)/main.cpp .cxxflags-stamp $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $(VERSION_DEFS) -c -o $@ $(PROGRAMS)/main.cpp

# The third, and the reason is that satellite.help's banner names the version
# too. It reads version_line() rather than carrying a literal, so the prompt,
# the help text and --version cannot drift apart -- which they had: the banner
# still said 0.1 on 2026-08-24.
$(EVAL)/help.o: $(EVAL)/help.cpp .cxxflags-stamp $(SYSTEM)/version.hpp
	$(CXX) $(CXXFLAGS) -I$(SRC) $(VERSION_DEFS) -c -o $@ $(EVAL)/help.cpp

# The one object that needs gtk and vte, and so the one place the missing-package
# check has to bite: the guard is inside this recipe rather than on a
# prerequisite because a phony prerequisite is always considered newer, which
# would rebuild window.o on every make even when nothing had changed.
$(PROGRAMS)/window.o: $(PROGRAMS)/window.cpp .cxxflags-stamp $(SYSTEM)/version.hpp
ifneq ($(MISSING_PKGS),)
	@printf 'satl-term needs %s, which pkg-config cannot find.\n' '$(MISSING_PKGS)'
	@printf 'To install it, %s\n' '$(DEPS_ADVICE)'
	@printf 'satl, the interpreter, needs neither and builds with: make satl\n'
	@exit 1
endif
	$(CXX) $(CXXFLAGS) $(GTKFLAGS) -I$(SRC) $(VERSION_DEFS) -c -o $@ $(PROGRAMS)/window.cpp

# The only object that sees the vendored PCG headers, for the same reason
# window.o is the only one that sees gtk: a dependency that one translation unit
# needs is not one the whole build should carry. $(NUMBER)/random.cpp does the
# arbitrary-precision half of §18 and includes nothing from pcg-cpp at all.
$(RANDOM)/random.o: $(RANDOM)/random.cpp
	$(CXX) $(CXXFLAGS) $(PCGFLAGS) -I$(SRC) -c -o $@ $(RANDOM)/random.cpp

# The prefix is written to a file so that make can see it. Make invalidates a
# target when a PREREQUISITE changes, and the value below is not a prerequisite
# of anything -- so without this stamp, `make && make install prefix=/usr`
# reuses the system.o compiled for /usr/local and installs a binary whose
# last-resort library directory names a prefix nothing was ever installed to,
# while `satl --where` reports that directory as fact. The stamp is
# rewritten only when the value actually changes, so a rebuild at the same
# prefix still recompiles nothing.
.libdir-stamp: FORCE
	@printf '%s' '$(datadir)/satellite/lib' | cmp -s - $@ || \
	    printf '%s' '$(datadir)/satellite/lib' > $@

# The same trick as .libdir-stamp, for the same reason, against a bigger hole:
# make invalidates a target when a PREREQUISITE changes, and CXXFLAGS is not a
# prerequisite of anything. So `make OPT=-O3` used to recompile NOTHING, and a
# tree that had been built at -O2 quietly stayed at -O2 except for whatever
# happened to be touched since -- a mixed binary that no output distinguishes
# from a clean one. Switching compilers had the same hole: the whole tree was
# built by GCC, CXX moved to clang, and `make` had nothing to say about it.
#
# CXX is in the stamp with the flags, because who compiled it is as much a
# property of an object file as what flags did.
.cxxflags-stamp: FORCE
	@printf '%s' '$(CXX) $(CXXFLAGS)' | cmp -s - $@ || \
	    printf '%s' '$(CXX) $(CXXFLAGS)' > $@

FORCE:

# system.o is the only object that learns the install prefix, so retargeting a
# build invalidates one object and not twelve. DESTDIR is deliberately absent
# from this define -- see the prefix/DESTDIR note above.
# VERSION_DEFS joins this recipe because arguments_for() reports what built the
# interpreter — the compiler, its flags, the make, the build stamp — and those
# are the defines that carry them. Same three-recipe argument as above: not in
# CXXFLAGS, or the build stamp rebuilds everything every time.
$(SYSTEM)/system.o: $(SYSTEM)/system.cpp .libdir-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) \
	    $(VERSION_DEFS) -DSATELLITE_LIB_DIR='"$(datadir)/satellite/lib"' \
	    -c -o $@ $(SYSTEM)/system.cpp

$(OBJS): $(HDRS) .cxxflags-stamp
