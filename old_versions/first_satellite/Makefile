# satellite -- the build.
#
# THIS FILE IS AN INDEX. The build itself is sixteen fragments under
# make_support/, included below in the order they are numbered, and that order
# is the order they used to appear in when this was one 1232-line file. Nothing
# was rewritten to get here: every line of the old file is in one of the
# fragments, in its original order, and `make` produces the same commands it
# did before -- verified by diffing the whole of `make -p` and the dry runs of
# all, satl, test, install, uninstall and clean across the change.
#
# WHY, in the words of the rule the rest of the tree now follows: no file over
# 325 lines, "mostly for AI's, they want to quickly zip around the project".
# A 1232-line Makefile is the file where a build detail goes to be lost. Each
# fragment below is one subject and none of them reaches 200 lines.
#
# WHERE TO LOOK, by what you want to change:
#
#     a compiler or a warning flag ........ 020-compiler.mk
#     the version number .................. 030-version.mk
#     where an install goes ............... 060-prefix.mk
#     a new source directory .............. 070-directories.mk
#     what gets compiled or linked ........ 080-sources.mk
#     how a .cpp becomes a .o ............. 110-compile.mk
#     a new test .......................... 070-directories.mk (TESTNAMES),
#                                           then 120-tests.mk
#     a file the install must ship ........ 140-install.mk
#
# ORDER IS LOAD-BEARING in exactly three places, and each fragment says so at
# its top: 020 before 030, because VERSION_DEFS bakes $(CXX) and $(CXXFLAGS)
# into the binary as strings; 100 is the first fragment that declares a target,
# which is what makes `all` the first target make sees; and 120 before 160,
# because .PHONY reads $(TESTALIASES). Everywhere else the fragments are
# recursively expanded variables and rules, which make resolves after the whole
# file is read, so a later fragment naming an earlier one's variable is fine
# and so is the reverse.
#
# The includes are RELATIVE, like every other path in this build -- src,
# dist/icons, example/. All of it assumes make is run with this directory as
# its working directory, which is how install.sh (`make -C "$repo"`) and
# debian/rules both invoke it.
#
# Split out of the single file on 2026-08-24. See plans/restructure.txt.

# The directory this file was read from, and the ONE thing the root Makefile
# must measure itself.
#
# 090-autoinstall.mk compares it against $(CURDIR) to decide whether `make`
# should end in an install, and the comparison only means anything if the path
# is THIS file's. MAKEFILE_LIST grows with every include, so a fragment asking
# the same question gets make_support/ for an answer and the auto-install
# switches itself off in silence. Taking the measurement here is the fix, and
# it has to be ABOVE the includes: after them, lastword is the last fragment.
#
# lastword rather than firstword, and realpath rather than abspath, for the two
# reasons 090-autoinstall.mk gives at length beside the variable that uses this.
SATELLITE_TREE := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

# NAMED ONE BY ONE rather than $(wildcard make_support/*.mk), for two reasons.
# A wildcard sorts asciibetically, where 100- lands between 010- and 020- and
# the three order constraints above are broken by a filename. And a wildcard
# silently includes whatever else ends up in that directory -- an editor's
# backup, a fragment somebody is part way through writing -- where this list
# fails loudly on a file that is missing and ignores one that is not wanted.
include make_support/010-packages.mk
include make_support/020-compiler.mk
include make_support/030-version.mk
include make_support/040-flags.mk
include make_support/050-thread-sanitizer.mk
include make_support/060-prefix.mk
include make_support/070-directories.mk
include make_support/080-sources.mk
include make_support/090-autoinstall.mk
include make_support/100-build.mk
include make_support/110-compile.mk
include make_support/120-tests.mk
include make_support/130-examples-and-man.mk
include make_support/140-install.mk
include make_support/150-install-report.mk
include make_support/160-uninstall-clean.mk
