# satellite -- the six parent fragments this build reuses, and the one line
# that redirects them.
#
# AFTER 010-tree.mk, which defines $(ROOT). BEFORE 030-output.mk, which derives
# every object path in this build from the source lists inherited here.
#
# WHY INCLUDE AND NOT COPY. What gets compiled is declared in
# ../make_support/040-sources.mk, and that is the only place it is declared --
# LAYOUT.md opens by saying so. A second copy of SATL_SRCS in this directory
# would be a second declaration, and the two would agree exactly until the day
# the language gained a source file and somebody updated one of them. That
# failure is silent in the worst way available: this build would keep linking,
# from a list that no longer describes the interpreter, and produce a satl that
# is missing a translation unit rather than a build that stops.
#
# The project has already paid for this once. 030-directories.mk records that
# the first satellite's test binary list WAS written out by hand, that two tests
# were added to the run list and to neither build list, and that `make test`
# then reported PASS from objects compiled before the change under test -- "not
# a red line, a green one that is out of date". A hand-copied SATL_SRCS is the
# same mistake with the interpreter in place of the suite.
#
# So the lists are inherited, and the ONLY thing this directory states about
# them is where they are rooted.
#
# SIX FRAGMENTS, AND THEY ARE THE SIX THAT DECLARE NO TARGETS. That is the
# property that makes this legal rather than clever: an include drags in
# everything a file contains, so a fragment holding a rule would hand this build
# the parent's rules, which put objects beside sources and binaries in the
# parent's directory -- the precise thing 010-tree.mk says must not happen.
# Checked fragment by fragment, 2026-08-28:
#
#     010-compiler.mk ........... 5 assignments, one ifeq. No targets.
#     020-version.mk ............ 6 assignments. No targets.
#     030-directories.mk ........ 7 assignments. No targets.
#     040-sources.mk ............ 6 assignments. No targets.
#     045-microarchitecture.mk .. 6 assignments, one ifneq. No targets.
#     047-window.mk ............. 6 assignments. No targets.
#
# and the five that are NOT included are exactly the five that do declare
# targets: 048-static.mk (.ldflags-stamp, static-available, static-note),
# 050-build.mk, 060-compile.mk, 065-tests.mk and 070-clean.mk. This directory
# writes its own versions of the first three, numbered 040, 050 and 060 to say
# which parent fragment each one answers.
#
# 048 IS REWRITTEN RATHER THAN INCLUDED FOR A SECOND REASON, which is that its
# two $(error) messages name dnf packages. On this family the answer is
# different and shorter -- both static libraries arrive with build-essential --
# and an installer message that sends a Debian user to `dnf --enablerepo=crb`
# is worse than no message. See 040-static.mk.
#
# NOT FREE OF SIDE EFFECTS, and worth knowing before adding a seventh: reading
# these forks subprocesses. 010 runs $(wildcard) against $HOME, 045 runs
# $(CXX) -dumpmachine, and 047 runs pkg-config three times. All are := or
# read-time conditionals, so each happens once per make invocation and not once
# per reference, which is the reason those fragments chose := in the first place.
include $(ROOT)/make_support/010-compiler.mk
include $(ROOT)/make_support/020-version.mk
include $(ROOT)/make_support/030-directories.mk
include $(ROOT)/make_support/040-sources.mk
include $(ROOT)/make_support/045-microarchitecture.mk
include $(ROOT)/make_support/047-window.mk

# THE ONE LINE THAT REDIRECTS ALL SIX, and it works because of an operator.
#
# 030-directories.mk assigns with a plain `=`:
#
#     SRC      = src
#     SYSTEM   = $(SRC)/system_facts
#     PROGRAMS = $(SRC)/programs
#     RANDOM   = $(SRC)/satellite_random
#     WORDS    = $(SRC)/satellite_words
#
# A recursively expanded variable stores its right-hand side as unexpanded text.
# PROGRAMS holds the literal characters `$(SRC)/programs`, and $(SRC) is looked
# up at every REFERENCE rather than once at assignment -- so reassigning SRC
# here reaches all four module variables, and through them SATL_SRCS, TERM_SRCS,
# CPU_LEVEL_SRC, RANDOM_SRCS and every entry of HDRS. Five words of override
# move the whole source tree.
#
# := AND NOT =, because the right-hand side names $(ROOT) and there must be no
# way for a later fragment to move the source tree by moving ROOT. This is a
# fact being recorded, not a knob.
#
# AFTER ALL SIX INCLUDES AND NOT BETWEEN THEM. It could go immediately after 030
# and mean the same thing today, because nothing in 040, 045 or 047 expands
# $(SRC) at read time -- their only := assignments are TARGET_TRIPLE and the
# three pkg-config probes, none of which mention it. Putting it last does not
# depend on that remaining true.
SRC := $(ROOT)/src
