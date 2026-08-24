PKGS     = gtk4 vte-2.91-gtk4

# pkg-config, and whether this machine has what satl-term needs.
#
# `make` used to die here, and the way it died is the point: PKGS is only ever
# needed by ONE object, window.o, but `all` asked for satl-term unconditionally,
# so a machine without the vte development package got pkg-config's complaint
# followed by `fatal error: gtk/gtk.h: No such file or directory` -- and no
# satl either, because make stops at the first failed recipe. The interpreter
# needs neither library (see the note on CXXFLAGS below) and there was no reason
# for it to go down with the window.
#
# So the packages are LOOKED FOR rather than assumed. Present, `make` builds
# both, exactly as before. Absent, it builds the interpreter, says which package
# is missing and how to install it, and exits 0 -- and `make satl-term` still
# fails, loudly and with the same instruction, because someone who asked for the
# window by name wants to know why they did not get it.
#
# --exists rather than --cflags: it answers yes or no and prints nothing either
# way, so this is the one place the question is asked and the noise stops here.
# := so it is asked once per make rather than once per reference.
PKG_CONFIG ?= pkg-config
MISSING_PKGS := $(strip $(foreach p,$(PKGS),     $(if $(shell $(PKG_CONFIG) --exists $(p) 2>/dev/null && echo yes),,$(p))))

# What to type to fix it, in the words of whichever package manager is here.
# A message that says "install the vte development package" and leaves the
# reader to find out what it is called on their distribution is half a message.
ifneq ($(shell command -v dnf 2>/dev/null),)
  # The vte gtk4 bindings live in CodeReady Builder on RHEL and its rebuilds,
  # which is disabled by default -- so the repository has to be named or dnf
  # reports "No match for argument" on a package that is sitting right there.
  DEPS_CMD = sudo dnf install --enablerepo=crb gtk4-devel vte291-gtk4-devel
else ifneq ($(shell command -v apt-get 2>/dev/null),)
  # The same names debian/control Build-Depends on, so the two cannot drift.
  DEPS_CMD = sudo apt-get install libgtk-4-dev libvte-2.91-gtk4-dev pkg-config
else ifneq ($(shell command -v pacman 2>/dev/null),)
  DEPS_CMD = sudo pacman -S --needed gtk4 vte4
else
  DEPS_CMD =
endif

# The message itself, once, because three recipes print it.
ifeq ($(DEPS_CMD),)
  DEPS_ADVICE = install the gtk4 and vte-2.91-gtk4 development packages
else
  DEPS_ADVICE = run:  make deps    (which is: $(DEPS_CMD))
endif

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

# TWO NUMBERS, moving at different rates. VERSION is the language and changes
# rarely; REVISION is this build of it and goes up as work lands. Overridable,
# so a packaging script can stamp its own without editing this file.
SATELLITE_VERSION  ?= 002
SATELLITE_REVISION ?= 01

# When this build happened. SOURCE_DATE_EPOCH WINS WHENEVER IT IS SET, and that
# is not a nicety: dpkg exports it precisely so that two builds of identical
# source produce identical binaries, and a stamp that read the wall clock
# instead would fail every reproducibility check Debian runs -- on packages
# this repo actually ships, and marked Architecture: any, so on every port.
# `date -u` because a timestamp without a zone is a timestamp that means
# something different on each machine that reads it.
BUILD_STAMP := $(shell date -u $(if $(SOURCE_DATE_EPOCH),-d @$(SOURCE_DATE_EPOCH),)                    '+%Y-%m-%d %H:%M:%S UTC' 2>/dev/null || echo unrecorded)

# On TWO recipes and never in CXXFLAGS. CXXFLAGS is what .cxxflags-stamp
# records, and BUILD_STAMP changes every second -- so putting these there would
# rewrite the stamp on every invocation and rebuild all forty-three objects,
# every time, forever, permanently silencing the one check that exists to catch
# a real flag change. This is the same reasoning that keeps -DSATELLITE_LIB_DIR
# on the system.o recipe alone.
#
# $(CXX) is recorded as the path make INVOKED. What that path turned out to be
# is a separate fact and version.hpp reads it from __VERSION__, because this
# project has already been bitten once by the two disagreeing: LLVM_BIN pointed
# at a directory that did not exist, `c++` answered instead, and every figure
# attributed to clang was GCC's with nothing anywhere saying so.
# The make that drove this build, first line of its --version. It joins
# VERSION_DEFS rather than CXXFLAGS for the reason version.hpp:12 gives about
# the build stamp: a define in CXXFLAGS rewrites .cxxflags-stamp, and a value
# that can change would rebuild all 43 objects on every make, forever. Here it
# reaches exactly the three recipes that need it.
#
# := so the sub-shell runs once per make and not once per recipe, and the
# stderr redirect so a make that has no --version leaves the fallback in
# system.cpp to answer `unrecorded` rather than putting an error message in a
# string a program can read.
SATELLITE_MAKE_VERSION := $(shell $(MAKE) --version 2>/dev/null | head -1)

VERSION_DEFS = -DSATELLITE_BUILD_MAKE='"$(SATELLITE_MAKE_VERSION)"' \
               -DSATELLITE_VERSION='"$(SATELLITE_VERSION)"' \
               -DSATELLITE_REVISION='"$(SATELLITE_REVISION)"' \
               -DSATELLITE_BUILT='"$(BUILD_STAMP)"' \
               -DSATELLITE_BUILD_CXX='"$(CXX)"' \
               -DSATELLITE_BUILD_FLAGS='"$(CXXFLAGS)"'
# Recursively expanded, so pkg-config is run only by the recipes that use them
# -- window.o and the satl-term link -- and never by a `make satl` or a
# `make test`. Errors are dropped because MISSING_PKGS above has already asked
# the question properly and reported it; a second complaint from the same
# missing package, in pkg-config's words, at the top of an otherwise fine build,
# is the noise this replaces.
GTKFLAGS = $(shell $(PKG_CONFIG) --cflags $(PKGS) 2>/dev/null)
LDLIBS   = $(shell $(PKG_CONFIG) --libs $(PKGS) 2>/dev/null)

# pcg-cpp is header-only and vendored in the tree, so it adds nothing to the
# link -- `ldd satl` lists the same six shared objects it did before §18.
#
# -isystem, not -I, and that is not a style preference: pcg_extras.hpp:223
# raises -Wunused-but-set-parameter under this file's own -Wall -Wextra, and §9
# makes a silent from-scratch rebuild a property the build has to keep. A
# warning from a vendored header is one nobody here can fix without forking it.
PCGFLAGS = -isystem pcg-cpp-0.98/include

# LDFLAGS is set nowhere in this file on purpose, so that a distribution's
# link-time hardening -- -Wl,-z,relro,-z,now from dpkg-buildflags, and whatever
# the next one adds -- arrives from the environment or the command line and
# reaches both link rules below. A Makefile that never mentions the variable is
# a Makefile those flags cannot reach.
LDFLAGS ?=

# ThreadSanitizer is the one part of the suite that is not portable: it is a
# 64-bit-only runtime, and Debian builds libtsan2 for amd64, arm64, mips64el,
# ppc64el, riscv64 and s390x and for nothing else, so on i386 or armhf
# -fsanitize=thread does not compile at all. TSAN=0 drops that one binary and
# still runs the other ten, which is what a package build on such an
# architecture needs: both binary packages are Architecture: any, and that is a
# promise the package builds everywhere (Debian Policy 5.6.8).
TSAN      ?= 1
TSAN_TEST  = $(if $(filter-out 0,$(TSAN)),$(TESTS)/library_test/library_test_tsan)

