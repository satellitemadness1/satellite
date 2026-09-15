# satellite -- linking without the libraries the build machine happened to have.
#
# THE PARENT'S 048-static.mk, ANSWERED FOR THIS FAMILY. Same three settings,
# same argument, different packages -- and one measurement the parent could not
# have taken, at the bottom.
#
# AFTER 020-inherited.mk, because what satl-term may do is not what satl may do
# and this fragment has to know whether there is a window at all; AFTER
# 030-output.mk, which names $(LDFLAGS_STAMP); BEFORE 050-build.mk, which puts
# these variables on the four link lines.
#
# WHY THIS EXISTS, in the parent's words and unchanged here: satl links
# libstdc++ and libgcc_s, and the linker turns LD_RUN_PATH into an RPATH inside
# the binary. In a home directory that is nobody's problem. Copied somewhere
# every account can reach, it becomes a program every account can find on PATH
# and exactly one of them can start, and the failure arrives as a loader error
# naming libstdc++.so.6 rather than the home directory it was loaded out of.
#
#     STATIC=0     (default) link as before. The right choice for a build you
#                  are going to run yourself out of this directory.
#     STATIC=1     -static-libstdc++ -static-libgcc. Drops the two libraries
#                  that were coming out of a home directory; libc and libm stay
#                  dynamic, which is what every distribution wants.
#     STATIC=full  -static. No dynamic dependencies at all: `ldd` answers "not
#                  a dynamic executable" and the file runs on a machine with no
#                  toolchain and a different libc version.
#
# satl-term IS NEVER FULLY STATIC and STATIC=full does not make it one. It links
# gtk4 and vte, which dlopen their own modules -- input methods, image loaders,
# GIO extension points -- and a statically linked binary cannot dlopen a module
# against its own copies of them. So the window takes the STATIC=1 treatment
# under both settings: its C++ runtime comes out of the same place satl's did,
# while gtk and vte come from /usr like the desktop it is drawing into.
STATIC ?= 0

# ASKED OF THE COMPILER, not looked for in a list of directories this file made
# up. -print-file-name answers with an absolute path when the library is there
# and echoes the name back unchanged when it is not, so $(filter /%,...) is the
# whole test.
#
# Measured on Debian 13 with clang 19 as `c++`: both answers come out of the
# system gcc 14 -- /usr/lib/gcc/x86_64-linux-gnu/14/libstdc++.a and
# /lib/x86_64-linux-gnu/libc.a -- which is the point. Which toolchain answers is
# the compiler's business and not this file's, and linking one toolchain's
# static libstdc++ into objects compiled against another's headers is exactly
# the kind of thing that works until it does not.
STATIC_LIBSTDCXX := $(filter /%,$(shell $(CXX) -print-file-name=libstdc++.a 2>/dev/null))
STATIC_LIBC      := $(filter /%,$(shell $(CXX) -print-file-name=libc.a 2>/dev/null))

ifneq ($(STATIC),0)

# THE PACKAGE NAMED IS build-essential BOTH TIMES, and that is the whole reason
# this fragment is not the parent's. On AlmaLinux the two static libraries are
# separate packages and one of them is in a repository that is off by default,
# so 048-static.mk has to say `dnf --enablerepo=crb install libstdc++-static`
# and `dnf install glibc-static`. Here libstdc++.a ships inside g++ and libc.a
# inside libc6-dev, both of which build-essential depends on -- so a machine
# that can compile this at all can almost always link it statically too, and
# the message says what to do in the case where somebody installed a bare
# compiler instead.
ifeq ($(STATIC_LIBSTDCXX),)
$(error STATIC=$(STATIC) needs a static libstdc++ and $(CXX) cannot find one. \
On Debian and Ubuntu it ships inside the g++ package: sudo apt install \
build-essential. If $(CXX) is a clang from outside the distribution, it is \
still the system g++ that supplies this file)
endif

