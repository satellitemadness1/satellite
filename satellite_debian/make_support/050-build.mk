# satellite -- the default goal and the four binaries.
#
# THE PARENT'S 050-build.mk, WITH build/ IN FRONT OF EVERY NAME and apt in place
# of dnf in the one message that names packages.
#
# AFTER 030-output.mk and 040-static.mk, and that ordering is a HARD requirement
# rather than a convention: this fragment tests MICROARCH_VARIANTS and
# HAVE_WINDOW with an ifeq, which make evaluates as it READS rather than
# afterwards, so both must already be set. They come from the parent's 045 and
# 047 by way of 020-inherited.mk.
#
# `all` IS NOT WHAT MAKES `all` THE DEFAULT GOAL HERE. 010-tree.mk sets
# .DEFAULT_GOAL in the first fragment and explains why this build cannot rely on
# the parent's arrangement: its stamps have a slash in their names.

# FOUR BINARIES ON AN x86-64 DESKTOP AND ONE ON A MACHINE WITH NEITHER gtk4 NOR
# x86-64, which is the parent's table unchanged. satl.haswell and satl are the
# same sources compiled against two different instruction sets, satl-cpu-level
# is the program that finds out which of them a machine can execute, and
# satl-term is conditional on its libraries rather than on the instruction set.
#
# The variants are built by `all` rather than only by an installer, because a
# build that only happens during an install is a build nobody sees fail until
# they are installing.
ALL_TARGETS = $(SATL)
ifeq ($(MICROARCH_VARIANTS),yes)
  ALL_TARGETS += $(SATL_HASWELL) $(CPU_LEVEL)
endif
ifeq ($(HAVE_WINDOW),yes)
  ALL_TARGETS += $(SATL_TERM)
endif

# $(RANDOM_OBJS) is an object and not a binary, and it is in `all` so that the
# module cannot rot unnoticed. ../make_support/040-sources.mk says at length why
# it links into nothing: the thing that would call it is satellite.random.*,
# which reaches no milestone, and compiling it under `all` is the cheapest thing
# that stops a header change or a compiler upgrade breaking it silently.
all: $(ALL_TARGETS) $(RANDOM_OBJS)
ifneq ($(HAVE_WINDOW),yes)
	@echo "note: satl-term not built -- no $(WINDOW_PKGS). The interpreter is unaffected."
	@echo "      sudo apt install libvte-2.91-gtk4-dev"
endif

# THE DIRECTORY IS AN ORDER-ONLY PREREQUISITE, and the bar is what makes this
# correct rather than merely working. A directory's timestamp changes every time
# a file is written into it, so an ordinary prerequisite would make every binary
# out of date the moment the next one was linked beside it, and `make` twice in
# a row would relink all four the second time. `|` says this must EXIST before
# the target is made and its age is not evidence about anything.
#
# ONE RULE FOR build/ AND ANOTHER FOR THE OBJECT DIRECTORIES IN 060, rather than
# one list here, because the two are needed at different moments and by
# different rules. `mkdir -p` is idempotent and race-safe, which is what makes it
# safe for both to create the same parent under `make -j`.
$(ALL_TARGETS): | $(BUILD)

$(BUILD):
	mkdir -p $@

# THE THREE THAT ALWAYS EXIST DEPEND ON THE LINK STAMP, so that changing STATIC
# relinks them the way changing CXXFLAGS recompiles the objects. 040-static.mk
# writes it and says why it is one file rather than two.
#
# satl-term IS NOT ON THIS LINE and is given the same prerequisite inside the
# window branch below instead. Naming it here would reach BOTH branches, and in
# the branch where the window cannot be built the whole point of that rule is
# that it has no prerequisites and therefore speaks before anything is made.
# A stamp is not much to build first, but "no prerequisites" is either true or
# it is a comment describing a rule that no longer works that way.
$(SATL) $(SATL_HASWELL) $(CPU_LEVEL): $(LDFLAGS_STAMP)

# $(LDFLAGS) BEFORE the objects, which is where a linker wants its options.
$(SATL): $(SATL_OBJS)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(STATIC_LDFLAGS) -o $@ $(SATL_OBJS)

# $(MARCH_HASWELL) ON THE LINK LINE TOO, and not only on the compiles. It
# changes nothing today -- the objects are already compiled and this build has
# no -flto -- and it is what keeps that true the day somebody adds link-time
# optimisation, where the linker becomes a compiler and would otherwise re-emit
# these objects against the baseline it was told nothing about.
$(SATL_HASWELL): $(SATL_HASWELL_OBJS)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(MARCH_HASWELL) $(LDFLAGS) $(STATIC_LDFLAGS) \
	    -o $@ $(SATL_HASWELL_OBJS)

