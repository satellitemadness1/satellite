# satellite -- what gets compiled, and what gets linked into what.
#
# ONE FILE PER NAME, spelled out rather than wildcarded. A wildcard here would
# quietly compile a file somebody is part way through writing, and quietly stop
# compiling one that got renamed; this list fails loudly on the first and
# noisily on the second.

# satl -- the interpreter. Links NO GUI, and that is measured rather than tidy.
#
# MEASURED HERE, 2026-08-26, rather than quoted. The first satellite's source
# says gtk4 and vte pull "119 shared objects"; on this machine its satl-term
# resolves 79 and maps 78 (`ldd | wc -l`, and LD_DEBUG=libs counted by "calling
# init:"). 119 is not this machine's number and is not repeated. What holds is
# the shape of it -- 78 against satl's 6 -- and the cost, which the first
# satellite measured at 25.9 ms with the link against 2.5 ms without.
#
# This build, best of five runs of 200 invocations:
#
#     bare int main(){return 0;}          1.74 ms
#     satl (M1, opening information)      1.75 ms
#     satl --version                      1.75 ms
#
# So satl's own share of starting up is about 0.01 ms, which is the whole point
# of taking the measurement now: every later milestone has a floor to be
# compared against, and a regression has somewhere to be attributed.
#
# The window is a separate binary (M11.A, built 2026-08-27) and, for
# satellite.window.new(), a
# dlopen'd library (M13) -- because the two-binary split cannot help a window
# opened from inside a user program, which runs in this one. PLAN_ONE.md sec 4.4.
SATL_SRCS = $(PROGRAMS)/main.cpp \
            $(PROGRAMS)/opening.cpp

SATL_OBJS = $(SATL_SRCS:.cpp=.o)

# Every header any object depends on. Listed rather than generated: -MMD would
# do this automatically and is the obvious answer, but it writes .d files into
# the tree and makes a from-scratch build depend on files a clean has removed.
# This list is short and stays short if it is maintained; when it stops being
# either, revisit that decision on purpose rather than by drift.
HDRS = $(SYSTEM)/version.hpp \
       $(PROGRAMS)/opening.hpp \
       $(PROGRAMS)/terminal.hpp

# Every object in the tree, which is what 060-compile.mk hangs the header
# dependency on. The haswell objects and the detector are named here rather than
# only where they are built, so that adding a header stays one edit in HDRS
# above and reaches every object rather than only the ones somebody remembered.
#
# $(SATL_HASWELL_OBJS) and $(CPU_LEVEL_OBJ) come from 045-microarchitecture.mk
# and $(TERM_OBJS) from 047-window.mk, both read after this file; recursive
# expansion is what makes that legal, and the top-level Makefile says so once
# for all the fragments. TERM_OBJS is empty on a machine with no gtk4, which is
# what keeps this line honest there rather than naming objects nothing builds.
OBJS = $(SATL_OBJS) $(SATL_HASWELL_OBJS) $(CPU_LEVEL_OBJ) $(TERM_OBJS)