ifeq ($(STATIC),full)
ifeq ($(STATIC_LIBC),)
$(error STATIC=full needs a static libc and $(CXX) cannot find one. \
On Debian and Ubuntu it ships inside libc6-dev: sudo apt install \
build-essential. Or use STATIC=1, which links the C++ runtime statically \
and leaves libc dynamic)
endif
STATIC_LDFLAGS = -static
else
STATIC_LDFLAGS = -static-libstdc++ -static-libgcc
endif

# And the window, which gets the milder treatment under both settings.
TERM_STATIC_LDFLAGS = -static-libstdc++ -static-libgcc

# LD_RUN_PATH IS CLEARED FOR THE LINK ITSELF, which is the half that is easy to
# miss. Linking statically stops the binary LOADING anything out of a home
# directory; it does not stop the linker WRITING that home directory into the
# RPATH anyway, because LD_RUN_PATH is read from the environment and has nothing
# to do with which libraries were used. The result would be a fully static
# binary carrying a list of directories on somebody else's machine -- harmless,
# since nothing is ever loaded through it, and still a lie that `readelf -d`
# would repeat to anybody who asked.
#
# `env -u` AND NOT `LD_RUN_PATH=`. Setting it empty leaves it SET-BUT-EMPTY, and
# GNU ld reads that as "an RPATH was requested, of nothing", which it duly
# writes: the parent's satl-term came out carrying a literal `RPATH []`. -u
# removes the variable, so the linker never sees it and emits no tag at all.
LINK_ENV = env -u LD_RUN_PATH

endif

# NO -no-pie ANYWHERE, AND THAT IS A MEASUREMENT RATHER THAN AN OMISSION.
#
# Every g++ on this family is built --enable-default-pie, and `-static` with a
# default-PIE compiler is a documented trap elsewhere: the two ask for opposite
# things and some toolchains make you say -no-pie or -static-pie to break the
# tie. This one does not. Measured on Debian 13 with g++ 14.2.0, 2026-08-28:
# `g++ -O2 -static` produced `ELF 64-bit LSB executable ... statically linked`
# which ran, and adding Ubuntu's hardening `-Wl,-z,relro,-z,now` changed
# nothing. clang 19 gave the same answer.
#
# Written down because the absence of a flag is invisible, and the next person
# to read this file under STATIC=full deserves to know the question was asked.

# THE SAME GUARD 060-compile.mk PUTS ON THE COMPILES, FOR THE LINKS.
#
# make invalidates a target when a PREREQUISITE changes, and STATIC is not a
# prerequisite of anything -- so `make STATIC=full` over a tree built without it
# relinks NOTHING, and four dynamically linked binaries come out of a run that
# said STATIC=full on its own transcript. The parent caught this on 2026-08-28
# by installing to a prefix and then to a home root: the second install got the
# first one's static binaries, and nothing in either output said so.
#
# ONE STAMP AND NOT TWO, unlike the two flag stamps in 060-compile.mk. There the
# two describe different builds of the same sources that a user changes
# independently. Here every variable in the string is set by the one STATIC
# switch, so they only ever change together.
#
# $(LINK_ENV) is in it because whether LD_RUN_PATH was cleared for the link
# decides whether an RPATH was written into the binary, which is a property of
# the binary in exactly the way $(CXX) is a property of an object file.
$(LDFLAGS_STAMP): FORCE | $(BUILD)
	@printf '%s' '$(LINK_ENV) $(LDFLAGS) $(STATIC_LDFLAGS) $(TERM_STATIC_LDFLAGS)' \
	    | cmp -s - $@ || \
	    printf '%s' '$(LINK_ENV) $(LDFLAGS) $(STATIC_LDFLAGS) $(TERM_STATIC_LDFLAGS)' > $@

# WHAT THIS MACHINE COULD LINK IF ASKED, printed for a script to read. Kept to
# the parent's name and its three answers -- full, 1 or no -- so that anything
# written against `make -s static-available` works in either directory. An
# installer must not work this out for itself: which compiler is used is
# 010-compiler.mk's decision, and a second probe in a shell script is a second
# decision that can disagree with this one.
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
