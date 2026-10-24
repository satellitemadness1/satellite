# satellite 004 -- which compiler, and the flags.
#
# THE CLANG THIS PROJECT IS DEVELOPED AGAINST, BUT ONLY IF IT IS THERE (003's
# rule). clang-current is the symlink each rebuilt toolchain is repointed at, so it
# cannot go stale the way a version directory would. `origin default` means nobody
# chose: `make CXX=g++` and CXX from the environment both win. 004's old Makefile
# said `CXX ?= g++`, which never fires -- CXX is one of make's built-ins.
LLVM_BIN = $(HOME)/opt/clang-current/bin
#
# THEN THE SYSTEM'S clang++, THEN g++ (a fresh clone, 2026-09-23). A machine without
# ~/opt built with c++ -- g++ -- even with clang installed. AlmaLinux 10's own clang 21
# (`sudo dnf install clang`) was measured that day: a fresh clone, a bare environment,
# 0 errors, 0 warnings, and the satl it made runs. g++ 14.3 builds it too, and stays
# last, because the project is developed against clang.
ifeq ($(origin CXX),default)
  CXX := $(firstword $(wildcard $(LLVM_BIN)/clang++) $(if $(shell command -v clang++ 2>/dev/null),clang++) c++)
endif

# AND THE C COMPILER, chosen the same way and for one file only: the GResource
# that glib-compile-resources writes for the window's carried data (WIN-1). Left
# at make's built-in `cc` it would be the system gcc while everything else is
# this clang, which is the mismatch PLAN M0.5 wrote build_libraries.py's .built_with
# file to stop happening quietly (that script went on 2026-10-07: the words are inside satl).
ifeq ($(origin CC),default)
  CC := $(firstword $(wildcard $(LLVM_BIN)/clang) $(if $(shell command -v clang 2>/dev/null),clang) cc)
endif

# OPT IS THE KNOB, AND CXXFLAGS IS NOT ONE: `make CXXFLAGS=-O3` replaces the whole
# variable and would take -std=c++20 with it. -O2, not 003's -O3, because every
# measurement in PROGRESS.md and DESIGN §12 was taken at -O2; moving it is a race,
# not a port. The order of the words is the old Makefile's, so the build
# fingerprint (020-version.mk) describes the same compiler command it did.
OPT ?= -O2