# ThreadSanitizer needs a runtime the COMPILER supplies, and $(CXX) is not
# guaranteed to have one. The clang-24 build -- which $(LLVM_BIN) named until
# 2026-08-24 -- is built WITHOUT compiler-rt: `clang++ -print-runtime-dir`
# answers "(runtime dir is not present)" and the link dies on a missing
# libclang_rt.tsan.a, which took the whole of `make test` down with it. That is
# a property of that build of clang and not of this machine -- /usr/bin/clang++
# (21.1.8) ships the runtime, and so does g++ as libtsan.
#
# $(LLVM_BIN) now names clang-24-2, which DOES ship it, so on this machine the
# fallback no longer fires and TSAN_CXX resolves to $(CXX). None of the
# machinery below changes, and it must not: it is not scaffolding for one
# broken install, it is what keeps `make test` alive on any machine whose
# compiler cannot supply the runtime -- which this one could not until today,
# and a packaging machine still may not be able to.
#
# So the sanitized binary gets its own compiler. The default asks $(CXX)
# whether it can supply either runtime and uses it if it can; otherwise it
# falls back, clang first because the instrumentation and the runtime then come
# from the same project. Every source in this binary is compiled by the SAME
# compiler as the runtime it links, which is the one thing that must not be
# mixed.
#
# Recursively expanded and referenced only in the library_test_tsan recipe, so
# the two `-print-file-name` subprocesses run when that binary is built and
# never on a `make satl`. `origin` rather than ?= so a command-line or
# environment TSAN_CXX still wins.
#
# The test is a LINK and not a file lookup, and the first attempt at this got it
# wrong in a way worth recording: asking clang for -print-file-name=libtsan.so
# answers /usr/lib/gcc/x86_64-redhat-linux/14/libtsan.so, because clang searches
# GCC's install directories -- a 38-byte linker script for a runtime clang would
# never link against, since clang's -fsanitize=thread wants libclang_rt.tsan.a.
# The lookup therefore said yes for the one compiler that cannot do it. Linking
# an empty main is the only probe that answers the question actually being asked.
#
# **Verified** on this machine on 2026-08-24, by running the probe below by hand
# against all four candidates: yes for $(LLVM_BIN)/clang++ (clang-24-2), NO for
# the old clang-24 build, yes for /usr/bin/clang++ and yes for c++. So TSAN_CXX
# resolves to $(CXX) after the move and resolved to /usr/bin/clang++ before it,
# and the one thing that must not be mixed -- instrumentation and runtime from
# one project -- now holds without the fallback having to arrange it.
#
# The binary the /usr/bin/clang++ fallback built PASSED: 160,000 increments
# intact, 403,921 lock-free reads, 165 variables in satellite.library.
ifeq ($(origin TSAN_CXX),undefined)
  TSAN_OK   = $(shell printf 'int main(){}' | $(1) -fsanitize=thread -x c++ - \
                          -o /dev/null >/dev/null 2>&1 && echo ok)
  TSAN_CXX  = $(strip $(if $(call TSAN_OK,$(CXX)),$(CXX),\
                  $(if $(call TSAN_OK,/usr/bin/clang++),/usr/bin/clang++,c++)))
endif

# `prefix` is baked into the binary; DESTDIR is staging and is baked into
# NOTHING. The distinction is the whole contract with a packaging system: a
# .deb is built with prefix=/usr into a DESTDIR chroot, and a binary that had
# learned the chroot's name would look for its library there on the user's
# machine, where that directory does not exist.
#
# The DEFAULT follows who is running make, for the reason install.sh already
# gives at length (install.sh:40-58) and now for a second one: `all` below ends
# in an install, and an install a normal user cannot perform is not a default,
# it is an error message. /usr/local is the right place for an install from
# source and it needs root; $HOME/.local needs none, is already on PATH, and is
# in the XDG data search path on any current desktop.
#
# id -u rather than $(HOME) alone: `sudo make` runs with HOME=/root, and a
# default of $(HOME)/.local would drop a system build into /root/.local, which
# is on nobody's PATH -- the same trap the note on LLVM_BIN above is about. A
# HOME that is unset entirely falls back to /usr/local, where the writability
# check in `all` refuses the install rather than inventing /.local.
#
# ONE prefix per invocation, and that is not a style point. prefix is compiled
# into system.o through .libdir-stamp below, so a build at one prefix followed
# by an install at another recompiles system.o and relinks satl -- in BOTH
# directions, on every invocation, so `make` never reaches a fixed point. That
# is why the auto-install below overrides nothing: it installs at the prefix
# this build already used.
SATELLITE_UID := $(shell id -u 2>/dev/null)
ifeq ($(SATELLITE_UID),0)
  DEFAULT_PREFIX = /usr/local
else ifeq ($(strip $(HOME)),)
  DEFAULT_PREFIX = /usr/local
else
  DEFAULT_PREFIX = $(HOME)/.local
endif

prefix  ?= $(DEFAULT_PREFIX)
DESTDIR ?=
bindir   = $(prefix)/bin
datadir  = $(prefix)/share
mandir   = $(datadir)/man
docdir   = $(datadir)/doc/satellite

# Every module lives in its own directory under src/, named for the job the
# module does rather than for the abbreviation its files still use. The FILE
# names are deliberately unchanged -- ast.cpp is still ast.cpp -- so the
# hundreds of references in DESIGN.md that name a file still name the right
# file, and only the directory in front of it is new.
#
# These are variables rather than spelled-out paths so that the explicit source
# lists below stay one file per name and readable at a glance, which is the
# property the note on ENV_SRCS is about.
SRC      = src
AST      = $(SRC)/abstract_syntax_tree
NUMBER   = $(SRC)/satellite_number
STRING   = $(SRC)/satellite_string
VALUE    = $(SRC)/satellite_value
LIBRARY  = $(SRC)/satellite_library
LEXER    = $(SRC)/lexical_analyzer
PARSER   = $(SRC)/syntax_parser
ENV      = $(SRC)/environment
EVAL     = $(SRC)/evaluator
INTERP   = $(SRC)/interpreter
LOADER   = $(SRC)/spaceship_loader
CONSOLE  = $(SRC)/console_output
RANDOM   = $(SRC)/random_numbers
SYSTEM   = $(SRC)/system_facts
FORMAT   = $(SRC)/bytecode_format
REG      = $(SRC)/register_file
PROGRAMS = $(SRC)/programs

# The tests root, and the one directory variable that is NOT under $(SRC).
# Every test lives in satellite_system/tests/<test_name>/, one folder per test
# binary, because a test that has been split into six files needs somewhere to
# put them that is not the module it tests. -I$(SRC) is what still makes their
# includes root-relative, so nothing about how a test includes changed.
TESTS    = satellite_system/tests

# One name per test binary, and the single place a test is declared to exist.
# TESTBINS, the aliases, `test`, `clean` and the per-test source wildcards are
# all derived from this list, so adding a test means adding one word here.
TESTNAMES = library_test satellite_string_test lexer_test ast_test parser_test \
            eval_test interp_test loader_test env_test spacesuit_test \
            bignum_test random_test format_test console_test reg_test

# eval.cpp was 2208 lines. Split into evaluator/ at the seams the code already
# had, none of the twelve reaching 400. Listed explicitly rather than by
# wildcard so that a file added to the directory and forgotten here fails to
# link instead of being silently dropped from the binary.
ENV_SRCS = $(ENV)/scopes.cpp $(ENV)/names.cpp $(ENV)/walk.cpp \
           $(ENV)/spacesuits.cpp $(ENV)/run.cpp
