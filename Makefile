# satellite 004 -- the build.
#
#     make                 build/satl, every word inside it (2026-10-07) -- a new BUILD
#                          number every time (050-build.mk's BARE_MAKE) -- then it ASKS whether
#                          to install into ~/.satl: Enter, y or a minute's silence installs, n
#                          leaves the build where it is (the author, the evening of 2026-10-07)
#     make install         build what changed, then install into ~/.satl, asking nothing
#                          (080-install.mk); make INSTALL_AFTER_BUILD=no builds and asks nothing
#     make test            check.sh and the string checks
#     make check           check.sh alone
#     make clean
#
# THIS FILE IS AN INDEX, the same kind 003's is (PLAN M0.5): the build is the
# fragments under make_support/, included in the order they are numbered. Ported
# from 003 revision 07 (old_versions/second_satellite/make_support/) on
# 2026-09-17, keeping the numbers and the reasons, not the contents -- 004 builds
# a different program.
#
# WHERE TO LOOK, by what you want to change:
#
#     how many recipes run at once ........ 005-jobs.mk
#     a compiler or a warning flag ........ 010-compiler.mk
#     the build number and what it covers  020-version.mk
#     a new source directory .............. 030-directories.mk
#     what satl is compiled from .......... 040-sources.mk
#     the window and the console .......... 047-window.mk
#     what a link is allowed to record .... 048-link.mk
#     a new binary ........................ 050-build.mk
#     a build for one processor, or all ... 055-cpus.mk
#     how a .cpp becomes a .o ............. 060-compile.mk
#     a test, a harness or a race ......... 065-tests.mk
#     what `make clean` removes ........... 070-clean.mk
#     make install ........................ 080-install.mk
#
# NOT PORTED from 003, and why: 045 (the haswell pair and satl-cpu-level) -- 004's
# answer is 055, every processor clang can build for, and `make` still builds one;
# 048's STATIC (004 was dynamic on purpose while the words were .so files: satl and every
# library had to share ONE libstdc++, or each had its own std::cout -- DESIGN §3.4; since
# 2026-10-07 the words are inside satl, and STATIC is a question open again),
# and 067 (003's start-up rows are 003 commands). 080 came across on 2026-09-22,
# when the author ruled D0.5.1: 004 installs into ~/.satl with every make -- and
# since 2026-10-07 with `make install` only; a bare make builds ("plain make should
# build the program").
#
# ORDER IS LOAD-BEARING, AND SO IS 050's `.DEFAULT_GOAL := all`: 047 declares `make
# window` before 050 declares anything (2026-09-23), so without that line a plain
# `make` would build the GTK stack instead of satl. (047 used to set HAVE_WINDOW for
# 050's satl-term ifeq; satl-term is gone since 2026-09-22.)
#
# NAMED ONE BY ONE, never $(wildcard make_support/*.mk): a wildcard sorts 100-
# before 020- and takes in an editor's backup. The paths are relative, so make
# runs from this directory.

# NEVER AS ROOT (2026-10-07: a `sudo make` left root-owned files in the tree and built with
# /usr/bin/clang++ 21, because root's HOME has no clang-current -- 010-compiler.mk -- and nothing
# installs for root anyway, 080-install.mk). Refused before a fragment is read.
ifeq ($(shell id -u),0)
$(error make as yourself, not as root: root's HOME has no clang-current, a root make leaves root-owned files in the tree, and nothing installs for root)
endif

include make_support/005-jobs.mk
include make_support/010-compiler.mk
include make_support/020-version.mk
include make_support/030-directories.mk
include make_support/040-sources.mk
include make_support/045-optimise.mk
include make_support/047-window.mk
include make_support/048-link.mk
include make_support/050-build.mk
include make_support/055-cpus.mk
include make_support/060-compile.mk
include make_support/065-tests.mk
include make_support/070-clean.mk
include make_support/080-install.mk
