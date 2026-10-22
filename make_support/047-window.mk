# satellite 004 -- the window satl draws, and the console in it.
#
# satl-term WAS BUILT HERE UNTIL 2026-09-22, and is gone: the author, that day,
# "we are getting rid of satl-term and replacing it with something built in to
# the satl exe". satl opens its own console (GTK-17, console_launch.cpp) -- when
# it is asked with --console, and when nobody gave it one -- and satl-term's
# window, menu, keys and end-of-run policy are in it. What is below is satl's.
#
# vte-2.91-gtk4 AND NOT vte-2.91, which is the GTK3 build of the same library. On
# AlmaLinux the package is vte291-gtk4-devel, in CRB; the vendored stack builds
# its own. Linux only: VTE is a Linux terminal widget.

# ---------------------------------------------------------------------------
# AND NOW satl ITSELF DRAWS -- satellite.window, 2026-09-20 (SATELLITE_WINDOW.md
# WIN-3) -- and since GTK-17 (2026-09-22) its console too, which is VTE.
# ---------------------------------------------------------------------------
#
# THE WINDOW AND THE CONSOLE ARE ASKED SEPARATELY, on purpose: HAVE_GTK and
# HAVE_CONSOLE. A machine can have gtk4 and no vte -- the ordinary case outside
# a desktop distribution's CRB repo -- and there satl draws its windows and
# refuses the console words by name. One flag would have made every window
# depend on a terminal library most of them never call.
#
# satl STILL BUILDS WITH NEITHER, which is 047's oldest rule: "a Makefile that
# dies there has made the interpreter unbuildable to deliver a window". With no
# gtk4, SATELLITE_HAS_WINDOW is 0, the two window sources are not compiled, and
# bytecode/window_calls.cpp -- which IS always compiled -- refuses the word with
# a sentence naming the package to install.
# ---------------------------------------------------------------------------
# `make GTK=vendor` CARRIES GTK INSIDE satl. `make` (GTK=system) does not.
# ---------------------------------------------------------------------------
#
# TWO BINARIES AT ONE PATH, AND THE SIZE IS HOW YOU TELL THEM APART:
#
#     make              ~1 MB    GTK loaded from the machine at run time
#     make GTK=vendor  ~87 MB    GTK compiled IN; 6 NEEDED entries, no GTK stack
#
# Both write build/satl, so the author's `satl` alias runs whichever was built
# last. The link prints which kind it made, because a 1 MB satl and an 87 MB satl
# behave identically on THIS machine and differently on every other one -- that
# is exactly the confusion worth spending a line of output to prevent.
#
# FLIPPED TO vendor, 2026-09-21, which is what the line below this one told its
# own future to do: "Flip this default to vendor the day WIN-1 lands". WIN-1
# landed (`1f36e95`) and the whole stack was rebuilt from vendor/new/ the night
# of 2026-09-20, so the reason `system` was the default is gone. What was
# measured before flipping, on the vendored binary and not on hello:
#
#     readelf -d              the seven allowed, and nothing else
#     check.sh                353 passed, 0 failed
#     examples/window.satl    opens a window under headless mutter
#     LD_DEBUG=libs           no GTK-stack library loaded from /usr at run time
#
# WHAT THIS CHANGES FOR SOMEBODY RUNNING PLAIN `make`: build/satl becomes ~52 MB
# instead of ~1 MB, and it stops needing gtk4 installed to open a window. It also
# needs vendor/stage to exist -- `/usr/bin/python3 vendor/build_stack.py`, about
# three minutes -- and HAVE_GTK is `no` without it, which builds an interpreter
# that refuses the window words by name rather than failing. That is 047's oldest
# rule holding: a Makefile that dies for want of a window has made the
# interpreter unbuildable to deliver one.
# SINCE 2026-10-08 A PLAIN make BUILDS vendor/stage ITSELF when it is missing, and
# STOPS when the machine cannot: the author's rulings since -- always GTK and VTE, a
# monitor always -- overrule that oldest rule for the plain make. The block just
# before HAVE_GTK, below, says how.
#
# `make GTK=system` is still there and still works. It is the right build for a
# machine that HAS gtk4 and wants the 1 MB binary.
GTK ?= vendor