ENV_OBJS = $(ENV_SRCS:.cpp=.o)

BIGNUM_SRCS = $(NUMBER)/limbs.cpp $(NUMBER)/number_core.cpp \
              $(NUMBER)/number_query.cpp $(NUMBER)/number_arith.cpp \
              $(NUMBER)/render.cpp $(NUMBER)/random.cpp
BIGNUM_OBJS = $(BIGNUM_SRCS:.cpp=.o)

PARSER_SRCS = $(PARSER)/cursor.cpp $(PARSER)/types.cpp $(PARSER)/expr.cpp \
              $(PARSER)/stmt.cpp $(PARSER)/decl.cpp $(PARSER)/run.cpp
PARSER_OBJS = $(PARSER_SRCS:.cpp=.o)

EVAL_SRCS = $(EVAL)/helpers.cpp $(EVAL)/help.cpp $(EVAL)/types.cpp \
            $(EVAL)/analyze.cpp \
            $(EVAL)/session.cpp $(EVAL)/stmt.cpp $(EVAL)/slots.cpp \
            $(EVAL)/expr.cpp $(EVAL)/methods.cpp $(EVAL)/mutators.cpp \
            $(EVAL)/modules.cpp $(EVAL)/calls.cpp $(EVAL)/operators.cpp \
            $(EVAL)/maps.cpp \
            $(EVAL)/modules_file.cpp $(EVAL)/modules_help.cpp \
            $(EVAL)/modules_system.cpp $(EVAL)/modules_directory.cpp \
            $(EVAL)/modules_random.cpp $(EVAL)/modules_console.cpp \
            $(EVAL)/methods_scalars.cpp $(EVAL)/methods_file.cpp \
            $(EVAL)/methods_containers.cpp $(EVAL)/methods_bits.cpp
EVAL_OBJS = $(EVAL_SRCS:.cpp=.o)

OBJS      = $(PROGRAMS)/main.o $(LIBRARY)/library.o \
            $(STRING)/satellite_string.o $(SYSTEM)/system.o \
            $(LEXER)/lexer.o $(AST)/ast.o $(VALUE)/value.o $(VALUE)/bits.o \
            $(LOADER)/loader.o $(INTERP)/interp.o $(RANDOM)/random.o \
            $(CONSOLE)/console.o $(EVAL_OBJS) \
            $(PARSER_OBJS) $(BIGNUM_OBJS) $(ENV_OBJS)
HDRS      = $(LIBRARY)/library.hpp $(VALUE)/value.hpp \
            $(STRING)/satellite_string.hpp $(SYSTEM)/system.hpp \
            $(NUMBER)/bignum.hpp $(LEXER)/lexer.hpp $(AST)/ast.hpp \
            $(PARSER)/parser.hpp $(ENV)/env.hpp $(EVAL)/eval.hpp \
            $(LOADER)/loader.hpp $(INTERP)/interp.hpp $(RANDOM)/random.hpp \
            $(CONSOLE)/console.hpp $(EVAL)/eval_internal.hpp \
            $(PARSER)/parser_internal.hpp $(NUMBER)/bignum_internal.hpp \
            $(ENV)/env_internal.hpp
# version.hpp is not in HDRS either, and for the OPPOSITE reason to format.hpp
# below: three objects DO include it -- main.o, window.o and help.o -- and those
# three name it as a prerequisite on their own rules instead. Every object
# depends on HDRS, so listing it here would rebuild forty-three of them to
# change a string three of them read, which is the same waste that keeps
# VERSION_DEFS off CXXFLAGS twenty lines up. What must NOT happen is what was
# true until this line was written: the header in no rule at all, so editing it
# rebuilt nothing and the binary went on reporting the version it was built
# with. That is format.def's defect exactly, and §17 spends a section on it.
#
# format.hpp and format.def are deliberately NOT in HDRS. Every object depends on
# HDRS, and no object includes either file — there is no VM yet — so listing them
# would make one edit to format.def rebuild the whole interpreter for nothing.
# The format_test rule below names them itself, which is the dependency that is
# actually real. Add them here when a translation unit in OBJS includes them.
TESTSRCS  = $(LIBRARY)/library.cpp $(STRING)/satellite_string.cpp \
            $(SYSTEM)/system.cpp $(LEXER)/lexer.cpp $(AST)/ast.cpp \
            $(VALUE)/value.cpp $(VALUE)/bits.cpp $(LOADER)/loader.cpp $(INTERP)/interp.cpp \
            $(RANDOM)/random.cpp $(CONSOLE)/console.cpp \
            $(EVAL_SRCS) $(PARSER_SRCS) $(BIGNUM_SRCS) $(ENV_SRCS)
TESTFLAGS = -std=c++20 -Wall -Wextra -pthread

# Every test binary used to recompile every source. That was tolerable while
# there were eleven of them; splitting eval.cpp into twelve took a serial
# `make test` past ten minutes, because the cost is (test binaries) x (sources)
# and the split doubled the second term. Linking the objects the binary already
# built makes it (sources) + (test binaries).
#
# main.o is excluded because it defines main() and so does every test.
# library_test_tsan is NOT converted: -fsanitize=thread has to be on every
# translation unit it links, and these objects are not built with it.
LIBOBJS   = $(filter-out $(PROGRAMS)/main.o,$(OBJS))

# Both, on a machine that can build both. On one that cannot, the interpreter
# and an explanation -- see the note on MISSING_PKGS at the top.
ifeq ($(MISSING_PKGS),)
  GUI_TARGET = satl-term
else
  GUI_TARGET = gui-skipped
endif

# `make` at the top of this tree BUILDS and then INSTALLS, and the second half
# of that sentence is unusual enough to earn a paragraph.
#
# A build tree is not an install, and at a shell prompt the two are
# indistinguishable: ./satl and the satl on PATH are different files, and which
# one answers to `satl` is whichever the shell finds first. So a fix compiled
# here was routinely being tried against the copy installed last week, and the
# artwork in dist/icons was routinely not the artwork the desktop was drawing.
# No test catches either: the tests link the objects and never go near either
# binary. Ending the build where the last install ended is what stops the two
# from drifting. The icon walk in `install` below is an unconditional copy, so
# every build overwrites the installed icons with what is in dist/icons right
# now -- which is the point of doing it every time rather than when something
# looks stale.
#
# It fires only when make was invoked with THIS directory as its working
# directory: $(CURDIR) against the directory this file was read from. That is
# necessary and it is NOT sufficient. `make -C <tree>` sets CURDIR to <tree>, so
# it is indistinguishable from `cd <tree> && make` -- same CURDIR, same PWD,
# same MAKELEVEL, measured -- and both of this repository's own callers invoke
# make exactly that way: install.sh builds with `make -C "$repo"` before running
# its own install, and debian/rules arrives through dh_auto_build with
# prefix=/usr and no DESTDIR, where an install would write into the build
# machine's live /usr instead of debian/tmp and slip past dh_missing entirely.
# Neither is guessed at. Both pass SATELLITE_AUTOINSTALL=0 on the make command
# line -- the one level that outranks the assignment below, for the reason
# debian/rules:10-13 already spells out -- and this comment is what the comments
# there point back to.
#
# And it never runs sudo. The prefix is the one chosen above, a directory the
# caller already owns; where it is not writable the install is REFUSED, in
# words, with the command that would have worked. `deps` above refuses to
# escalate for the same reason, one target down. Nothing here can fail the
# build, either: every path through the recipe exits 0, because a tarball built
# by an rpm spec or a Nix derivation reaches this line with an unwritable
# prefix and no way to pass the opt-out, and a printed note is the right
# outcome there.
# A NAMED prefix is the second half of the answer, and it is the half that
# needs no cooperation from anybody. Every caller that must not auto-install --
# debian/rules through dh_auto_build, install.sh's two build steps, and the rpm
# spec or Nix derivation or PKGBUILD that no edit in this tree can reach --
# passes prefix on the make command line, because all of them have somewhere
# specific to put the files. So `$(origin prefix)` answers the question that
# CURDIR cannot: a build that was told where the tree goes is a build whose
# install somebody else is performing. The two callers in this repository pass
# SATELLITE_AUTOINSTALL=0 as well, and the redundancy is deliberate -- it says
# in their own files what they mean, rather than leaving it to be inferred from
# a variable they pass for another reason entirely.
#
# lastword rather than firstword: the MAKEFILES environment variable prepends
# to MAKEFILE_LIST, so firstword can name a file nobody here wrote. realpath
# rather than abspath: CURDIR arrives from getcwd() with symlinks already
# resolved, while a -f path does not, so abspath compares a resolved path
# against an unresolved one and answers no to a tree reached through a symlink.
SATELLITE_AUTOINSTALL ?= 1
HERE := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
AUTOINSTALL := $(strip $(if $(filter-out 0,$(SATELLITE_AUTOINSTALL)),\
                   $(if $(DESTDIR),,\
                     $(if $(filter file undefined,$(origin prefix)),\
                       $(if $(subst $(realpath $(CURDIR)),,$(HERE)),,\
                         $(if $(filter 0,$(MAKELEVEL)),yes))))))

