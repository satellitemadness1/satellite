# satellite -- the default goal and the binaries.
#
# THE FIRST FRAGMENT THAT DECLARES A TARGET, which is what makes `all` the first
# target make sees. .DEFAULT_GOAL below says it outright anyway, because after a
# split like this one the order of the rules is the order of the includes, and a
# rule that lands in the wrong file should not be able to change what `make`
# does.

.DEFAULT_GOAL := all

# FOUR BINARIES ON AN x86-64 DESKTOP AND ONE ON A MACHINE WITH NEITHER
# gtk4 NOR x86-64. satl-term is the fourth and it is conditional on its
# libraries rather than on the instruction set -- 047-window.mk is where that
# question is asked and why. PLAN.md M11.A says this table becomes four and two
# at that milestone; the window landed ahead of the prompt it will host.
#
# THREE BINARIES ON x86-64 AND ONE EVERYWHERE ELSE. satl.haswell and satl are
# the same sources compiled against two different instruction sets, and
# satl-cpu-level is the program install.sh runs to find out which of them this
# machine can execute. 045-microarchitecture.mk decides which case this is, and
# says why the question is put to the compiler rather than to `uname`.
#
# The variants are built by `all` rather than only by the installer, because a
# build that only happens during an install is a build nobody sees fail until
# they are installing. `make` builds all three; install.sh calls this same make.
ALL_TARGETS = satl
ifeq ($(MICROARCH_VARIANTS),yes)
  ALL_TARGETS += satl.haswell satl-cpu-level
endif
ifeq ($(HAVE_WINDOW),yes)
  ALL_TARGETS += satl-term
endif

all: $(ALL_TARGETS)
ifneq ($(HAVE_WINDOW),yes)
	@echo "note: satl-term not built -- no $(WINDOW_PKGS). The interpreter is unaffected."
endif

# $(LDFLAGS) BEFORE the objects, which is where a linker wants its options.
satl: $(SATL_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(SATL_OBJS)

# $(MARCH_HASWELL) ON THE LINK LINE TOO, and not only on the compiles. It
# changes nothing today -- the objects are already compiled and this build has
# no -flto -- and it is what keeps that true the day somebody adds link-time
# optimisation, where the linker becomes a compiler and would otherwise
# re-emit these objects against the baseline it was told nothing about.
satl.haswell: $(SATL_HASWELL_OBJS)
	$(CXX) $(CXXFLAGS) $(MARCH_HASWELL) $(LDFLAGS) -o $@ $(SATL_HASWELL_OBJS)

# NO -march, deliberately, and it is the one binary here for which that is a
# correctness requirement rather than a default. This is the program that runs
# before anything is known about the machine. See src/programs/cpu_level.cpp.
satl-cpu-level: $(CPU_LEVEL_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(CPU_LEVEL_OBJ)

# NO -march here either, and for a different reason than satl-cpu-level's: this
# binary interprets nothing, so there is no hot loop for an instruction set to
# act on. It is the fourth binary and not a fifth -- one build, and it gets the
# haswell interpreter for free by spawning whichever satl the installer chose.
#
# $(WINDOW_LIBS) AFTER the objects. pkg-config emits -l flags, and a linker
# resolves an -l against the undefined symbols it has accumulated SO FAR; put
# before the objects there are none yet, the libraries are dropped as unused,
# and the link dies on every gtk symbol in the file. This is the one rule in
# this build where the order of a variable on the line is a correctness
# requirement rather than a convention -- which is why $(LDFLAGS) still comes
# first, where a linker wants its options.
#
# A HARD ERROR when the libraries are missing, unlike `all`, which skips this
# target with a note. Somebody who typed `make satl-term` asked for this binary
# by name and is owed the reason it cannot be built.
#
# TWO WHOLE RULES AND NOT ONE RULE WITH A GUARD INSIDE IT, and the difference is
# the entire point. A conditional inside the recipe is part of the recipe, so it
# runs only when make has already decided to run one -- which means it is
# skipped when a satl-term from an earlier build is sitting there looking up to
# date, and, worse, it is reached only AFTER $(TERM_OBJS) have been built. On
# the machine this message exists for those objects cannot compile: the failure
# arrives as a gtk header that is not found, forty lines from anything naming
# the real cause, and the message written to prevent exactly that never prints.
#
# So the unbuildable case gets a rule with NO PREREQUISITES -- nothing is
# compiled before it speaks -- and it is .PHONY, so it speaks every time rather
# than declaring a file this make could not have produced to be up to date.
ifeq ($(HAVE_WINDOW),yes)

satl-term: $(TERM_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(TERM_OBJS) $(WINDOW_LIBS)

else

.PHONY: satl-term
satl-term:
	@echo "satl-term needs $(WINDOW_PKGS), which pkg-config cannot find." >&2
	@echo "  AlmaLinux/RHEL: dnf --enablerepo=crb install vte291-gtk4-devel" >&2
	@echo "  Debian/Ubuntu:  apt install libvte-2.91-gtk4-dev" >&2
	@echo "satl, satl.haswell and satl-cpu-level do not need it." >&2
	@false

endif

.PHONY: all clean