GTK_PKGS  = gtk4

# THE NEW STACK, 2026-09-20. Built by `vendor/build_stack.py` -- 24 projects from
# the frozen tarballs in vendor/new/, bottom-up into vendor/stage, 172 seconds.
# This replaced vendor/gtk-old/build-static, which was GTK 4.16.7 and its
# subprojects and took an hour.
#
# THE SHAPE IS DIFFERENT, AND THAT IS WHY THREE LINES BELOW CHANGED TOO. In the old
# build every dependency was a meson SUBPROJECT inside build-static/, so one `find`
# over one directory gathered all 63 archives and one meson-uninstalled directory
# answered every pkg-config question. Now GTK is built ALONE against an install
# prefix: its build tree holds seven archives and vendor/stage holds the other 32.
#
# vendor/gtk-old is kept until the vendored satl is proven end to end, and
# GTK_AND_NO_DEPENDENCIES.md Part 0 describes it -- deleting it makes that untrue.
GTK_BUILD = $(CURDIR)/vendor/build/gtk
GTK_STAGE = $(CURDIR)/vendor/stage

# THE STACK'S CLANG: ~/opt's, else the clang on PATH -- the same order 010-compiler.mk
# chooses satl's compiler in, so a clang in /usr/local or a loaded module is found too.
# ITS FOLDER AS CLANG ITSELF SAYS IT (InstalledDir, which 045-optimise.mk asks clang++
# for too), not as the path is spelled: EPEL's ccache puts a /usr/lib64/ccache/clang that
# is a link to ccache first on PATH, and no folder can be read off that name (the review,
# 2026-10-08). None at all, and build_stack.py names clang among what the machine lacks.
VENDOR_CLANG = $(firstword $(wildcard $(LLVM_BIN)/clang) $(shell command -v clang 2>/dev/null))
VENDOR_CLANG_DIR = $(if $(VENDOR_CLANG),$(or $(patsubst %/bin,%,$(shell $(VENDOR_CLANG) -v 2>&1 | sed -n 's/^InstalledDir: //p')),$(patsubst %/bin/clang,%,$(VENDOR_CLANG))))
BUILD_CARRIED_GTK = /usr/bin/python3 -u vendor/build_stack.py $(if $(VENDOR_CLANG_DIR),--clang $(VENDOR_CLANG_DIR))

ifeq ($(GTK),vendor)

# THE VENDORED STACK. Its pkg-config answers entirely out of vendor/ -- measured
# 2026-09-20: `--cflags gtk4` through meson-uninstalled returns ZERO -I/usr paths,
# which is what makes uninstalling the system GTK unnecessary. The build simply
# never asks it.
# PKG_CONFIG_LIBDIR, **NOT** PKG_CONFIG_PATH, and this is not a tidy-up -- the old
# line is measurably wrong against the new stack. PKG_CONFIG_PATH only PREPENDS to
# pkg-config's built-in path, so /usr/lib64/pkgconfig stays visible. Measured
# 2026-09-20 with exactly the old line and the new build:
#
#     exit=1
#     Package 'pango' has version '1.54.0', required version is '>= 1.58'
#     Package 'gio-2.0' has version '2.80.4', required version is '>= 2.89.3'
#
# -- it walked straight into the SYSTEM's pango and glib. It failed only because
# GTK 4.24's floors happen to be higher than what this machine has installed; with
# lower floors it would have succeeded against system headers and said nothing. And
# `$(shell ...)` discards the exit status, so GTK_CFLAGS would simply have been
# EMPTY and the compile would have failed hundreds of lines later, pointing nowhere
# near here.
#
# The uninstalled directory answers for gtk4 itself; the stage answers for
# everything under it; pkgconfig-system holds the four .pc files no vendored
# project produces (wayland-client, wayland-egl, wayland-scanner, libdrm).
GTK_PC_LIBDIR = $(GTK_BUILD)/meson-uninstalled:$(GTK_STAGE)/lib/pkgconfig:$(GTK_STAGE)/lib64/pkgconfig:$(GTK_STAGE)/share/pkgconfig:$(GTK_STAGE)/pkgconfig-system