# --silent for that sub-make, but only when this make is not a dry run: `make -n`
# has to SHOW the install it would do, and -s suppresses exactly that printing.
# A recipe line mentioning $(MAKE) is run even under -n -- that is how a dry run
# recurses at all -- so the sub-make is where the question has to be asked. The
# first word of MAKEFLAGS is the bundle of single-letter flags; long options
# arrive later and start with a dash, which is what the filter drops.
MAKE_SHORT_FLAGS := $(filter-out -%,$(firstword $(MAKEFLAGS)))
INSTALL_QUIET    := $(if $(findstring n,$(MAKE_SHORT_FLAGS)),,--silent)

# Three more words to that sub-make, each buying one thing:
#
#   -o satl -o satl-term   `make -B` means rebuild everything, and it reaches
#                          the sub-make through MAKEFLAGS, where `install`
#                          lists both binaries as prerequisites -- so a -B
#                          build would compile the whole tree, install it, and
#                          then compile the whole tree a second time. --old-file
#                          on the two binaries `all` has just finished building
#                          is exactly the statement that they are current, and
#                          it beats -B (measured).
#   GUI_TARGET=            the GUI question was already asked and answered by
#                          the make that got here. Left to ask it again, the
#                          sub-make prints the gui-skipped banner a second time
#                          on every build on a machine without gtk.
#
# And where the advice below names a command for the caller to run as root, it
# names install.sh and not `sudo make`. `make` inside the tree writes .o files,
# relinks satl and rewrites .libdir-stamp, all as root, in a directory the user
# owns -- after which their next ordinary `make` cannot write the stamp and the
# build fails with a permission error nobody connects to the sudo they typed
# an hour earlier. install.sh:263-273 already solves this: it compiles as the
# human and uses root for the copy alone.
# The folder https://satellite.foundation/ hands out when somebody clicks
# download: the two binaries this machine just built, and the installer beside
# them. Enterprise Linux is what it is because that is what this machine is --
# an EL 10 build links EL 10's libstdc++ and runs on EL 9/10 and its rebuilds,
# and on nothing older. A Debian bundle has to be built on Debian, which is why
# this variable names the distribution rather than the word "linux".
#
# Under the same condition as the auto-install below, and not on `every make`
# literally: debian/rules reaches `all` too, and a package build that dropped a
# directory of binaries into the unpacked source would have dpkg-source
# complaining about a tree that changed while it was being built.
#
# gitignored, because it holds build products. Nothing in it is a source file
# and every one of them is rewritten by the next make.
DOWNLOAD_DIR = enterprise_download
DOWNLOAD_TAR = satellite_rhel.tar.xz

# The data half of the bundle: the whole install tree, staged prefix-relative,
# so install.sh can put it somewhere without a Makefile, a compiler or a copy of
# the file list. Written by the `bundle` rule below, which explains all three.
# Under DOWNLOAD_DIR so that it travels inside the tarball with the binaries.
BUNDLE_TREE = $(DOWNLOAD_DIR)/install_tree

# `all`, whatever order the rules below end up in.
.DEFAULT_GOAL := all

all: satl $(GUI_TARGET) $(if $(AUTOINSTALL),bundle)
ifeq ($(AUTOINSTALL),yes)
	@dir='$(prefix)'; \
	while [ ! -d "$$dir" ] && [ "$$dir" != / ]; do dir=`dirname "$$dir"`; done; \
	if [ ! -w "$$dir" ]; then \
	    printf 'satl built, and NOT installed: %s is not writable by %s.\n' \
	        "$$dir" "`id -un 2>/dev/null || echo you`"; \
	    printf 'A build does not run sudo. To install it as root:\n'; \
	    printf '    sudo ./install.sh --prefix %s\n' '$(prefix)'; \
	elif ! $(MAKE) --no-print-directory $(INSTALL_QUIET) \
	          -o satl -o satl-term GUI_TARGET= \
	          install install-report SATELLITE_AUTOINSTALL=0; then \
	    printf 'satl built, but the install did not finish.\n'; \
	    printf 'Run `make install` to see what stopped it.\n'; \
	fi
endif

# BELOW `all`, and that position is load-bearing: the first target in a
# makefile is the default goal, so defining this one above `all` quietly made
# `make` build the bundle and nothing else -- binaries fresh, install skipped,
# exit 0, no error anywhere. .DEFAULT_GOAL is set at the top as well, so the
# next rule that lands in the wrong place cannot repeat it.
bundle: satl $(GUI_TARGET)
	@rm -rf '$(DOWNLOAD_DIR)'
	@install -d -m755 '$(DOWNLOAD_DIR)'
	@install -m755 satl '$(DOWNLOAD_DIR)/satl'
	@if [ -f satl-term ]; then \
	    install -m755 satl-term '$(DOWNLOAD_DIR)/satl-term'; \
	fi
	@install -m755 install.sh '$(DOWNLOAD_DIR)/install.sh'
