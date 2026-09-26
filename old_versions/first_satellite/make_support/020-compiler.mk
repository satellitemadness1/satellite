# satellite -- which compiler, and the one flag knob.
#
# BEFORE 030-version.mk, and that order is load-bearing: VERSION_DEFS bakes
# $(CXX) and $(CXXFLAGS) into the binary as strings, so both have to be settled
# before it is written.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# The clang 24 trunk build this project is developed against, but ONLY if it is
# actually there. It lives under $HOME, and $HOME is not a constant: `sudo make`
# runs with HOME=/root, so a hard-coded $(HOME)/opt/... resolves to
# /root/opt/clang-24/bin/clang++ and the build dies with "No such file or
# directory" on a machine where the compiler is sitting in plain sight. The
# same absence is the normal case on any build machine, which is why
# debian/rules already overrides CXX by hand.
#
# The path moved from $(HOME)/.local/llvm/bin, which is where this line was
# written and where nothing has been for some time -- so the wildcard below
# quietly missed and every build since fell through to `c++`, which on this
# machine is GCC 17. That is not a failure mode anybody sees: the fallback
# compiles the tree perfectly well, so the only symptom was that the compiler
# named in DESIGN's measurements was not the compiler doing the measuring.
#
# It moved a SECOND time on 2026-08-24, from clang-24 to clang-24-2, and that
# move is not cosmetic even though both are the same source revision (git
# 3c2eaf39, verified by --version on each). They are the same COMPILER and two
# different INSTALLS: clang-24 was built without compiler-rt and clang-24-2
# ships the whole set. `-print-runtime-dir` answers "(runtime dir is not
# present)" for the first and a real path for the second. Repointing this one
# line is therefore also what moves TSAN_CXX onto $(CXX), which is the
# arrangement the note below has always said it wanted and could not have.
#
# origin, rather than ?=, because CXX is one of make's built-in variables and is
# therefore already set: ?= would never fire. `default` means nobody has chosen,
# so `make CXX=g++` and CXX from the environment both still win.
LLVM_BIN = $(HOME)/opt/clang-24-2/bin
ifeq ($(origin CXX),default)
  CXX := $(if $(wildcard $(LLVM_BIN)/clang++),$(LLVM_BIN)/clang++,c++)
endif

# The GUI flags are DELIBERATELY not in CXXFLAGS. Linking the interpreter
# against gtk4 and vte pulls 119 shared objects that the dynamic linker loads
# before main() on every run, and `satl --run` touches none of them:
# 25.9 ms with the link, 2.5 ms without, against a 2.2 ms bare-process floor.
# Only window.o gets them, and only satl-term links them.
#
# OPT is the knob, and CXXFLAGS is not one. A command-line `make CXXFLAGS=-O3`
# REPLACES this variable whole -- that is what a command-line assignment means
# in make -- so it takes -std=c++20 with it, and the build dies on
# std::atomic<std::shared_ptr<T>> being a C++20 feature, twenty lines of
# static_assert away from anything that names the real cause. Asking someone to
# restate three flags to change one is a trap with a note beside it, so the one
# flag anybody actually wants to change gets its own variable.
#
# `make OPT=-O2` and `make OPT="-O3 -march=native"` therefore work and keep the
# three flags the build cannot compile without. The DEFAULT moved from -O2 to
# -O3 on 2026-08-24, at the maintainer's request; -O2 is one word away and
# every packaging path already names its own level, so nothing downstream is
# committed by the choice. -march=native is deliberately
# not a default: it bakes in this machine's instruction set, and debian/rules
# drives this same Makefile to build a package that has to run on machines that
# are not this one.
OPT ?= -O3
CXXFLAGS = -std=c++20 -Wall -Wextra $(OPT)