# A PLAIN make BUILDS THE CARRIED GTK FIRST WHEN THIS CHECKOUT HAS NONE (the author, 2026-10-08:
# "we need an extraction program built into make so plain make runs" -- a copy from the server
# holds the tarballs in vendor/new/ and no vendor/stage, so its plain make took the no-window path,
# which stopped at the link that day, and makes a satl that since 2026-10-05 runs nothing typed in a
# terminal). It is `make window`, run HERE, before HAVE_GTK is asked, so this same make goes on to
# build satl with its window. vendor/build_stack.py checks what the machine lacks -- all of it at
# once, with the dnf line -- then builds the 29 projects from their source in vendor/<project>/,
# which is in git since 2026-10-10 ("no zipped vendor stuff", the author; a tree that is missing is
# unpacked from its tarball in vendor/new/ with Python's own tarfile, where the machine still has
# it): about five minutes, ONCE (a re-run skips what is built). Its progress
# reaches the terminal through stderr, because $(shell ...) keeps stdout; the whole logs are in
# vendor/build_logs/. FOR THE GOALS THAT BUILD `all` -- none, all, install, check, test, cpus,
# race -- in the make that was typed: never -n, -q or -t, never a make inside a make (the one
# install.sh runs from a make's recipe, the training build's), never one processor's. install.sh
# run by hand builds it too, which is right: its make is the one that was typed.
# A MACHINE THAT CANNOT BUILD IT STOPS HERE -- satellite is always built with GTK and VTE (the
# author, 2026-10-05 and 2026-10-07), so a satl without its window is not a build worth finishing;
# and so does a build that says it finished while either archive is missing (a project marked
# built whose files were deleted -- the review). `make GTK=system` builds against the system's GTK
# instead. A target named by itself (make build/satl) does not start this build; with no carried GTK
# it reaches the old no-window path, which does not link -- and is not a build at all (the author,
# 2026-10-09: "there is no no-window build").
CARRIED_GTK_IS_BUILT = $(and $(wildcard $(GTK_BUILD)/gtk/libgtk.a),$(wildcard $(GTK_STAGE)/lib/libvte-2.91-gtk4.a))
ifeq ($(CARRIED_GTK_IS_BUILT),)
ifeq ($(MAKELEVEL)$(CPU)$(PGO_STAGE)$(filter-out all install check test cpus race,$(MAKECMDGOALS))$(findstring n,$(firstword -$(MAKEFLAGS)))$(findstring q,$(firstword -$(MAKEFLAGS)))$(findstring t,$(firstword -$(MAKEFLAGS))),0)
$(info the carried GTK is not built in this checkout -- building it first, once: vendor/build_stack.py, about five minutes)
CARRIED_GTK_STATUS := $(shell $(BUILD_CARRIED_GTK) --quiet >&2; echo $$?)
ifneq ($(CARRIED_GTK_STATUS),0)
$(error the carried GTK was not built -- vendor/build_stack.py says why, above -- so satl was not built either)
endif
ifeq ($(CARRIED_GTK_IS_BUILT),)
$(error vendor/build_stack.py finished, but libgtk.a or libvte-2.91-gtk4.a is not there -- a project marked built whose files are gone; `/usr/bin/python3 vendor/build_stack.py --force --only gtk,vte` builds them again)
endif
endif
endif

HAVE_GTK   := $(shell [ -f $(GTK_BUILD)/gtk/libgtk.a ] && echo yes || echo no)

# AND THE CONSOLE (GTK-17, 2026-09-22). VTE has been in the stage since the
# same day, its archive -- with lz4's and simdutf's -- is already among
# GTK_ARCHIVES below, and satl's link group pulls in whatever the console
# references: all a console costs the build is VTE's include path, which is
# what adding it to GTK_PKGS buys. ASKED SEPARATELY from HAVE_GTK, for 047's
# oldest reason: a stage built without VTE still makes a satl that draws, and
# that satl refuses the console words by name (window_console.cpp's other
# half) rather than failing to build.
HAVE_CONSOLE := $(shell [ -f $(GTK_STAGE)/lib/libvte-2.91-gtk4.a ] && echo yes || echo no)
ifeq ($(HAVE_CONSOLE),yes)
GTK_PKGS = gtk4 vte-2.91-gtk4
endif