# THE DATA HALF OF THE BUNDLE, and the reason install.sh works from inside it.
#
# The three files above are a program; they are not an install. The icons, the
# mime packet, the .desktop entry, the man pages, the examples, the design and
# the licence are the rest of it, and until this rule existed the bundle had
# none of them -- so install.sh, which drives `make install` and needs a
# Makefile beside it, exited with "no Makefile beside ./install.sh" and a user
# who downloaded, unpacked and ran it installed nothing at all.
#
# STAGED BY `make install` ITSELF rather than by a second list of files here.
# That is the whole point: install.sh:4-8 says the install tree is declared
# exactly once, in the `install` target, because two lists is how an install
# tree rots. This bundle does not carry a copy of the list -- it carries the
# RESULT of the one list, produced by running it. Add a data file to `install`
# above and it is in the next bundle with no edit here.
#
# prefix= EMPTY, which is what makes the staged tree prefix-relative:
# $(bindir) becomes /bin and $(datadir) becomes /share, so the tree is
# install_tree/bin/satl and install_tree/share/..., and installing it anywhere
# is a copy with no path translation on either side. install.sh does not have
# to know what prefix this machine built at.
#
# -o satl -o satl-term GUI_TARGET=, for the reason `all` passes the same three
# words one target up: prefix is compiled into system.o through .libdir-stamp,
# so a sub-make at a DIFFERENT prefix -- and empty is a different prefix --
# would rewrite the stamp, recompile system.o and relink satl, at a prefix that
# is not a directory. --old-file on the two binaries says they are current, and
# the sub-make then reaches `install` with nothing to compile. **Verified**
# with `make -n`: zero compile lines, and .libdir-stamp is not touched.
#
# The binary in the tree is therefore the one built for THIS machine's prefix,
# with that prefix baked in as SATELLITE_LIB_DIR -- and it does not matter,
# because tier 2 of library_path() resolves ../share/satellite/lib from
# /proc/self/exe (DESIGN §9), and `install` creates that directory empty for
# exactly this reason. The bundle installs correctly at /usr/local, at
# $HOME/.local, or anywhere else, with no rebuild and no environment variable.
#
# DESTDIR is set, so `install` skips its own index-rebuild block -- correct
# here, this is staging. install.sh runs those three tools after it copies.
	@$(MAKE) --no-print-directory --silent -o satl -o satl-term GUI_TARGET= \
	    install prefix= DESTDIR='$(CURDIR)/$(BUNDLE_TREE)' \
	    SATELLITE_AUTOINSTALL=0
# The tarball sits OUTSIDE the folder it archives, in the project root, so that
# unpacking it recreates the folder rather than scattering three files into
# whatever directory the download landed in.
#
# --sort=name, and the mtime/owner clamps, for the reason `gzip -9n` is used on
# the man pages: two builds of the same binaries should produce the same
# archive, byte for byte, so a published checksum means something.
	@tar --create --xz --file '$(DOWNLOAD_TAR)' \
	     --sort=name --owner=root:0 --group=root:0 \
	     --mtime='@0' --format=gnu '$(DOWNLOAD_DIR)'
	@printf 'bundled    %s/  and  %s\n' '$(DOWNLOAD_DIR)' '$(DOWNLOAD_TAR)'

gui-skipped:
	@printf 'satl built.\n'
	@printf 'satl-term SKIPPED: pkg-config cannot find %s\n' '$(MISSING_PKGS)'
	@printf 'To build the terminal window too, %s\n' '$(DEPS_ADVICE)'

# Explicit, and never run by `make` on its own: installing system packages is
# the user's decision, and a build that quietly took it would be a build that
# runs sudo without being asked. No -y -- the package manager lists what it is
# about to do and asks, which is the confirmation this deliberately keeps.
deps:
ifeq ($(DEPS_CMD),)
	@printf 'No dnf, apt-get or pacman here -- %s\n' '$(DEPS_ADVICE)'
	@exit 1
else
	$(DEPS_CMD)
endif

satl: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ -pthread

satl-term: $(PROGRAMS)/window.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

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
$(SRC)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC) -c -o $@ $<

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