# NO -march, deliberately, and it is the one binary here for which that is a
# correctness requirement rather than a default. This is the program that runs
# before anything is known about the machine, so it is the one program that may
# not assume anything about it. See ../src/programs/cpu_level.cpp.
$(CPU_LEVEL): $(CPU_LEVEL_OBJ)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(STATIC_LDFLAGS) -o $@ $(CPU_LEVEL_OBJ)

# NO -march here either, and for a different reason than satl-cpu-level's: this
# binary interprets nothing, so there is no hot loop for an instruction set to
# act on. It is the fourth binary and not a fifth -- one build, and it gets the
# haswell interpreter for free by spawning whichever satl sits beside it.
#
# BESIDE IT IS THE WHOLE POINT, and it is why these four may live in build/ at
# all. ../src/programs/terminal.cpp reads /proc/self/exe and appends `satl` to
# its directory, and ../src/programs/window_handover.cpp does the mirror image
# with `satl-term` -- both deliberately, both refusing to trust argv[0] or PATH,
# so that a satl-term from one install cannot spawn another install's
# interpreter. Four binaries in one directory is exactly the arrangement they
# were written for, and build/ is a directory like any other.
#
# $(WINDOW_LIBS) AFTER the objects. pkg-config emits -l flags, and a linker
# resolves an -l against the undefined symbols it has accumulated SO FAR; put
# before the objects there are none yet, the libraries are dropped as unused,
# and the link dies on every gtk symbol in the file. This is the one rule in
# this build where the order of a variable on the line is a correctness
# requirement rather than a convention -- which is why $(LDFLAGS) still comes
# first, where a linker wants its options.
#
# TWO WHOLE RULES AND NOT ONE RULE WITH A GUARD INSIDE IT. A conditional inside
# the recipe is part of the recipe, so it runs only when make has already
# decided to run one -- which means it is skipped when a satl-term from an
# earlier build is sitting there looking up to date, and, worse, it is reached
# only AFTER $(TERM_OBJS) have been built. On the machine this message exists
# for those objects cannot compile: the failure arrives as a gtk header that is
# not found, forty lines from anything naming the real cause, and the message
# written to prevent exactly that never prints.
#
# So the unbuildable case gets a rule with NO PREREQUISITES -- nothing is
# compiled before it speaks -- and it is .PHONY, so it speaks every time rather
# than declaring a file this make could not have produced to be up to date.
ifeq ($(HAVE_WINDOW),yes)

# $(TERM_STATIC_LDFLAGS) AND NOT $(STATIC_LDFLAGS), which is the one place the
# two differ: under STATIC=full the other three take -static and this one may
# not, because gtk and vte dlopen their own modules and a static binary cannot.
# 040-static.mk carries the argument.
$(SATL_TERM): $(TERM_OBJS) $(LDFLAGS_STAMP)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(TERM_STATIC_LDFLAGS) \
	    -o $@ $(TERM_OBJS) $(WINDOW_LIBS)

else

# A HARD ERROR when the libraries are missing, unlike `all`, which skips this
# target with a note. Somebody who typed `make satl-term` asked for this binary
# by name and is owed the reason it cannot be built -- and, on this family, the
# release that is the real answer. vte gained its GTK4 build at 0.70; Ubuntu
# 22.04 ships 0.68 and has no such package to install.
.PHONY: $(SATL_TERM)
$(SATL_TERM):
	@echo "satl-term needs $(WINDOW_PKGS), which pkg-config cannot find." >&2
	@echo "  Debian 13 and Ubuntu 24.04+:  sudo apt install libvte-2.91-gtk4-dev" >&2
	@echo "  Ubuntu 22.04 ships vte 0.68, which has no GTK4 build at all." >&2
	@echo "satl, satl.haswell and satl-cpu-level do not need it." >&2
	@false

endif

# THE FOUR SHORT NAMES, so that `make satl-term` works from this directory the
# way it does from the parent. Without these a person would have to type
# `make build/satl-term`, having read a Makefile to find out that they must --
# and the message above, which exists to be reached by `make satl-term`, would
# be reached by nobody.
#
# ALIASES AND NOT SECOND RULES: each is a phony name with the real target as its
# only prerequisite and no recipe of its own, so there is still exactly one rule
# that knows how to link each binary.
.PHONY: satl satl.haswell satl-cpu-level satl-term
satl:           $(SATL)
satl.haswell:   $(SATL_HASWELL)
satl-cpu-level: $(CPU_LEVEL)
satl-term:      $(SATL_TERM)

.PHONY: all clean
