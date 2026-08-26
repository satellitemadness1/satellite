# satellite -- which compiler, and the one flag knob.
#
# BEFORE 020-version.mk, and that order is load-bearing: VERSION_DEFS bakes
# $(CXX) and $(CXXFLAGS) into the binary as strings, so both have to be settled
# before it is written.

# The clang trunk build this project is developed against, but ONLY if it is
# actually there. It lives under $HOME, and $HOME is not a constant: `sudo make`
# runs with HOME=/root, so a hard-coded path resolves to /root/opt/... and the
# build dies on a machine where the compiler is sitting in plain sight. The same
# absence is the normal case on any other build machine.
#
# The first satellite was bitten by the silent half of this: LLVM_BIN pointed at
# a directory that no longer existed, the wildcard quietly missed, `c++` -- GCC
# -- answered instead, and every figure attributed to clang was GCC's with
# nothing anywhere saying so. That is why version.hpp prints BOTH the path make
# invoked and the __VERSION__ the compiler reported.
#
# clang-current, and NOT clang-24-2 or any other version directory, for exactly
# that reason. Each rebuilt toolchain is installed to its own prefix and
# ~/opt/clang-current is repointed at it, so naming a version here would go
# stale the first time one of those directories is replaced -- and going stale
# is the failure above, silent and attributed to the wrong compiler. A symlink
# cannot drift out of date, because repointing it IS the install step.
#
# origin, rather than ?=, because CXX is one of make's built-in variables and is
# therefore already set: ?= would never fire. `default` means nobody has chosen,
# so `make CXX=g++` and CXX from the environment both still win.
LLVM_BIN = $(HOME)/opt/clang-current/bin
ifeq ($(origin CXX),default)
  CXX := $(if $(wildcard $(LLVM_BIN)/clang++),$(LLVM_BIN)/clang++,c++)
endif

# OPT is the knob, and CXXFLAGS is not one. A command-line `make CXXFLAGS=-O3`
# REPLACES this variable whole -- that is what a command-line assignment means
# in make -- so it would take -std=c++20 with it and the build would die on a
# C++20 feature, twenty lines of diagnostic away from anything naming the real
# cause. Asking someone to restate three flags in order to change one is a trap
# with a note beside it, so the one flag anybody actually wants to change gets
# its own variable.
#
# -march=native is deliberately NOT a default: it bakes in this machine's
# instruction set, and this same Makefile has to be able to build something that
# runs on machines that are not this one.
OPT ?= -O3
CXXFLAGS = -std=c++20 -Wall -Wextra $(OPT)

# Set nowhere in this build on purpose, so that a distribution's link-time
# hardening -- -Wl,-z,relro,-z,now and whatever the next one adds -- arrives
# from the environment or the command line and reaches the link rule. A Makefile
# that never mentions the variable is a Makefile those flags cannot reach.
LDFLAGS ?=