# THE STACK'S .pc FILES REMEMBER THE FOLDER THEY WERE BUILT IN, AND THIS CHECKOUT NEED NOT BE THERE
# (2026-10-15). build_stack.py's prefixes are absolute -- prefix=/home/madness/code/cxx/satellite/
# vendor/stage -- and so are GTK's meson-uninstalled files, which name its source folder too. The tree
# built on AlmaLinux was unpacked on Debian 13 at .../last_satellite/satellite, pkg-config answered
# with 24 folders that were not there, and the window sources stopped at `fatal error: 'gtk/gtk.h'
# file not found` (window_canvas.hpp at 'cairo.h') -- an error that reads like an order to install
# GTK, about a build that carries its own.
#
# SO A FOLDER UNDER A ROOT THE STACK RECORDED IS SAID AGAIN UNDER THIS CHECKOUT (the author, that
# day: "we have to use relative paths because this may be compiled on another machine"). The roots
# are read off the prefix= lines -- <root>/vendor/stage and <root>/vendor/build/gtk, one root unless
# part of the stack was built somewhere else -- and -I<root>/vendor/ becomes -I$(CURDIR)/vendor/,
# which make works out again on every run, wherever the checkout is: GTK_BUILD, GTK_STAGE and the
# archives below were always made that way, and only answers read back from files could go stale.
# $(CURDIR) AND NOT A BARE vendor/, though make runs from this folder: clang's .d files write a
# header the way its -I spelled it, and check.sh's row "every header the compiler reads is in the
# build fingerprint" asks every header with a relative path to be one of BUILD_INPUTS -- with
# -Ivendor/, 770 of the carried GTK's were, and the row failed (the review, 2026-10-15). Absolute,
# they stay outside it, as they always were (040-sources.mk keeps PCG's out with -isystem instead).
#
# NOT pkg-config --define-prefix, which rewrites prefix= alone: ten of the stage's .pc files spell
# includedir or libdir out in full (zlib, libpng, libffi, libjpeg, libtiff, expat, pcre2, fmt -- the
# cmake and autotools ones), and no prefix describes GTK's source folder.
GTK_PC_FILES := $(wildcard $(GTK_BUILD)/meson-uninstalled/*.pc $(GTK_STAGE)/lib/pkgconfig/*.pc \
                           $(GTK_STAGE)/lib64/pkgconfig/*.pc $(GTK_STAGE)/share/pkgconfig/*.pc)
GTK_RECORDED_ROOTS := $(if $(GTK_PC_FILES),$(sort $(shell sed -n -e 's,^prefix=\(/.*\)/vendor/stage/*$$,\1,p' \
                          -e 's,^prefix=\(/.*\)/vendor/build/gtk/*$$,\1,p' $(GTK_PC_FILES))))
# $(call gtk_reroot,FLAGS,ROOTS): every -I<root>/vendor/ in FLAGS as -I$(CURDIR)/vendor/, one root
# at a time.
gtk_reroot = $(if $(2),$(call gtk_reroot,$(subst -I$(firstword $(2))/vendor/,-I$(CURDIR)/vendor/,$(1)),$(wordlist 2,$(words $(2)),$(2))),$(1))
GTK_CFLAGS := $(call gtk_reroot,$(shell env -u PKG_CONFIG_PATH PKG_CONFIG_LIBDIR=$(GTK_PC_LIBDIR) pkg-config --cflags $(GTK_PKGS) 2>/dev/null),$(GTK_RECORDED_ROOTS))

# EVERY ARCHIVE, FROM TWO PLACES NOW -- GTK's own build tree (7) and the install
# prefix everything below it was staged into (32). The old build found all 63 under
# one directory because they were meson subprojects; these are separate builds, so
# this searches both. libgtk.a is excluded here and named FIRST in GTK_LIBS below,
# so its undefined symbols drive the rest, and the whole lot sits in a --start-group
# because the graph has cycles.
#
# FIVE EXCLUDED BY NAME. libmalloc-stats.a DEFINES malloc/realloc; libcairo-trace.a
# and libcairo-fdr.a are LD_PRELOAD interposers that redefine cairo_*; libdemo.a is
# pixman's demo; libintl.a is a STUB gettext that collides with glibc's own
# _nl_msg_cat_cntr. Only libcairo-trace.a is actually present in the new stage --
# cairo builds it unconditionally, there is no option for it (CAIRO_HAS_TRACE is set
# whenever the OS can LD_PRELOAD) -- but the other four stay listed, because the
# cost of a name that matches nothing is zero and the cost of rediscovering why
# libintl.a breaks a link is an evening.
#
# The test is the NAME, not the directory: cairo keeps two REAL libraries under the
# same util/ that GSK needs, so excluding util/ wholesale breaks the link instead.
GTK_ARCHIVES := $(shell find $(GTK_BUILD) $(GTK_STAGE) -name '*.a' ! -name 'libgtk.a' 2>/dev/null | \
                        grep -vE '/(libmalloc-stats|libcairo-trace|libcairo-fdr|libdemo|libintl)\.a$$' | sort)

# -static-libstdc++ IS NOT USED, AND THE REASON IS A MEASUREMENT (2026-09-20).
#
# It links, and it takes the last two entries off NEEDED, and 68 test programs
# come out byte-identical -- so it LOOKED right. Then check.sh's /dev/full row
# failed: writing to a full device answered 0 instead of display_error (2). The
# same objects linked WITHOUT the flag answer 2.
#
# WHY: every library in build/satellite-numbers/ has NEEDED libstdc++.so.6. With
# satl carrying its own copy there are TWO std::cout in one process --
# satellite.console.display writes through the shared one, and satl checks the
# error state of its own, which never saw the failure. A write that failed is
# reported as a run that succeeded, which is "an answer that is wrong and does
# not say so".
#
# 048-link.mk SAID THIS BEFORE ANY OF IT WAS BUILT: "satl and every library in
# build/satellite-numbers/ must share one libstdc++, or each has its own
# std::cout (DESIGN 3.4)". It was written about STATIC=full in 003 and it is
# exactly as true here.
#
# AND IT GENERALISES PART 1's RULE. That rule read "a library whose objects cross
# into a dlopened DRIVER cannot be static", learned from libwayland-client and the
# GPU driver. libstdc++ is the same shape with a different boundary: our own
# dlopened word libraries. The rule is really **a library whose state is shared
# across a dlopen boundary cannot be static** -- and satl dlopens 62 things.
#
# SO THE WAY TO SIX IS NOT THIS FLAG. It is to stop dlopening the word libraries
# and link them into satl (SATELLITE_WINDOW.md WIN-6 shape (i)), which is the
# author's decision and a real change to DESIGN 3.4, not a link flag.
GTK_LINK_FLAGS =

# -lwayland-client AND -lwayland-egl STAY SHARED, AND THAT IS NOT A COMPROMISE.
# Linked statically there are two copies in one process -- ours and the one the GPU
# driver dlopens -- and GDK hands EGL a wl_display whose lists the driver's copy
# never initialised. SIGSEGV, measured, in BOTH link modes. A library whose objects
# cross into a dlopened driver cannot be static. -ldl is the honest other half:
# libepoxy dlopens libGL/libEGL by design, because the driver belongs to the
# machine's graphics card and not to satellite.
#
# NO -lresolv, SINCE 2026-09-22 (GTK_AND_NO_DEPENDENCIES.md DEP-4). It was here
# from the first vendored link and made libresolv.so.2 the eighth NEEDED entry --
# and satl never took one symbol from it: glibc 2.34 moved res_nquery, dn_expand,
# __res_ninit and __res_nclose (the four gio's threaded resolver names) into
# libc.so.6, satl's floor is glibc 2.38, `readelf -V` had no version-needs for
# libresolv at all, and gio's own .pc never asked for it (gio/meson.build adds
# -lresolv only where res_query does NOT link plainly). A -l with no --as-needed
# is a NEEDED entry whether or not anything binds to it. Seven now.
# -lpthread and -lrt are the same shape and cost nothing: glibc 2.34+ has no
# separate .so for either, so they add no NEEDED entry; they stay for the day a
# link is tried against an older glibc, where they would be the honest answer.
#
# ONLY WHEN THE STACK IS BUILT (a fresh clone, 2026-09-23: `ld: cannot find
# vendor/build/gtk/gtk/libgtk.a`). HAVE_GTK above was already `no` and the window
# sources already left out, but the link still named the archive, so a checkout
# without vendor/stage could never link at all -- the opposite of 047's oldest rule.
ifeq ($(HAVE_GTK),yes)
GTK_LIBS = -Wl,--start-group $(GTK_BUILD)/gtk/libgtk.a $(GTK_ARCHIVES) -Wl,--end-group \
           -lm -lpthread -lrt -lwayland-client -lwayland-egl

GTK_KIND = vendored (GTK carried inside satl)
else
GTK_LIBS =
GTK_KIND = no window (vendor/stage is not built -- make window)
endif

else

# THE SYSTEM'S VTE IS THE CONSOLE'S, when the system has one; its .pc names
# gtk4 too.
HAVE_CONSOLE := $(shell pkg-config --exists vte-2.91-gtk4 2>/dev/null && echo yes || echo no)
ifeq ($(HAVE_CONSOLE),yes)
GTK_PKGS = gtk4 vte-2.91-gtk4
endif
HAVE_GTK       := $(shell pkg-config --exists $(GTK_PKGS) 2>/dev/null && echo yes || echo no)
GTK_CFLAGS     := $(shell pkg-config --cflags $(GTK_PKGS) 2>/dev/null)
GTK_LIBS       := $(shell pkg-config --libs $(GTK_PKGS) 2>/dev/null)
GTK_LINK_FLAGS =
GTK_KIND       = system (GTK loaded from this machine at run time)

endif

ifeq ($(HAVE_GTK),yes)
  WINDOW_DEFINE = -DSATELLITE_HAS_WINDOW=1
else
  WINDOW_DEFINE = -DSATELLITE_HAS_WINDOW=0
endif

# READ BY THE WINDOW FOLDER'S OBJECTS ONLY (060-compile.mk): window_console.cpp
# and console_launch.cpp are compiled wherever GTK is, and this is what picks
# their VTE half or their refusing half. The bytecode files never see it.
ifeq ($(HAVE_CONSOLE),yes)
  CONSOLE_DEFINE = -DSATELLITE_HAS_CONSOLE=1
else
  CONSOLE_DEFINE = -DSATELLITE_HAS_CONSOLE=0
endif

# THE WINDOW SOURCES satl LINKS IN: the desk that owns the one GTK thread, what a program can do to a
# window, and the pieces that go inside one (window_pieces.cpp, split out at
# GTK-1 for the line rule). Empty when there is no gtk4, which is what makes satl buildable
# without one. bytecode/window_calls.cpp is NOT here -- it is in
# INTERPRETER_SOURCES and compiled always, because it is the file that says
# this satl has no window.
ifeq ($(HAVE_GTK),yes)
GTK_SOURCES = $(SATELLITE)/satellite_variable_window/window_desk.cpp \
              $(SATELLITE)/satellite_variable_window/satellite_window.cpp \
              $(SATELLITE)/satellite_variable_window/window_open.cpp \
              $(SATELLITE)/satellite_variable_window/window_pieces.cpp \
              $(SATELLITE)/satellite_variable_window/window_asks.cpp \
              $(SATELLITE)/satellite_variable_window/window_state.cpp \
              $(SATELLITE)/satellite_variable_window/window_answers.cpp \
              $(SATELLITE)/satellite_variable_window/window_look.cpp \
              $(SATELLITE)/satellite_variable_window/window_asking.cpp \
              $(SATELLITE)/satellite_variable_window/window_menu.cpp \
              $(SATELLITE)/satellite_variable_window/window_menu_bar.cpp \
              $(SATELLITE)/satellite_variable_window/window_canvas.cpp \
              $(SATELLITE)/satellite_variable_window/window_strokes.cpp \
              $(SATELLITE)/satellite_variable_window/window_spill.cpp \
              $(SATELLITE)/satellite_variable_window/window_widgets.cpp \
              $(SATELLITE)/satellite_variable_window/window_console.cpp \
              $(SATELLITE)/satellite_variable_window/console_launch.cpp \
              $(SATELLITE)/satellite_variable_window/console_feed.cpp \
              $(SATELLITE)/satellite_variable_window/console_menu.cpp \
              $(SATELLITE)/satellite_variable_window/console_settings.cpp \
              $(SATELLITE)/satellite_variable_window/console_shadow.cpp \
              $(SATELLITE)/satellite_variable_window/console_status.cpp
# AND THE ONE WAYLAND TABLE THE CARRIED GTK NEEDS AND WAYLAND 1.23 LACKS (2026-10-15):
# window_wayland_fixes.cpp says why. The carried GTK's only -- the system's GTK and the
# system's libwayland agree already.
ifeq ($(GTK),vendor)
GTK_SOURCES += $(SATELLITE)/satellite_variable_window/window_wayland_fixes.cpp
endif
else
GTK_SOURCES =
endif

GTK_OBJECTS = $(GTK_SOURCES:%.cpp=$(OBJECTS)/%.o)

# THE CARRIED DATA (WIN-1), generated rather than written: xkeyboard-config, the
# IBM Plex Mono family, satl's own fonts.conf and GTK's compiled schemas, all as
# one compressed GResource in .rodata. make_window_data.py says why each is
# fatal without it. It is a C file, so it compiles with the plain rule and needs
# no GTK include path of its own -- only glib's, which GTK_CFLAGS already has.
WINDOW_DATA_SOURCE = $(BUILD)/generated/window_data.c
WINDOW_DATA_OBJECT = $(OBJECTS)/generated/window_data.o
WINDOW_DATA_INPUTS = $(SATELLITE)/satellite_variable_window/make_window_data.py \
                     $(SATELLITE)/satellite_variable_window/fonts.conf \
                     $(wildcard vendor/fonts/ibm-plex-mono/*.ttf) \
                     $(wildcard $(GTK_STAGE)/share/xkeyboard-config-2/rules/*)

ifeq ($(HAVE_GTK),yes)
GTK_OBJECTS += $(WINDOW_DATA_OBJECT)
endif

# `make window` BUILDS THE GTK satl CARRIES, from the source in vendor/<project>/ (in git since
# 2026-10-10), into vendor/stage and vendor/build/gtk -- neither is in git, nor in the server's copies.
# SINCE 2026-10-08 A PLAIN `make` RUNS THIS SAME BUILD ITSELF when the checkout has none
# (the block before HAVE_GTK); this is it by name, every line on the terminal, and a
# `make` after it builds satl with it. vendor/build_stack.py first checks what the machine
# needs -- a clang, ninja, cmake, bison, perl, make, pkg-config, patch, nasm
# (/usr/local/bin/nasm first, else the one on PATH; AlmaLinux has it and ninja in CRB), the
# three wayland .pc files where the system's own pkg-config keeps them (wayland-devel; 1.24
# or newer, and the wayland-scanner they name), and since 2026-10-09 the system headers the
# stack reads with no .pc to ask for them (EGL and KHR: libglvnd-devel; drm and linux:
# kernel-headers) -- and names ALL it lacks, with one install line for the machine's family
# (the dnf line on AlmaLinux, as before); then it builds, unpacking first only a tree that is missing. VENDOR_CLANG and BUILD_CARRIED_GTK are with GTK_STAGE, above.
.PHONY: window
window:
	$(BUILD_CARRIED_GTK)
	@echo "the carried GTK is built -- now run: make"