# --- what a test is built from ----------------------------------------------
# EVERY .cpp in a test's folder is part of that test. That is the rule, so it is
# a wildcard rather than fifteen hand-written lists: a test that gets split into
# six files needs no Makefile edit, and -- more to the point -- a new piece
# CANNOT be forgotten. .gitignore's note records what the hand-copied list cost
# twice; this is the same lesson applied one level up.
#
# $(wildcard) expands when the Makefile is read, so a file added during a build
# is picked up by the next one. That is the only cost and it is the right trade.
$(foreach t,$(TESTNAMES),$(eval $(t)_SRCS = $$(wildcard $$(TESTS)/$(t)/*.cpp)))

# The HEADERS are a separate list because they are a DEPENDENCY and not an
# input: a split test grew a <name>.hpp holding the harness declarations and the
# section prototypes, and without this a change to that header would not relink
# the binary. Found by the audit of the 2026-08-24 split, which is exactly the
# kind of stale-build hazard that only shows up as a confusing test result later.
$(foreach t,$(TESTNAMES),$(eval $(t)_HDRS = $$(wildcard $$(TESTS)/$(t)/*.hpp)))

$(TESTS)/library_test/library_test: $(library_test_SRCS) $(library_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(library_test_SRCS) $(LIBOBJS)

# The one test that does NOT follow $(OPT), and deliberately: a sanitizer build
# wants -O1 and -g, because the instrumentation is what is being run and a
# report without line numbers is not one. $(OPT) is the knob for the code that
# ships, and this binary does not ship.
$(TESTS)/library_test/library_test_tsan: $(library_test_SRCS) $(TESTSRCS) $(HDRS)
	$(TSAN_CXX) $(TESTFLAGS) $(PCGFLAGS) -I$(SRC) -O1 -g -fsanitize=thread \
	    -o $@ $(library_test_SRCS) $(TESTSRCS)

$(TESTS)/satellite_string_test/satellite_string_test: $(satellite_string_test_SRCS) $(satellite_string_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(satellite_string_test_SRCS) $(LIBOBJS)

$(TESTS)/lexer_test/lexer_test: $(lexer_test_SRCS) $(lexer_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(lexer_test_SRCS) $(LIBOBJS)

$(TESTS)/ast_test/ast_test: $(ast_test_SRCS) $(ast_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(ast_test_SRCS) $(LIBOBJS)

$(TESTS)/parser_test/parser_test: $(parser_test_SRCS) $(parser_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(parser_test_SRCS) $(LIBOBJS)

$(TESTS)/eval_test/eval_test: $(eval_test_SRCS) $(eval_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(eval_test_SRCS) $(LIBOBJS)

$(TESTS)/interp_test/interp_test: $(interp_test_SRCS) $(interp_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(interp_test_SRCS) $(LIBOBJS)

$(TESTS)/loader_test/loader_test: $(loader_test_SRCS) $(loader_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(loader_test_SRCS) $(LIBOBJS)

$(TESTS)/env_test/env_test: $(env_test_SRCS) $(env_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(env_test_SRCS) $(LIBOBJS)

$(TESTS)/spacesuit_test/spacesuit_test: $(spacesuit_test_SRCS) $(spacesuit_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(spacesuit_test_SRCS) $(LIBOBJS)

$(TESTS)/bignum_test/bignum_test: $(bignum_test_SRCS) $(bignum_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(bignum_test_SRCS) $(LIBOBJS)

# The one test binary whose runtime is a design parameter rather than an
# accident: every end-to-end case spends its tier's throwaway window before it
# answers, so the cases here are on `fast` (50-100 ms) and the sampler itself is
# tested through a stub generator that does not spin at all.
$(TESTS)/random_test/random_test: $(random_test_SRCS) $(random_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(random_test_SRCS) $(LIBOBJS)

# format.hpp links against NOTHING — it includes only <cstdint> and <cstddef>,
# so this is the one test binary that needs no objects at all. That is a
# property of the format and worth keeping: the registry must be readable by a
# disassembler, a loader, or the bootstrap's generated C, none of which should
# have to drag the interpreter in to learn what id 7 is.
#
# Most of this test runs at COMPILE time. format.hpp ends in static_asserts over
# the X-macro lists, so a duplicate id or a hole in the registry fails right
# here rather than in the binary.
$(TESTS)/format_test/format_test: $(format_test_SRCS) $(format_test_HDRS) $(FORMAT)/format.hpp \
                    $(FORMAT)/format.def
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(format_test_SRCS)

$(TESTS)/console_test/console_test: $(console_test_SRCS) $(console_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(console_test_SRCS) $(LIBOBJS)

# reg.hpp is not linked into satl: there is no VM yet, and nothing in the
# interpreter includes it. This binary is the only consumer. (This comment sat
# over console_test until the move; it was always describing this recipe.)
$(TESTS)/reg_test/reg_test: $(reg_test_SRCS) $(reg_test_HDRS) $(REG)/reg.hpp $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(reg_test_SRCS) $(LIBOBJS)

# The fifteen test binaries. They no longer sit beside the code they test --
# each one is built in its own folder under $(TESTS), because a test split into
# six files needs a folder and a test binary needs somewhere to land that is not
# the module's source directory.
#
# Still ONE list, used as the dependency list here, by the aliases below and by
# `clean` -- because .gitignore's own note records what a second hand-copied
# list costs: reg_test drifted out of one and 561KB of binary went into a
# commit.
TESTBINS = $(TESTS)/library_test/library_test $(TESTS)/satellite_string_test/satellite_string_test \
           $(TESTS)/bignum_test/bignum_test $(TESTS)/random_test/random_test \
           $(TESTS)/format_test/format_test $(TESTS)/reg_test/reg_test $(TESTS)/console_test/console_test \
           $(TESTS)/lexer_test/lexer_test $(TESTS)/ast_test/ast_test $(TESTS)/parser_test/parser_test \
           $(TESTS)/env_test/env_test $(TESTS)/eval_test/eval_test $(TESTS)/interp_test/interp_test \
           $(TESTS)/loader_test/loader_test $(TESTS)/spacesuit_test/spacesuit_test

test: $(TESTBINS) $(TSAN_TEST)
	./$(TESTS)/library_test/library_test
	$(if $(TSAN_TEST),./$(TSAN_TEST))
	./$(TESTS)/satellite_string_test/satellite_string_test
	./$(TESTS)/bignum_test/bignum_test
	./$(TESTS)/random_test/random_test
	./$(TESTS)/format_test/format_test
	./$(TESTS)/reg_test/reg_test
	./$(TESTS)/console_test/console_test
	./$(TESTS)/lexer_test/lexer_test
	./$(TESTS)/ast_test/ast_test
	./$(TESTS)/parser_test/parser_test
	./$(TESTS)/env_test/env_test
	./$(TESTS)/eval_test/eval_test
	./$(TESTS)/interp_test/interp_test
	./$(TESTS)/loader_test/loader_test
	./$(TESTS)/spacesuit_test/spacesuit_test

# A test binary lives under $(TESTS)/<name>/, so its path is no longer its name.
# These keep `make ast_test` working, which is what fingers already type and
# what every note in DESIGN.md that mentions running one still says. Each is
# phony and does nothing but ask for the real path.
ast_test:              $(TESTS)/ast_test/ast_test
bignum_test:           $(TESTS)/bignum_test/bignum_test
console_test:          $(TESTS)/console_test/console_test
env_test:              $(TESTS)/env_test/env_test
eval_test:             $(TESTS)/eval_test/eval_test
format_test:           $(TESTS)/format_test/format_test
interp_test:           $(TESTS)/interp_test/interp_test
lexer_test:            $(TESTS)/lexer_test/lexer_test
library_test:          $(TESTS)/library_test/library_test
library_test_tsan:     $(TESTS)/library_test/library_test_tsan
loader_test:           $(TESTS)/loader_test/loader_test
parser_test:           $(TESTS)/parser_test/parser_test
random_test:           $(TESTS)/random_test/random_test
reg_test:              $(TESTS)/reg_test/reg_test
satellite_string_test: $(TESTS)/satellite_string_test/satellite_string_test
spacesuit_test:        $(TESTS)/spacesuit_test/spacesuit_test

TESTALIASES = ast_test bignum_test console_test env_test eval_test \
              format_test interp_test lexer_test library_test \
              library_test_tsan loader_test parser_test random_test \
              reg_test satellite_string_test spacesuit_test

# satellite against compiled C++ with the compiler's own time counted, which is
# the comparison that changes the answer. Depends on `satl` because the
# driver refuses to guess at an interpreter that is not built; the build of the
# interpreter is deliberately not part of what it measures.
compare: satl
	$(MAKE) -C example/cxx_compare run

# satellite against CPython. Two interpreters, so there is no build step on
# either side and the number is simply the whole-process wall clock.
python: satl
	$(MAKE) -C example/py_compare run

# gzip -9n, not -9c on a named file: -n keeps the source file name and the
# current time out of the gzip header, which is what makes two builds of the
# same page byte-identical and is why dh_compress uses it. dh_compress cannot
# repair ours, because it skips what is already .gz. Compressing here rather
# than in the install recipe is what puts the modes under install(1) instead of
# under the caller's umask; the temporary file is so that a failed gzip cannot
# leave a truncated page behind under a name make would then believe in.
dist/%.1.gz: dist/%.1
	gzip -9nc $< > $@.tmp && mv $@.tmp $@

# The tree that library_path()'s tiers 2 and 3 both describe. Its whole
# obligation is that `satl --where` answers correctly afterwards with no
# environment variable set -- an install that needs one is not an install.
#
# Every path is quoted, because prefix and DESTDIR are given by the caller and
# a tarball is routinely unpacked, or staged, somewhere with a space in the
# path. Unquoted, `prefix="/opt/my satellite"` turns one path into two words
# and the recipes below silently install to the wrong places -- and the
# uninstall recipe, being rm -rf, deletes them.
#
# $(GUI_TARGET) rather than satl-term, and the two satl-term lines below ask
# whether the file is there: this target is now reached by `all`, and `all` is
# what must keep working on a machine with no gtk. Naming satl-term outright
# made the missing-package guard on window.o fire (Makefile's note on
# MISSING_PKGS at the top), which turned the graceful skip into the hard build
# failure that guard exists to prevent. The interpreter still installs there,
# alone, which is the same argument one target up: there is no reason for it to
# go down with the window. A package build is unaffected -- debian/control
# Build-Depends on both libraries, so the file is always there -- and if it ever
# were not, debian/satellite-term.install names usr/bin/satl-term and dh_install
# fails loudly, which is where that failure belongs.
install: satl $(GUI_TARGET) dist/satl.1.gz dist/satl-term.1.gz
	install -Dm755 satl "$(DESTDIR)$(bindir)/satl"
	if [ -f satl-term ]; then \
	    install -Dm755 satl-term "$(DESTDIR)$(bindir)/satl-term"; \
	fi
# Every directory this install creates gets its mode stated, for the same
# reason every file does. install -d without -m takes the umask, so under the
# 002 that is Ubuntu's default for the primary user -- and common in CI images
# -- share/ and share/satellite/ came out group-writable, which is a package
# shipping a group-writable /usr/share. Each level is named, because -m applies
# to the directories named and not to the ancestors created along the way.
#
# lib/ is created empty on purpose. There are no .satl files to ship yet, but
# the search accepts a candidate only if the DIRECTORY is there, so this empty
# directory is exactly what makes a relocated tarball resolve to itself instead
# of falling through to the prefix it was built for.
	install -d -m755 "$(DESTDIR)$(datadir)" \
	                 "$(DESTDIR)$(datadir)/satellite" \
	                 "$(DESTDIR)$(datadir)/satellite/lib" \
	                 "$(DESTDIR)$(mandir)" "$(DESTDIR)$(mandir)/man1"
# example/ is a tree, not a flat list, so it is walked rather than globbed.
# Executables are skipped: `make compare` and `make python` leave their
# compiled C++ drivers in these directories, and one machine's binaries are
# not an example of anything.
	find example -type f ! -perm -u+x -printf '%P\n' | while read -r f; do \
	    install -Dm644 "example/$$f" \
	        "$(DESTDIR)$(datadir)/satellite/examples/$$f" || exit 1; \
	done
	install -Dm644 DESIGN.md "$(DESTDIR)$(docdir)/DESIGN.md"
# DESIGN.md is now the index and design/ is the document -- nineteen numbered
# sections, one per file -- so installing one without the other ships a page
# of links to nothing. Globbed rather than walked because design/ is flat and
# every file in it is a part; -t rather than a per-file -D because a
# multi-source install needs the destination named as a directory. The leading
# directories still come out 755 under a 002 umask, which is what the note on
# `install -d` above is about -- **verified** rather than assumed.
	install -Dm644 -t "$(DESTDIR)$(docdir)/design" design/*.md
# The licence ships under the name every tool looks for, so that a tarball
# install states its terms as fully as the .deb does: dh_installdocs writes
# debian/copyright to this same path, and debian/copyright is a transcription
# of LICENSE precisely so the two cannot say different things.
	install -Dm644 LICENSE "$(DESTDIR)$(docdir)/copyright"
# README.md is not written yet. The guard is so that `make install` works today
# and picks it up the day it lands, rather than failing now and needing a
# second edit here later. It is an `if` rather than a `|| true` so that a real
# failure to copy an existing README still stops the install.
	if [ -f README.md ]; then \
	    install -Dm644 README.md "$(DESTDIR)$(docdir)/README.md"; \
	fi
	install -Dm644 dist/satl.1.gz \
	    "$(DESTDIR)$(mandir)/man1/satl.1.gz"
	install -Dm644 dist/satl-term.1.gz \
	    "$(DESTDIR)$(mandir)/man1/satl-term.1.gz"
# Named after the GApplication id that window.cpp registers, because that is
# the string GTK puts on the toplevel and the string a desktop shell looks a
# .desktop file up by. Installed as satl-term.desktop the window arrives in the
# shell associated with nothing: no icon, and nothing to pin.
# Under the same guard as the binary: a launcher whose Exec= names a program
# that was never built is an entry in the shell's menu that fails when clicked.
	if [ -f satl-term ]; then \
	    install -Dm644 dist/org.satellite.terminal.desktop \
	        "$(DESTDIR)$(datadir)/applications/org.satellite.terminal.desktop"; \
	fi
# The icon tree under dist/icons mirrors its install destination exactly, so
# this is a copy and not a translation: every path under dist/icons/hicolor is
# already <size>/<context>/<name>, and getting the layout wrong is a mistake
# that shows up as a missing icon rather than as a build failure. hicolor is
# the theme every other theme falls back to, so the artwork is found whichever
# theme the user has chosen, and the basenames are the two names that are
# looked up: org.satellite.terminal for Icon= in the .desktop entry, and
# application-x-satellite for the mime type declared in
# dist/application-x-satellite.xml.
#
# These are pixel sizes rather than the single scalable/ SVG that used to live
# here, because the artwork is now a photograph and a photograph has no
# scalable form. dist/org.satellite.terminal.svg is still in the tree and is
# still a complete icon: to go back to it, restore the one-line install of
# scalable/apps/ and drop the apps/ half of this walk. Installing BOTH is the
# one thing that does not work -- the theme spec lets either satisfy a lookup,
# so which one a shell picks stops being predictable.
	find dist/icons -type f -name '*.png' -printf '%P\n' | while read -r f; do \
	    install -Dm644 "dist/icons/$$f" \
	        "$(DESTDIR)$(datadir)/icons/$$f" || exit 1; \
	done
# The mime packet, which is what makes a .satl file a satellite file rather
# than an unlabelled text file. It has to be installed before
# update-mime-database runs below: that tool compiles every packet in
# packages/ into the binary index a file manager actually reads, and a packet
# added afterwards is inert until something triggers the compile again.
	install -Dm644 dist/application-x-satellite.xml \
	    "$(DESTDIR)$(datadir)/mime/packages/application-x-satellite.xml"
# A shell finds a launcher through two indexes, and a newly installed .desktop
# and icon are invisible until both are rebuilt -- which is why a fresh install
# shows the generic icon until the next login. Skipped entirely when DESTDIR is
# set: that tree is staging for a package, and dpkg fires its own triggers on
# the installing machine. Failure is ignored because neither tool is required
# for the install to be correct, only for it to be noticed promptly.
#
# update-mime-database joins them for the same reason and under the same guard:
# the packet installed above is XML that nothing reads directly, and until it
# is compiled into share/mime/mime.cache a .satl file keeps whatever type
# content sniffing alone gives it.
#
# The index.theme line is not decoration. An icon directory is only a THEME if
# it contains one, and without it GTK does not look inside at all: has_icon()
# answers false for every icon just installed, at every size, and the desktop
# shows the generic fallback. The file belongs to hicolor-icon-theme, which
# installs it under /usr and nowhere else -- so EVERY prefix except /usr starts
# without one, and that includes this Makefile's own default of /usr/local. An
# install that leaves it missing has copied eighteen PNGs nothing will ever read.
#
# gtk-update-icon-cache does not reveal the problem, because -t is
# --ignore-theme-index: the cache builds happily over a directory that is not
# yet a theme, so the only symptom is artwork that never appears.
#
# Copied rather than generated, because the file describes hicolor itself -- its
# directory list and their sizes and contexts -- and not our subset of it. Only
# when absent, so any prefix that already has one is left alone.
#
# It is deliberately NOT removed by uninstall below. Every other application
# that installs an icon into this prefix depends on it, so deleting ours on the
# way out would break theirs; an orphaned theme index is the correct outcome.
	@if [ -z "$(DESTDIR)" ]; then \
	    if [ ! -f "$(datadir)/icons/hicolor/index.theme" ] && \
	       [ -f /usr/share/icons/hicolor/index.theme ]; then \
	        cp /usr/share/icons/hicolor/index.theme \
	           "$(datadir)/icons/hicolor/index.theme" 2>/dev/null || true; \
	    fi; \
	    command -v update-desktop-database >/dev/null 2>&1 && \
	        update-desktop-database "$(datadir)/applications" 2>/dev/null || true; \
	    command -v gtk-update-icon-cache >/dev/null 2>&1 && \
	        gtk-update-icon-cache -qtf "$(datadir)/icons/hicolor" 2>/dev/null || true; \
	    command -v update-mime-database >/dev/null 2>&1 && \
	        update-mime-database "$(datadir)/mime" 2>/dev/null || true; \
	fi
# bash-completion loads the file named after the command, so the extension
# that distinguishes it in dist/ is dropped on the way in.
	install -Dm644 dist/satl.bash-completion \
	    "$(DESTDIR)$(datadir)/bash-completion/completions/satl"

# What the auto-install says when it is done, and the reason it is a target of
# its own rather than three more lines in `all`: a recipe line that mentions
# $(MAKE) is executed even under `make -n` -- that is how a dry run recurses at
# all -- so a report printed inline would announce an install that a dry run did
# not perform. Reached as a GOAL of that sub-make it inherits -n along with
# everything else and is printed rather than run, which is what a dry run is for.
#
# It is deliberately NOT part of `install`. A packager staging a tree into
# debian/tmp is not the audience for advice about this machine's PATH.
install-report:
	@printf 'installed  %s\n' '$(bindir)/satl'
	@if [ -f satl-term ]; then printf 'installed  %s\n' '$(bindir)/satl-term'; fi
	@printf 'icons      %s\n' '$(datadir)/icons/hicolor'
# Membership in PATH is not the question; PRECEDENCE is. `satl` runs whichever
# copy the shell finds first, so an install into a directory that is on PATH but
# behind another directory that also has a satl changes nothing the user can
# see -- and printing "installed" and stopping there would be the kind of true
# sentence that misleads. install.sh:332 asks the membership question and says
# "so `satl` finds it", which on this machine was already false.
#
# -ef as well as a string compare, because a PATH entry can reach the same
# directory through a symlink: the strings differ, the file does not, and a
# string compare alone reports a shadow that is not there.
#
# `hash -r` is named only in the branch that needs it. Overwriting a binary in
# place needs nothing, because the shell caches the resolved path and not the
# inode; CREATING one earlier in PATH than the copy the shell has already hashed
# is the case that needs the reminder. Advice printed every time is advice
# nobody reads.
	@found=`command -v satl 2>/dev/null || :`; \
	mine='$(bindir)/satl'; \
	if [ -z "$$found" ]; then \
	    printf 'note: %s is not on your PATH, so typing `satl` will not find it.\n' \
	        '$(bindir)'; \
	    printf '      Nothing was changed for you -- no startup file was edited.\n'; \
	    printf '      To add it yourself:  export PATH="%s:$$PATH"\n' '$(bindir)'; \
	elif [ "$$found" = "$$mine" ] || [ "$$found" -ef "$$mine" ]; then \
	    printf 'satl       on your PATH is this one\n'; \
	else \
	    printf 'note: `satl` still runs %s, not the copy just installed.\n' "$$found"; \
	    printf '      An earlier PATH entry shadows %s.\n' '$(bindir)'; \
	    printf '      If this shell has run satl already, run: hash -r\n'; \
	fi
# GTK resolves an icon name to the LAST base directory in its search path that
# holds it, and not the first -- measured on gtk4 4.16.7, in both directions:
# with the search path [A,B] the icon in B answers, with [B,A] the one in A
# does. That is the reverse of what the icon theme spec says, and it is the
# difference between an install that changes the picture on the screen and one
# that does not. $HOME/.local/share/icons comes FIRST in that path, so a copy of
# this artwork left behind in /usr/local/share/icons or /usr/share/icons -- both
# later -- goes on being drawn however many times this install refreshes ours.
#
# Reported, never repaired: those directories belong to root, and the note above
# `all` is that a build does not escalate. cmp rather than mtimes, because
# "different bytes" is the question and a timestamp answers a different one.
	@if [ -z "$(DESTDIR)" ]; then \
	    for base in /usr/local/share/icons /usr/share/icons; do \
	        [ -d "$$base" ] || continue; \
	        [ "$$base" = "$(datadir)/icons" ] && continue; \
	        stale=0; \
	        for f in `find dist/icons -type f -name '*.png' -printf '%P\n'`; do \
	            if [ -f "$$base/$$f" ] && \
	               ! cmp -s "dist/icons/$$f" "$$base/$$f"; then \
	                stale=`expr $$stale + 1`; \
	            fi; \
	        done; \
	        if [ "$$stale" -gt 0 ]; then \
	            other=`dirname "$$base"`; other=`dirname "$$other"`; \
	            printf 'note: %s holds %s satellite icons that are not these,\n' \
	                "$$base" "$$stale"; \
	            printf '      and GTK reads that directory AFTER %s.\n' \
	                '$(datadir)/icons'; \
	            printf '      The last one with an icon wins, so the desktop keeps drawing\n'; \
	            printf '      the old artwork. It belongs to root, so this build leaves it:\n'; \
	            printf '          sudo ./install.sh --prefix %s              refreshes it\n' \
	                "$$other"; \
	            printf '          sudo ./install.sh --uninstall --prefix %s  removes it\n' \
	                "$$other"; \
	        fi; \
	    done; \
	fi

# Symmetric with install, and the asymmetry in the commands is deliberate:
# share/satellite and share/doc/satellite are directories this install created
# and owns outright, so removing the tree is exact. Everything else lives in a
# directory shared with the rest of the system, where only the named files may
# go and the directory itself must stay.
uninstall:
	rm -f "$(DESTDIR)$(bindir)/satl" "$(DESTDIR)$(bindir)/satl-term"
# The icons are removed by the same walk that installed them, so the two lists
# cannot drift: hicolor is a directory shared with every other application on
# the system, so only the files this install named may go, and the size
# directories themselves must stay even when ours was the only icon in one.
	find dist/icons -type f -name '*.png' -printf '%P\n' | while read -r f; do \
	    rm -f "$(DESTDIR)$(datadir)/icons/$$f"; \
	done
	rm -f "$(DESTDIR)$(datadir)/mime/packages/application-x-satellite.xml"
	rm -f "$(DESTDIR)$(mandir)/man1/satl.1.gz" \
	      "$(DESTDIR)$(mandir)/man1/satl-term.1.gz"
	rm -f "$(DESTDIR)$(datadir)/applications/org.satellite.terminal.desktop"
	rm -f "$(DESTDIR)$(datadir)/bash-completion/completions/satl"
	rm -rf "$(DESTDIR)$(datadir)/satellite"
	rm -rf "$(DESTDIR)$(docdir)"
# Both indexes are rebuilt on the way out as well as on the way in. Without
# this the launcher stays in the shell's menu and .satl files keep an icon
# whose file is gone, which reads to a user as an uninstall that did not work.
	@if [ -z "$(DESTDIR)" ]; then \
	    command -v update-desktop-database >/dev/null 2>&1 && \
	        update-desktop-database "$(datadir)/applications" 2>/dev/null || true; \
	    command -v gtk-update-icon-cache >/dev/null 2>&1 && \
	        gtk-update-icon-cache -qtf "$(datadir)/icons/hicolor" 2>/dev/null || true; \
	    command -v update-mime-database >/dev/null 2>&1 && \
	        update-mime-database "$(datadir)/mime" 2>/dev/null || true; \
	fi

clean:
	$(MAKE) -C example/cxx_compare clean
	$(MAKE) -C example/py_compare clean
# example/website has had a clean target of its own all along and this file
# never called it, so `tour` and the two .out files it diffs survived every
# clean in the tree. Called now, for the reason the two above are.
	$(MAKE) -C example/website clean
# library_test_tsan is named outright rather than through $(TSAN_TEST), so that
# `make clean TSAN=0` still removes the 27MB binary an earlier build left.
	rm -f satl satl-term $(TESTBINS) $(TESTS)/library_test/library_test_tsan \
	      $(SRC)/*/*.o $(SRC)/*/*.o.tmp .libdir-stamp .cxxflags-stamp \
	      dist/satl.1.gz dist/satl-term.1.gz
# Three compiled things no rule in this file builds and no clean target had
# ever claimed. example/website/gtk_window is the one that matters: debian/
# rules deletes it during a package build with a note saying nothing else
# does, and an ELF binary under share/satellite/examples breaks Debian Policy
# 9.1.1's architecture-independence rule. The other two sit beside the sources
# they were compiled from. A build product that survives `make clean` is a
# build product that ends up in somebody's tarball.
	rm -f example/website/gtk_window pcg_test/pcg_test speed_test/cxx_speed
	rm -rf '$(DOWNLOAD_DIR)' '$(DOWNLOAD_TAR)'

.PHONY: all test compare python install install-report uninstall clean deps \
        bundle gui-skipped FORCE $(TESTALIASES)
