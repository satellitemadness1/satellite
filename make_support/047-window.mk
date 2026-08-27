# satellite -- satl-term, and whether this machine can build it.
#
# AFTER 040-sources.mk, which names the sources satl is made from, and BEFORE
# 050-build.mk, which needs HAVE_WINDOW to know whether `all` has a fourth
# binary to produce. That is the same hard ordering requirement 045 has and for
# the same reason: this fragment sets a variable with a plain =, and 050 tests
# it with an ifeq, which make evaluates as it reads rather than afterwards. It
# is numbered between 045 and 050 because it belongs where it is read.
#
# WHY THIS IS CONDITIONAL AND satl IS NOT. satl-term is the one binary in this
# build with a dependency the machine may simply not have: gtk4 and vte are a
# desktop's libraries, and a build machine, a container and a package builder
# routinely have neither. A Makefile that dies there has made the interpreter
# unbuildable to deliver a terminal nobody asked for. So the window is built
# when it CAN be, skipped with a word when it cannot, and `make satl-term`
# asked for explicitly still fails loudly -- because somebody who typed that
# name wants the reason, not a shrug.
#
# vte-2.91-gtk4 AND NOT vte-2.91. They are different modules: the second is the
# GTK3 build of the same library, it is what is installed on most machines that
# have either, and linking it against gtk4 produces a wall of type errors
# forty lines from anything naming the real cause. On AlmaLinux the package is
# vte291-gtk4-devel and it lives in CRB, which is not enabled by default.
#
# LINUX ONLY, and that is a property of VTE rather than a decision taken here:
# it is a Linux terminal widget, so satl-term does not cross-compile to Windows
# with the rest of the tree. SCRATCH.md/PORTING.md is where that is recorded.
WINDOW_PKGS = vte-2.91-gtk4

HAVE_WINDOW := $(shell pkg-config --exists $(WINDOW_PKGS) 2>/dev/null && echo yes || echo no)

# := and not =, so pkg-config is run once when this line is read rather than
# once per object, per link, and once more for every place the flags appear.
WINDOW_CFLAGS := $(shell pkg-config --cflags $(WINDOW_PKGS) 2>/dev/null)
WINDOW_LIBS   := $(shell pkg-config --libs $(WINDOW_PKGS) 2>/dev/null)

# satl-term -- the window, and NOTHING of the runtime. The only file it shares
# with satl is version.hpp, which is a header. When that stops being true the
# split has been broken and this list is where it shows.
TERM_SRCS = $(PROGRAMS)/window.cpp \
            $(PROGRAMS)/terminal.cpp

TERM_OBJS = $(TERM_SRCS:.cpp=.o)

# No .haswell variant, deliberately. This binary interprets nothing, so there
# is no hot loop for -march to act on, and building it twice would double a
# link against 78 shared objects to produce two identical windows.
