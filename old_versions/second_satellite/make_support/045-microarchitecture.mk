# satellite -- two builds of satl, and the one that decides between them.
#
# AFTER 040-sources.mk, which names the sources both builds are made from, and
# BEFORE 050-build.mk, which needs MICROARCH_VARIANTS to know how many binaries
# `all` has to produce.
#
# WHAT THIS IS FOR. satl is built twice on x86-64: once assuming nothing beyond
# the x86-64 baseline, and once assuming the whole x86-64-v3 instruction set --
# the level Haswell introduced in 2013. install.sh runs satl-cpu-level, which
# asks the CPU which of those it is, and installs that one as $HOME/.satl/satl.
# The machine gets the fastest build it can actually execute, and neither build
# has to be a compromise for the other.

# x86-64-v3 AND NOT haswell, and the difference is the entire correctness
# argument for this file.
#
# -march=haswell licenses AES, RDRND, PCLMUL and INVPCID in addition to the v3
# set. Every real Haswell has them, so the flag is not wrong -- but the check
# on the other side, __builtin_cpu_supports("x86-64-v3") in
# src/programs/cpu_level.cpp, does not cover them. The set the compiler may emit
# from would then be a strict superset of the set that was verified, and that
# gap is where a SIGILL on somebody else's machine comes from. -march=x86-64-v3
# closes it by construction: the compiler is licensed to emit exactly what the
# detector checked for, no more.
#
# -mtune=haswell separately, because tuning is not licensing. It reorders and
# schedules for Haswell and emits no instruction that -march did not already
# allow, so it costs nothing on a newer machine -- a Zen 4 running this build
# runs correct code that was merely scheduled for a different pipeline -- and it
# is the right guess, since Haswell is the oldest machine that can run it.
MARCH_HASWELL = -march=x86-64-v3 -mtune=haswell

# ASK THE COMPILER WHAT IT TARGETS, not `uname -m` what this machine is. The two
# answers differ under every cross build, and the flags above go to the
# compiler, so the compiler is the one whose answer decides. -dumpmachine is
# understood by both gcc and clang and prints a triple like
# x86_64-unknown-linux-gnu.
#
# := and not =, because this is a $(shell) and a recursive variable would fork
# the compiler again at every single reference.
TARGET_TRIPLE := $(shell $(CXX) -dumpmachine 2>/dev/null)

# Two builds only where the levels exist. On aarch64 and ppc64le -- both of
# which Enterprise Linux ships -- -march=x86-64-v3 is not a flag the compiler
# will take, and there is nothing for a second build to be second to. Those
# machines build one satl and install.sh installs it, which is the path
# cpu_level.cpp's #else branch and 050-build.mk's conditional both describe.
#
# An empty TARGET_TRIPLE -- a compiler that does not understand -dumpmachine, or
# is not there at all -- lands in the `no` branch. One build is the answer that
# is never wrong; two builds on a machine we could not identify is a guess.
ifneq (,$(filter x86_64-% amd64-%,$(TARGET_TRIPLE)))
  MICROARCH_VARIANTS = yes
else
  MICROARCH_VARIANTS = no
endif

# .haswell.o rather than a second directory, so that the pattern rules in
# 060-compile.mk stay one line each and `clean` keeps naming what it removes.
SATL_HASWELL_OBJS = $(SATL_SRCS:.cpp=.haswell.o)

# THE DETECTOR IS BUILT AT THE BASELINE and appears in no variant list above.
# It is the program that runs before anything is known about the machine, so it
# is the one program that may not assume anything about it. See the note at the
# top of src/programs/cpu_level.cpp.
CPU_LEVEL_SRC = $(PROGRAMS)/cpu_level.cpp
CPU_LEVEL_OBJ = $(CPU_LEVEL_SRC:.cpp=.o)
