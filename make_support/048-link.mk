# satellite 004 -- what a link is allowed to record about the machine it ran on.
#
# 003's 048 WITHOUT STATIC (PLAN M0.5). 004 was dynamic on purpose while the words were
# .so files -- satl and every library had to share one libstdc++, or each had its own
# std::cout (DESIGN §3.4). Since 2026-10-07 the words are inside satl and that reason is
# gone; whether libstdc++ goes static is the author's (050-build.mk's ALLOWED_NEEDED,
# DEP-2). `make STATIC=full` still builds exactly what `make` does, and says so once.
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
$(info note: STATIC is 003's. satl 004 links dynamically; a static libstdc++ is a question open since the words went inside satl, 2026-10-07.)
endif

# make relinks when a PREREQUISITE changes, and the link flags are not one; this
# stamp is, and is rewritten only when the flags change (003's .ldflags-stamp).
LINK_STAMP = $(BUILD)/.link-flags
