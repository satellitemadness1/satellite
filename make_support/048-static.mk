# satellite -- linking without the libraries the build machine happened to have.
#
# AFTER 047-window.mk, because what satl-term may do is not what satl may do and
# this fragment has to know whether there is a window at all; BEFORE
# 050-build.mk, which puts these variables on the four link lines. Numbered in
# eights for the same reason 047 is in sevens: it belongs where it is read.
#
# WHY THIS EXISTS. `satl` links libstdc++ and libgcc_s, and on the machine this
# was written on it found them at /home/madness/opt/gcc-17/lib64, because
# LD_RUN_PATH is exported in that environment and the linker turns LD_RUN_PATH
# into an RPATH inside the binary. In $HOME/.satl that is nobody's problem: the
# only account that runs the file is the one whose home it is. Copied into
# /usr/local by install.sh --system it becomes a program every account on the
# machine can find on PATH and exactly one of them can start, and the failure
# arrives as a loader error naming libstdc++.so.6 rather than the home
# directory it was being loaded out of. STATIC=full is the answer to that, and
# it is the reason the installer has a --static.
#
#     STATIC=0     link as before. What a bare `make` did until 2026-09-06.
#     STATIC=1     -static-libstdc++ -static-libgcc. Drops the two libraries
#                  that were coming out of a home directory; libc and libm stay
#                  dynamic, which is what every distribution wants and what
#                  every machine already has in /lib64.
#     STATIC=full  (default) -static. No dynamic dependencies at all: `ldd`
#                  answers "not a dynamic executable" and the file runs on a
#                  machine with no toolchain, no gcc and a different libc
#                  version.
#
# satl-term IS NEVER FULLY STATIC and STATIC=full does not make it one. It links
# gtk4 and vte, which pull in a long tail of shared libraries that dlopen their
# own modules -- input methods, image loaders, GIO extension points -- and a
# statically linked binary cannot dlopen the matching module against its own
# copies of them. So the window takes the STATIC=1 treatment under both
# settings, which is the half of the problem it actually has: its C++ runtime
# comes out of the same home directory satl's did, while gtk and vte come from
# /usr like the desktop it is drawing into. Silently different from the other
# three, and said out loud here because a reader who checks with `ldd` will find
# it and deserves to know it was meant.
# WHAT A STATIC satl COSTS, AND THE ONE HEADER THAT ENDS IT. Measured
# 2026-08-29 at M3, on this machine:
#
#     STATIC=full   satl   1,111,528 bytes
#     STATIC=1      satl     367,568 bytes
#
# THAT NUMBER SURVIVES ONLY WHILE NOTHING TOUCHES <iostream>, <fstream> OR
# <sstream>. satl prints with fputs and fprintf throughout, so the iostreams
# machinery -- locales, facets, and the static initialisation behind them --
# is absent from the link. M3 needed to read a source file and the obvious
# ifstream-plus-ostringstream reader put it back: STATIC=full went to
# 2,596,120 and STATIC=1 to 1,657,808, an increase of about 1.4 MB on every
# installed copy, to read a file into a string. It was replaced with fread
# (programs/main.cpp's read_file, which carries the same figures) and the
# whole of M3 then cost 32,696 bytes. programs/source_file.cpp carries the
# speed half of the same measurement, which pointed the other way.
#
# Said here rather than only there, because this is the file that decides satl
# is shipped statically, and the cost of that decision belongs beside it.

