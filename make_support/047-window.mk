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
GTK_PKGS = gtk4

HAVE_GTK   := $(shell pkg-config --exists $(GTK_PKGS) 2>/dev/null && echo yes || echo no)
GTK_CFLAGS := $(shell pkg-config --cflags $(GTK_PKGS) 2>/dev/null)
GTK_LIBS   := $(shell pkg-config --libs $(GTK_PKGS) 2>/dev/null)

# THE STATIC STACK IS THE SHIPPING ANSWER AND IS NOT WIRED IN YET. vendor/gtk/
# proves a GTK4 binary that carries GTK opens a window where no GTK is installed
# (SATELLITE_WINDOW.md Part 1), and turning that on is a change to these three
# variables and nothing else -- the archives, the --start-group, and the five
# excluded by name, exactly as vendor/gtk/hello/build.sh gathers them. It also
# needs WIN-1's startup spill first, because a bare machine with no
# xkeyboard-config SIGSEGVs inside gtk_init(). Until then satl draws where GTK
# is installed, which is every machine this is developed on.
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
              $(SATELLITE)/satellite_variable_window/satellite_window.cpp
else
GTK_SOURCES =
endif

GTK_OBJECTS = $(GTK_SOURCES:%.cpp=$(OBJECTS)/%.o)
