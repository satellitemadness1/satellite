# satellite 004 -- satl-term, and whether this machine can build it.
#
# BEFORE 050-build.mk, which tests HAVE_WINDOW with an ifeq as it reads.
#
# CONDITIONAL, AND satl IS NOT (003's reason). gtk4 and vte are a desktop's
# libraries, and a build machine or a container routinely has neither; a Makefile
# that dies there has made the interpreter unbuildable to deliver a window. So
# `make` builds satl-term when it CAN and says one line when it cannot, and `make
# build/satl-term` asked for by name fails loudly with the reason.
#
# vte-2.91-gtk4 AND NOT vte-2.91, which is the GTK3 build of the same library. On
# AlmaLinux the package is vte291-gtk4-devel, in CRB. Linux only: VTE is a Linux
# terminal widget.
WINDOW_PKGS = vte-2.91-gtk4

HAVE_WINDOW   := $(shell pkg-config --exists $(WINDOW_PKGS) 2>/dev/null && echo yes || echo no)
WINDOW_CFLAGS := $(shell pkg-config --cflags $(WINDOW_PKGS) 2>/dev/null)
WINDOW_LIBS   := $(shell pkg-config --libs $(WINDOW_PKGS) 2>/dev/null)

# ---------------------------------------------------------------------------
# AND NOW satl ITSELF DRAWS -- satellite.window, 2026-09-20 (SATELLITE_WINDOW.md
# WIN-3). GTK4 ONLY, NOT VTE: satl opens a window, satl-term is a terminal in
# one, and satl must not link a terminal widget to do it.
# ---------------------------------------------------------------------------
#
# SEPARATE FROM HAVE_WINDOW ABOVE, on purpose. A machine can have gtk4 and no
# vte -- that is the ordinary case outside a desktop distribution's CRB repo --
# and there satl draws while satl-term is not built. Sharing one flag would have
# made satl's window depend on a terminal library it never calls.
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
#     readelf -d              the eight allowed, and nothing else
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

HAVE_GTK   := $(shell [ -f $(GTK_BUILD)/gtk/libgtk.a ] && echo yes || echo no)
GTK_CFLAGS := $(shell env -u PKG_CONFIG_PATH PKG_CONFIG_LIBDIR=$(GTK_PC_LIBDIR) pkg-config --cflags $(GTK_PKGS) 2>/dev/null)

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
GTK_LIBS = -Wl,--start-group $(GTK_BUILD)/gtk/libgtk.a $(GTK_ARCHIVES) -Wl,--end-group \
           -lm -lpthread -lrt -lresolv -lwayland-client -lwayland-egl

GTK_KIND = vendored (GTK carried inside satl)

else

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

# satl-term links the window and NOTHING of the runtime. What it shares with satl
# is headers only -- satellite/version/title_lines.hpp, which reads the rows, and
# satellite/machine/machine_codes.hpp and shown.hpp -- so no .cpp of the
# interpreter may appear here. From the outside in: the command line, the menu,
# the tabs, one terminal, the satl inside it, and what a keystroke means.
TERM_SOURCES = $(TERM_DIR)/window.cpp $(TERM_DIR)/menu.cpp $(TERM_DIR)/tabs.cpp \
               $(TERM_DIR)/terminal.cpp $(TERM_DIR)/child.cpp $(TERM_DIR)/keys.cpp
TERM_HEADERS = $(TERM_DIR)/menu.hpp $(TERM_DIR)/tabs.hpp $(TERM_DIR)/terminal.hpp \
               $(TERM_DIR)/child.hpp $(TERM_DIR)/keys.hpp

TERM_OBJECTS = $(TERM_SOURCES:%.cpp=$(OBJECTS)/%.o)

# THE WINDOW SOURCES satl LINKS IN, and they are the interpreter's, not
# satl-term's: the desk that owns the one GTK thread, what a program can do to a
# window, and the pieces that go inside one (window_pieces.cpp, split out at
# GTK-1 for the line rule). Empty when there is no gtk4, which is what makes satl buildable
# without one. bytecode/window_calls.cpp is NOT here -- it is in
# INTERPRETER_SOURCES and compiled always, because it is the file that says
# this satl has no window.
ifeq ($(HAVE_GTK),yes)
GTK_SOURCES = $(SATELLITE)/satellite_variable_window/window_desk.cpp \
              $(SATELLITE)/satellite_variable_window/satellite_window.cpp \
              $(SATELLITE)/satellite_variable_window/window_pieces.cpp \
              $(SATELLITE)/satellite_variable_window/window_asks.cpp \
              $(SATELLITE)/satellite_variable_window/window_state.cpp \
              $(SATELLITE)/satellite_variable_window/window_answers.cpp \
              $(SATELLITE)/satellite_variable_window/window_look.cpp \
              $(SATELLITE)/satellite_variable_window/window_spill.cpp
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
