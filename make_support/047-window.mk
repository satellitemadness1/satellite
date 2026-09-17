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
