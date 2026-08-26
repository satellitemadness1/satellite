# satellite -- the default goal and the binaries.
#
# THE FIRST FRAGMENT THAT DECLARES A TARGET, which is what makes `all` the first
# target make sees. .DEFAULT_GOAL below says it outright anyway, because after a
# split like this one the order of the rules is the order of the includes, and a
# rule that lands in the wrong file should not be able to change what `make`
# does.

.DEFAULT_GOAL := all

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

all: $(ALL_TARGETS)

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

.PHONY: all clean