# ONE PROCESSOR'S BUILD (the author, 2026-09-23: "we could technically just build all
# targets, and alter satl-cpu-level to check for them"). `make CPU=haswell` builds satl
# with -march=haswell into build/cpu/haswell/ (030-directories.mk),
# installs nothing, and never raises the build number -- 055-cpus.mk says why, and
# builds every processor at once. Empty, the ordinary build: no -march, the x86-64
# baseline every processor runs.
#
# -march GOES LAST, AND ONLY WHEN THERE IS ONE, so the ordinary build's flags are the
# same string they always were: 020-version.mk fingerprints them, and a stray space
# would have raised the build number for nothing.
#
# FROM THE COMMAND LINE ONLY: an exported CPU in a shell would turn every make into a
# processor's build that installs nothing, silently (the review, 2026-09-23). And ONE
# name, with no / or .. in it -- `make clean CPU=...` removes build/cpu/$(CPU).
ifeq ($(origin CPU),environment)
override CPU :=
endif
CPU ?=
ifneq ($(CPU),)
ifneq ($(words $(CPU))$(findstring /,$(CPU))$(findstring ..,$(CPU)),1)
$(error CPU=$(CPU) is not one processor's name -- make_support/055-cpus.mk lists them)
endif
endif

# THE C++ LIBRARY: LLVM's libc++ WHEN THE COMPILER IS clang, AND INSIDE satl (the author,
# 2026-10-22: "port it to libc++"). libc++, libc++abi and the unwinder are linked STATICALLY,
# so satl asks the machine it runs on for no C++ library at all -- libc++ is installed on almost
# none of them -- and its floor stays glibc 2.38, EL10's (050-build.mk's ALLOWED_NEEDED, and
# check.sh's floor row, which asks this build's record which library it was). g++ KEEPS
# libstdc++, dynamic, exactly as before: GCC has no libc++ path here.
#
# SAID OUTRIGHT, NEVER LEFT TO A clang++.cfg: ~/opt/clang-current has none, a root-owned
# /opt/clang-current's says -stdlib=libc++, and the two must build the same satl.
#
# EACH FLAG, AND WHY (measured 2026-10-22 with clang 23.1.3, under GNU ld and lld alike):
#   -stdlib=libc++     libc++'s headers to compile against, libc++ to link.
#   -static-libstdc++  the driver's own spelling of a static C++ library: -Bstatic -lc++ -Bdynamic,
#                      after the objects.
#   libc++abi, WHOLE   libc++.a holds no __cxa_* (LLVM builds the ABI library apart: `nm libc++.a`
#                      defines no __cxa_throw), and the driver names none -- -static-libstdc++ alone
#                      stops at `undefined reference to __cxa_pure_virtual`. --whole-archive,
#                      because these flags ride BEFORE the objects (CXXFLAGS, below) and GNU ld
#                      takes from an archive only what it has already been asked for: a plain
#                      -l:libc++abi.a there takes nothing. lld would not care. The whole archive
#                      cost 5 KB over the members a link takes.
#   -static-libgcc     the unwinder, static: libgcc_eh.a, from the gcc whose crt files every clang
#                      on Linux links with. NOT LLVM's libunwind: clang refuses --unwindlib=libunwind
#                      beside libgcc's builtins ("--rtlib=libgcc requires --unwindlib=libgcc"), so it
#                      would take --rtlib=compiler-rt too -- a second runtime swapped for the same
#                      NEEDED list. A cfg that says --unwindlib=libunwind gets -l:libunwind.a from
#                      this same flag.
#   --start-no-unused-arguments ... --end-no-unused-arguments
#                      the link-only three are in CXXFLAGS, which every link here passes (satl,
#                      the training build, every harness, satl-cpu-level), so a harness added
#                      tomorrow links the same way without anybody remembering; this tells a
#                      compile not to warn that it had no use for them.
#
# ONLY WHERE THE ARCHIVES ARE: a clang that finds no libc++.a or libc++abi.a -- beside it, or in
# the folders it searches -- builds with libstdc++, as g++ does, and says so once --
# 045-optimise.mk's rule for its tools. Asked of the
# driver (-print-file-name answers a bare name when it finds nothing), once a make.
CXX_RUNTIME = libstdc++
CXX_RUNTIME_FLAGS =
ifneq ($(findstring clang version,$(shell $(CXX) --version 2>/dev/null | head -n 1)),)
  LIBCXX_ARCHIVES := $(filter /%,$(foreach a,libc++.a libc++abi.a,$(shell $(CXX) -stdlib=libc++ -print-file-name=$(a) 2>/dev/null)))
  ifeq ($(words $(LIBCXX_ARCHIVES)),2)
    CXX_RUNTIME = libc++
    CXX_RUNTIME_FLAGS = -stdlib=libc++ --start-no-unused-arguments -static-libstdc++ -static-libgcc \
                        -Wl,--push-state,--whole-archive,-l:libc++abi.a,--pop-state --end-no-unused-arguments
  else
    CXX_RUNTIME_FLAGS = -stdlib=libstdc++
    ifeq ($(MAKELEVEL),0)
      $(info note: $(CXX) has no libc++.a and libc++abi.a beside it -- satl is built with libstdc++, as g++ builds it)
    endif
  endif
endif

# STILL C++20 (2026-10-22): libc++ gives resize_and_overwrite to C++23 alone, so under
# libc++ the strings take string_overwrite.hpp's plain resize -- correct, only slower.
# C++23 waits: in the libc++ trial every C++23 build, libstdc++'s too, failed the pty
# prompt row (5 of 5) where every C++20 one passed (4 of 4), measured under a gcc build's
# load. Move once that row passes on an idle machine.
CXXFLAGS = -std=c++20$(if $(CXX_RUNTIME_FLAGS), $(CXX_RUNTIME_FLAGS)) $(OPT) -Wall -Wextra$(if $(CPU), -march=$(CPU))

# Set nowhere here, so a distribution's link hardening -- or this machine's
# -fuse-ld=lld, which the environment exports -- reaches every link.
LDFLAGS ?=
