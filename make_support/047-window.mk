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
# WHY system IS STILL THE DEFAULT, 2026-09-20: WIN-1 is not built, so a vendored
# satl SIGSEGVs inside gtk_init() the moment a window word runs -- it has no
# xkeyboard-config and no fontconfig config, and neither of those is code that
# can be linked. Measured, with the real satl, not with hello. **Flip this
# default to vendor the day WIN-1 lands**; nothing else here has to change.
GTK ?= system

GTK_PKGS  = gtk4

# POINTS AT gtk-old ON PURPOSE, 2026-09-20. vendor/gtk was renamed to vendor/gtk-old
# to free the name for the new stack built from vendor/new/ (GTK 4.24.0 and 25 other
# frozen sources). gtk-old still holds the ONLY built GTK on this machine -- 2.2 GB,
# an hour to make -- so `make GTK=vendor` keeps working through the whole changeover
# rather than going dark until the new stack links. Nothing else in this file changes:
# every other GTK path is derived from this one line.
#
# **Move this to vendor/gtk the day the new stack links green**, and delete gtk-old
# only after that -- GTK_AND_NO_DEPENDENCIES.md Part 0 describes the old tree and stops
# being true the moment it is gone.
GTK_BUILD = $(CURDIR)/vendor/gtk-old/build-static

ifeq ($(GTK),vendor)

# THE VENDORED STACK. Its pkg-config answers entirely out of vendor/ -- measured
# 2026-09-20: `--cflags gtk4` through meson-uninstalled returns ZERO -I/usr paths,
# which is what makes uninstalling the system GTK unnecessary. The build simply
# never asks it.
HAVE_GTK   := $(shell [ -f $(GTK_BUILD)/gtk/libgtk.a ] && echo yes || echo no)
GTK_CFLAGS := $(shell PKG_CONFIG_PATH=$(GTK_BUILD)/meson-uninstalled pkg-config --cflags $(GTK_PKGS) 2>/dev/null)

# EVERY ARCHIVE THE BUILD PRODUCED, gathered exactly as vendor/gtk/hello/build.sh
# gathers them -- libgtk.a FIRST so its undefined symbols drive the rest, the whole
# lot in a --start-group because the graph has cycles, and FIVE EXCLUDED BY NAME:
# libmalloc-stats.a DEFINES malloc/realloc, libcairo-trace.a and libcairo-fdr.a are
# LD_PRELOAD interposers that redefine cairo_*, libdemo.a is pixman's demo, and
# libintl.a is a STUB gettext that collides with glibc's own _nl_msg_cat_cntr.
# The test is the NAME, not the directory: cairo keeps two REAL libraries under the
# same util/ that GSK needs, so excluding util/ wholesale breaks the link instead.
GTK_ARCHIVES := $(shell find $(GTK_BUILD) -name '*.a' ! -name 'libgtk.a' 2>/dev/null | \
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
# satl-term's: the desk that owns the one GTK thread, and what a program can do
# to a window. Empty when there is no gtk4, which is what makes satl buildable
# without one. bytecode/window_calls.cpp is NOT here -- it is in
# INTERPRETER_SOURCES and compiled always, because it is the file that says
# this satl has no window.
ifeq ($(HAVE_GTK),yes)
GTK_SOURCES = $(SATELLITE)/satellite_variable_window/window_desk.cpp \
              $(SATELLITE)/satellite_variable_window/satellite_window.cpp \
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
                     $(wildcard vendor/xkb/xkb-data/rules/*)

ifeq ($(HAVE_GTK),yes)
GTK_OBJECTS += $(WINDOW_DATA_OBJECT)
endif
