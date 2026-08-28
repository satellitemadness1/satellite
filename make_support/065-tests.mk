# satellite -- the tests.
#
# THE FIRST TEST TARGET THIS TREE HAS HAD. FORMAT/CXX.md §6 opens with "there is
# no test infrastructure in this tree -- not a target, not a directory, not a
# harness", and says the first satellite's should be PORTED rather than
# reinvented. This is that port, cut down to the parts that earn their place at
# one test: TESTNAMES in 030-directories.mk, per-test source wildcards, and a
# binary list derived from the same name.
#
# AFTER 030-directories.mk, AND THAT ONE IS A HARD REQUIREMENT. The $(foreach
# ...)$(eval ...) lines below are expanded by make AS IT READS THEM, so TESTNAMES
# has to be set by the time it gets here -- the same class of constraint the
# Makefile calls out for 045 and 047 before 050. Moved above 030, TESTNAMES is
# empty, no per-test source list is ever defined, TESTBINS is empty, and `test`
# becomes a rule with no prerequisites that runs a binary nothing built. That is
# this fragment's own stale-binary lesson, arriving through the include order.
#
# Before 070-clean.mk only by convention. That was written here as "load-bearing"
# and it is not: `clean` names $(TESTBINS), which is recursive and expanded when
# the recipe RUNS, and .PHONY is collected across the whole read. Verified by
# swapping them 2026-08-28 -- nothing changed. Corrected rather than deleted,
# because a fragment claiming a constraint it does not have teaches the next
# person that the ones which ARE real can be ignored too.
#
# NOT PART OF `all`. A test binary is not something an install ships, and PLAN
# §4.3's startup measurements are taken against what `make` produces -- adding a
# binary to the default goal would change what "the build" means for a number
# that is compared across milestones.

# EVERY .cpp IN A TEST'S FOLDER IS PART OF THAT TEST. A wildcard rather than a
# hand-written list, so a test that gets split into six files needs no edit here
# and -- more to the point -- a new piece CANNOT be forgotten.
$(foreach t,$(TESTNAMES),$(eval $(t)_SRCS = $$(wildcard $$(TESTS)/$(t)/*.cpp)))

# The headers are a SEPARATE list because they are a DEPENDENCY and not an
# input. The first satellite found this the hard way: a split test grew a
# <name>.hpp holding the harness declarations and the section prototypes, and
# without this a change to that header relinked nothing.
$(foreach t,$(TESTNAMES),$(eval $(t)_HDRS = $$(wildcard $$(TESTS)/$(t)/*.hpp)))

TESTBINS = $(foreach t,$(TESTNAMES),$(TESTS)/$(t)/$(t))

# words_test LINKS NO OBJECTS, and that is a property of the registry worth
# keeping rather than an accident of it being first. Everything under
# satellite_words/ except dump.cpp is constexpr data and pure functions over it,
# so the numbering can be read by a test, a future .satc reader or a
# disassembler without dragging the interpreter in behind it.
#
# It depends on words.def as well as on the headers, because the .def is what
# actually changes when the language gains a word.
#
# AND ON WORD_NUMBERS.md, which is the unusual one. This test's subject is
# whether the transcription matches the authority, so the authority is an input
# to it in exactly the way a source file is: editing the numbering must re-run
# the test that checks the numbering, and without this line it would not.
# AND ON .cxxflags-stamp, which is the one prerequisite it is easiest to leave
# off and the one this binary needs most. CXXFLAGS and CXX are prerequisites of
# nothing, so without it `make OPT=-O0 test` and `make CXX=g++ test` RE-RAN THE
# BINARY THE PREVIOUS BUILD LEFT -- verified 2026-08-28: dump.o recompiled, the
# test binary did not, and it printed `ok` from a clang -O3 build while the
# command line said g++ -O0. Most of words_test is `static_assert`s, so "does
# the registry compile under this compiler at this -O" IS the test, and that is
# exactly what was being skipped. FORMAT/CXX.md §5 names the stamps as one of
# the two things not to break; this rule broke one on its first day.
$(TESTS)/words_test/words_test: $(words_test_SRCS) $(words_test_HDRS) \
                                $(WORDS)/words.def $(HDRS) WORD_NUMBERS.md \
                                .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I$(TESTS)/words_test -o $@ $(words_test_SRCS)

# The run list is written out rather than derived, because it is an ORDER and
# not a set. What it can no longer do is run a binary nobody built.
#
# WORD_NUMBERS.md IS PASSED AS AN ARGUMENT rather than found. The test defaults
# to the tree root, which is where make runs, and taking the path lets the
# binary be run from anywhere -- including from an editor, which is where a
# failing transcription is most likely to be looked at.
test: $(TESTBINS)
	./$(TESTS)/words_test/words_test WORD_NUMBERS.md

# Keeps `make words_test` working, which is what fingers type.
words_test: $(TESTS)/words_test/words_test

TESTALIASES = words_test

.PHONY: test $(TESTALIASES)
