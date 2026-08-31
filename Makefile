# satellite -- the build.
#
# THIS FILE IS AN INDEX. The build itself is the fragments under make_support/,
# included below in the order they are numbered. The first satellite arrived at
# this arrangement by splitting a 1232-line Makefile after the fact; this one
# starts here, which is the same decision PLAN_ONE.md sec 6a makes about C++
# files. Writing to a ceiling changes a file's shape; splitting to one preserves
# it.
#
# WHERE TO LOOK, by what you want to change:
#
#     a compiler or a warning flag ........ 010-compiler.mk
#     the version number .................. 020-version.mk
#     a new source directory .............. 030-directories.mk
#     what gets compiled or linked ........ 040-sources.mk
#     the two microarchitecture builds .... 045-microarchitecture.mk
#     satl-term and whether it is built ... 047-window.mk
#     a new target ........................ 050-build.mk
#     how a .cpp becomes a .o ............. 060-compile.mk
#     a test, or the test target .......... 065-tests.mk
#     what `make startup` measures ........ 067-startup.mk
#
# ORDER IS LOAD-BEARING in two places, and each fragment says so at its top:
# 010 before 020, because VERSION_DEFS bakes $(CXX) and $(CXXFLAGS) into the
# binary as strings and both must be settled first; and 050 is the first
# fragment that declares a target, which is what makes `all` the default goal.
# 045 and 047 are also read before 050, and that one IS a hard requirement
# rather than a convention: they set MICROARCH_VARIANTS and HAVE_WINDOW with a
# plain =, and 050 tests both with an ifeq, which make evaluates as it reads
# rather than afterwards. A variable read by a conditional has to be set by the
# time that conditional is reached. Both are numbered between 040 and 050
# rather than appended for exactly that reason -- they belong where they are
# read, not where they were added.
# Everywhere else these are recursively expanded variables and rules, which make
# resolves after the whole file is read, so a later fragment naming an earlier
# one's variable is fine and so is the reverse.
#
# NAMED ONE BY ONE rather than $(wildcard make_support/*.mk). A wildcard sorts
# asciibetically, which puts 100- between 010- and 020- and breaks the order
# constraints above with nothing but a filename; and it silently includes
# whatever else lands in that directory -- an editor's backup, a fragment
# somebody is part way through writing. This list fails loudly on a file that is
# missing and ignores one that is not wanted.
#
# The paths are RELATIVE, so make must be run with this directory as its working
# directory.

include make_support/010-compiler.mk
include make_support/020-version.mk
include make_support/030-directories.mk
include make_support/040-sources.mk
include make_support/045-microarchitecture.mk
include make_support/047-window.mk
include make_support/048-static.mk
include make_support/050-build.mk
include make_support/060-compile.mk
include make_support/065-tests.mk
include make_support/067-startup.mk
include make_support/070-clean.mk