# ------------------------------------------------------------------------------
# THE DEFAULT WAS 0 UNTIL 2026-09-06 AND IS NOW full. The author's decision, in
# their own words: "Why build something without everything needed to run it
# included in it?"
# ------------------------------------------------------------------------------
#
# WHAT THE OLD DEFAULT SAID FOR ITSELF, kept because it was not wrong: "the
# right choice for a build you are going to run yourself out of your own home
# directory." A dynamic link is quicker, and a binary you only ever start from
# the directory it was built in has no portability problem to solve.
#
# WHAT DECIDED IT THE OTHER WAY, measured at M17 on this machine:
#
#                            static      dynamic
#     shared objects to map       0            4
#     peak resident memory  3,016 KB     4,368 KB
#     startup, absolute      0.908 ms     1.687 ms
#
# THE STATIC BUILD IS THE BIGGER FILE AND THE SMALLER PROCESS, which is the
# result that settles it. A dynamic satl maps the whole of libstdc++ to use a
# fraction of it; a static one carries only what the linker kept. It also starts
# 0.78 ms sooner, because there is no loader to run, nothing to relocate and no
# PLT to indirect through. Reproduce it with `/usr/bin/time -f %M` over the two
# builds -- the numbers above are best-of-five.
#
# AND IT REMOVES A TRAP IN THE MEASUREMENT ITSELF. make_support/startup.rows
# takes its baseline at STATIC=full, while `make startup` inherited this
# variable -- so the obvious command measured a build that was NOT comparable to
# the table it printed itself against, and the harness said so in a paragraph
# nobody reads until they have already been caught by it. M17 was caught by it.
# With the default at full the two agree, and the paragraph becomes a note about
# `make startup STATIC=0` rather than about the normal case.
#
# WHAT IT COSTS, STATED RATHER THAN DISCOVERED: `make` now REFUSES on a machine
# without glibc-static and libstdc++-static, with the error below naming the
# package. That is a real regression for a first checkout on a bare box and it
# was raised before the change was made. `make STATIC=0` is the one-word answer,
# and BOTH errors were edited to say so on the day the default moved -- they
# named the package and not the escape hatch, which is the wrong half to omit
# from a message that now greets a first build rather than a deliberate one. `make -s static-available` reports what a machine can
# do without attempting a link.
#
# THE INSTALLER IS UNAFFECTED AND ALWAYS WAS. install_support/050-building.sh
# resolves its own STATIC= and passes it explicitly, asking `make -s
# static-available` when it is set to auto -- so an install has been static
# since 2026-08-28 and this change does not touch it. What changed is only what
# a developer's bare `make` produces, which is now the same thing an install
# ships. Two things that were different for no reason a reader could see.
STATIC ?= full

# ASKED OF THE COMPILER, not looked for in a list of directories this file made
# up. -print-file-name answers with an absolute path when the library is there
# and echoes the name back unchanged when it is not, so the $(filter /%,...) is
# the whole test: a bare `libstdc++.a` is not an absolute path and drops out.
#
# Which toolchain answers is the compiler's business and not this file's. On
# this machine clang selects the system gcc 14 and finds both of these in
# /usr/lib/gcc/x86_64-redhat-linux/14, which is the point -- linking a static
# libstdc++ from one toolchain into objects compiled against another one's
# headers is exactly the kind of thing that works until it does not.
STATIC_LIBSTDCXX := $(filter /%,$(shell $(CXX) -print-file-name=libstdc++.a 2>/dev/null))
STATIC_LIBC      := $(filter /%,$(shell $(CXX) -print-file-name=libc.a 2>/dev/null))

ifneq ($(STATIC),0)

ifeq ($(STATIC_LIBSTDCXX),)
$(error STATIC=$(STATIC) needs a static libstdc++ and $(CXX) cannot find one. \
On AlmaLinux/RHEL: dnf --enablerepo=crb install libstdc++-static. \
On Debian/Ubuntu it is part of the g++ package and should already be there. \
Or build `make STATIC=0` for a dynamic satl that runs fine on this machine)
endif

# THE FLAGS THE THREE PLAIN BINARIES GET.
ifeq ($(STATIC),full)
ifeq ($(STATIC_LIBC),)
$(error STATIC=full needs a static libc and $(CXX) cannot find one. \
On AlmaLinux/RHEL: dnf install glibc-static. Or `make STATIC=0` for a \
dynamic build, or STATIC=1, which links \
the C++ runtime statically and leaves libc dynamic)
endif
STATIC_LDFLAGS = -static
else
STATIC_LDFLAGS = -static-libstdc++ -static-libgcc
endif

# And the window, which gets the milder treatment under both settings.
TERM_STATIC_LDFLAGS = -static-libstdc++ -static-libgcc

