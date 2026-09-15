# satellite -- `make startup`, the measurement PLAN §9 asks for every milestone.
#
# THE TARGET THAT RULE HAD BEEN MISSING SINCE THE PLAN WAS WRITTEN. §9 says
# "startup is re-measured every milestone" against §4.3's floor, and nothing
# implemented it -- so M4, M4.5 and M5 each landed without one, and M6's
# measurement found satl's own share had gone from 0.018 ms to 0.620 with three
# milestones of history to search for the cause (MILESTONES/M6.md §9.1, and §6.9
# is the open item this fragment closes). A rule with no target behind it is a
# rule nobody runs.
#
# THREE FILES, WHICH IS THE SAME SPLIT 065-tests.mk MAKES AND FOR THE SAME
# REASON -- the parts change on different schedules:
#
#     startup.rows   the commands to time and what each cost last time.
#                    Changes every milestone. Adding a command is one line.
#     startup.sh     how a measurement is taken. Changes almost never.
#     this file      how the build hands the two of them what they need.
#
# AFTER 065-tests.mk BY CONVENTION AND NOT BY REQUIREMENT, which is the same
# thing 065 says about its own position before 070 -- and 065 says it having
# been corrected once, so it is worth stating rather than assuming. Nothing here
# is read as make parses: $(STARTUP_FLOOR) below is a recursive variable and
# 070-clean.mk names it in a recipe, which make expands when the recipe runs.
# What this fragment DOES need is 048-static.mk's $(STATIC_LDFLAGS) and
# $(LINK_ENV), and 010's $(CXX) and $(CXXFLAGS) -- all set by plain assignment
# well above here.
#
# NOT IN `all` AND NOT IN `test`. startup.sh's header carries the argument for
# staying out of the suite; staying out of `all` is 065-tests.mk's rule applied
# to itself -- "PLAN §4.3's startup measurements are taken against what `make`
# produces, so adding a binary to the default goal would change what 'the build'
# means for a number that is compared across milestones." That warning was
# written about a test binary and it lands hardest here, on the one target whose
# whole subject IS that number.

# THE FLOOR: an empty program, linked exactly the way satl is.
#
# A DOTFILE, like the three stamps, because it is a build artefact of a target
# nobody runs by accident and the tree root has four binaries in it already.
#
# GENERATED ON THE COMPILER'S STDIN RATHER THAN CHECKED IN. `int main(){return
# 0;}` is three tokens, and a .cpp file holding them would be a source in this
# tree that `all` does not build -- which 040-sources.mk already had to write a
# whole paragraph of exception for, once, about satellite_random. One deliberate
# exception is a decision and two is a habit. Compiling from `-` leaves nothing
# behind and puts the thing being measured on the line that measures it.
STARTUP_FLOOR = .startup-floor

# THE SAME LINK LINE AS satl'S, WORD FOR WORD, AND THAT IS THE POINT OF THE
# TARGET. 040-sources.mk records what happens when the two sides differ: satl is
# shipped static, timing it against a DYNAMIC empty program made satl look 0.9 ms
# FASTER than a program that does nothing, and the instruction written down at
# the time was "both sides static". Deriving the floor's flags from the same
# variables 050-build.mk uses is stronger than an instruction -- there is no
# longer a second place to get it wrong, at any setting of STATIC and not only
# at `full`.
#
# ON BOTH STAMPS, which is the prerequisite easiest to leave off and the one this
# binary needs most. CXXFLAGS and STATIC are prerequisites of nothing, so without
# these `make startup STATIC=full` over a tree built without it would time a
# fresh static satl against the DYNAMIC floor the last run left lying around --
# the exact mistake above, reintroduced by a stale file rather than by a typo.
# 065-tests.mk found this on words_test's first day and 048-static.mk found it on
# four installed binaries; it is the third time in this build.
$(STARTUP_FLOOR): .cxxflags-stamp .ldflags-stamp
	@echo 'int main(){return 0;}' | $(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) \
	    $(STATIC_LDFLAGS) -x c++ - -o $@

# `make startup` -- and `make startup RUNS=1000 BATCHES=10` for a quiet machine.
#
# THE CONDITIONS ARE PASSED IN rather than probed by the script. Which compiler
# is used is 010-compiler.mk's decision and how the tree is linked is 048's, and
# a second probe inside a shell script is a second decision that can disagree
# with them -- which is the argument 048's `static-available` target already
# makes for install.sh, made again one fragment later.
#
# satl IS A PREREQUISITE, so `make startup` on a clean tree builds what it is
# about to measure instead of timing whatever binary was there. What it must NOT
# depend on is $(ALL_TARGETS): satl-term is a GUI binary that nothing here runs,
# and a measurement that cannot be taken on a machine without gtk4 would be a
# measurement this project's own headless builds could never check.
.PHONY: startup
startup: satl $(STARTUP_FLOOR) make_support/startup.rows make_support/startup.sh
	@SATL=./satl FLOOR=$(STARTUP_FLOOR) ROWS=make_support/startup.rows \
	 RUNS=$(RUNS) BATCHES=$(BATCHES) \
	 STARTUP_CXX='$(CXX)' STARTUP_CXXFLAGS='$(CXXFLAGS)' STARTUP_LINK='$(STATIC)' \
	 bash make_support/startup.sh

# The defaults, here rather than only in the script, so that `make startup
# RUNS=50` works without the script having to know it was overridden. Both are
# the numbers every figure in 040-sources.mk was taken with, and changing them
# makes a run incomparable to the table it prints itself against.
RUNS    ?= 200
BATCHES ?= 5
