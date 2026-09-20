# satellite 004 -- which compiler, and the flags.
#
# THE CLANG THIS PROJECT IS DEVELOPED AGAINST, BUT ONLY IF IT IS THERE (003's
# rule). clang-current is the symlink each rebuilt toolchain is repointed at, so it
# cannot go stale the way a version directory would. `origin default` means nobody
# chose: `make CXX=g++` and CXX from the environment both win. 004's old Makefile
# said `CXX ?= g++`, which never fires -- CXX is one of make's built-ins.
LLVM_BIN = $(HOME)/opt/clang-current/bin
ifeq ($(origin CXX),default)
  CXX := $(if $(wildcard $(LLVM_BIN)/clang++),$(LLVM_BIN)/clang++,c++)
endif

# AND THE C COMPILER, chosen the same way and for one file only: the GResource
# that glib-compile-resources writes for the window's carried data (WIN-1). Left
# at make's built-in `cc` it would be the system gcc while everything else is
# this clang, which is the mismatch PLAN M0.5 wrote build_libraries.py's
# .built_with file to stop happening quietly.
ifeq ($(origin CC),default)
  CC := $(if $(wildcard $(LLVM_BIN)/clang),$(LLVM_BIN)/clang,cc)
endif

# OPT IS THE KNOB, AND CXXFLAGS IS NOT ONE: `make CXXFLAGS=-O3` replaces the whole
# variable and would take -std=c++20 with it. -O2, not 003's -O3, because every
# measurement in PROGRESS.md and DESIGN §12 was taken at -O2; moving it is a race,
# not a port. The order of the words is the old Makefile's, so the build
# fingerprint (020-version.mk) describes the same compiler command it did.
OPT ?= -O2
CXXFLAGS = -std=c++20 $(OPT) -Wall -Wextra

# Set nowhere here, so a distribution's link hardening -- or this machine's
# -fuse-ld=lld, which the environment exports -- reaches every link.
LDFLAGS ?=
