# satellite 004 -- what a link is allowed to record about the machine it ran on.
#
# 003's 048 WITHOUT STATIC (PLAN M0.5). 004 is dynamic on purpose -- satl and every
# library in build/satellite-numbers/ must share one libstdc++, or each has its own
# std::cout (DESIGN §3.4) -- and GTK4 and VTE ship no .a anyway. So `make
# STATIC=full` builds exactly what `make` does, and says so once.
#
# LD_RUN_PATH IS REMOVED FOR EVERY LINK: satl and every library.
# This shell exports it, and GNU ld silently writes it into the binary as an
# RPATH naming directories in one home -- a binary that then finds libstdc++ in
# /home/madness/opt/gcc-17 here and something else everywhere else. `env -u` and
# not `LD_RUN_PATH=`: set-but-empty is read as "an RPATH of nothing" and still
# writes a tag. build_libraries.py removes it from its own environment for the
# same reason.
LINK_ENV = env -u LD_RUN_PATH

ifneq ($(origin STATIC),undefined)
$(info note: STATIC is 003's. satl 004 and its libraries link dynamically, on purpose: they must share one libstdc++.)
endif

# make relinks when a PREREQUISITE changes, and the link flags are not one; this
# stamp is, and is rewritten only when the flags change (003's .ldflags-stamp).
LINK_STAMP = $(BUILD)/.link-flags
