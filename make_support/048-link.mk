# satellite 004 -- what a link is allowed to record about the machine it ran on.
#
# 003's 048 WITHOUT STATIC (PLAN M0.5). 004 was dynamic on purpose while the words were
# .so files -- satl and every library had to share one libstdc++, or each had its own
# std::cout (DESIGN §3.4). Since 2026-10-07 the words are inside satl and that reason is
# gone, and since 2026-10-22 a clang build carries its C++ library inside satl: libc++,
# libc++abi and the unwinder, static (010-compiler.mk's CXX_RUNTIME_FLAGS, the author's
# "port it to libc++"). A g++ build still links the machine's libstdc++ (050-build.mk's
# ALLOWED_NEEDED). `make STATIC=full` still builds exactly what `make` does, and says so once.
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
$(info note: STATIC is 003's and changes nothing. Built with clang, satl carries libc++ inside it already (010-compiler.mk); built with g++, it links the machine's libstdc++.)
endif

# make relinks when a PREREQUISITE changes, and the link flags are not one; this
# stamp is, and is rewritten only when the flags change (003's .ldflags-stamp).
LINK_STAMP = $(BUILD)/.link-flags