# LD_RUN_PATH IS CLEARED FOR THE LINK ITSELF, which is the other half of the
# job and the half that is easy to miss. Linking statically stops the binary
# LOADING anything out of a home directory; it does not stop the linker writing
# that home directory into the RPATH anyway, because LD_RUN_PATH is read from
# the environment and has nothing to do with which libraries were used. The
# result would be a fully static binary carrying a list of directories on
# somebody else's machine -- harmless, since nothing is ever loaded through it,
# and still a lie about the file that `readelf -d` would repeat to anybody who
# asked. Set empty for the duration of the recipe only: this changes no
# variable outside the link and does not touch the environment make was run in.
#
# `env -u` AND NOT `LD_RUN_PATH=`. Setting it empty leaves it SET-BUT-EMPTY, and
# GNU ld reads that as "an RPATH was requested, of nothing", which it duly
# writes: satl-term came out carrying a literal `RPATH []`. Harmless -- an empty
# search list finds nothing and the loader moves on -- and still a tag in the
# binary describing a decision nobody made. -u removes the variable, so the
# linker never sees it and emits no tag at all.
LINK_ENV = env -u LD_RUN_PATH

endif

# THE SAME GUARD 060-compile.mk PUTS ON THE COMPILES, FOR THE LINKS.
#
# make invalidates a target when a PREREQUISITE changes, and STATIC is not a
# prerequisite of anything -- so `make STATIC=full` over a tree built without it
# relinks NOTHING, and four dynamically linked binaries are installed by a run
# that said STATIC=full on its own transcript. Caught 2026-08-28 by installing
# to a prefix and then to a home root: the second install got the first one's
# static binaries, and nothing in either output said so.
#
# ONE STAMP AND NOT TWO, which is the opposite of the choice 060-compile.mk
# makes for .cxxflags-stamp and .cxxflags-stamp-haswell, so it is worth saying
# why. There the two stamps describe two DIFFERENT builds of the same sources
# that a user changes independently -- MARCH_HASWELL moves without CXXFLAGS
# moving. Here every variable in the string is set by the one STATIC switch, so
# they only ever change together, and two stamps would be two files that are
# always written in the same instant. The string holds all of them so that the
# file answers "how was this tree linked" completely.
#
# $(LINK_ENV) is in it because whether LD_RUN_PATH was cleared for the link is a
# property of the binary -- it decides whether an RPATH was written into it --
# in exactly the way 060's note says $(CXX) is a property of an object file.
.ldflags-stamp: FORCE
	@printf '%s' '$(LINK_ENV) $(LDFLAGS) $(STATIC_LDFLAGS) $(TERM_STATIC_LDFLAGS)' \
	    | cmp -s - $@ || \
	    printf '%s' '$(LINK_ENV) $(LDFLAGS) $(STATIC_LDFLAGS) $(TERM_STATIC_LDFLAGS)' > $@

# WHAT THIS MACHINE COULD LINK IF ASKED, printed for install.sh to read. The
# installer needs the answer to choose a default for --system and must not
# work it out for itself: which compiler is used is 010-compiler.mk's decision,
# and a second probe in a shell script is a second decision that can disagree
# with this one. `make -s static-available` answers full, 1 or no.
.PHONY: static-available
static-available:
	@if [ -n "$(STATIC_LIBSTDCXX)" ] && [ -n "$(STATIC_LIBC)" ]; then echo full; \
	 elif [ -n "$(STATIC_LIBSTDCXX)" ]; then echo 1; \
	 else echo no; fi

# Reported by `make` rather than left to be discovered with ldd, and only when
# it was asked for -- a default build says nothing.
.PHONY: static-note
static-note:
ifeq ($(STATIC),full)
	@echo "note: STATIC=full -- satl, satl.haswell and satl-cpu-level link no shared libraries."
ifeq ($(HAVE_WINDOW),yes)
	@echo "      satl-term links gtk4 and vte dynamically, as it must, with a static C++ runtime."
endif
else ifeq ($(STATIC),1)
	@echo "note: STATIC=1 -- the C++ runtime is linked in; libc and libm stay dynamic."
endif
