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

# lexer_test LINKS TWO MODULE SOURCES, and that is the difference between this
# test and words_test above. It is a property of the SUBJECT and not of the
# test: the registry is constexpr data and pure functions, so its test compiles
# the headers and links nothing, while a lexer is a function over a string and
# has to actually run. The two named here are exactly the lexer and its
# alphabet, which is the whole of what DESIGN §5 says a lexer depends on.
#
# NOT $(SATL_OBJS), deliberately. Linking the interpreter's objects would drag
# main.o and its window handover into a test binary, and a test that starts by
# deciding whether to open a GUI is a test that hangs on a build machine.
LEXER_TEST_SRCS = $(LEXER)/lexer.cpp $(STRING)/satellite_string.cpp

# AND ON example/hello_world.satl, which is the unusual prerequisite and is the
# same argument words_test makes for WORD_NUMBERS.md one line up. LAYOUT.md
# calls the files in example/ "not samples -- each of these is what a milestone
# means by done", and section_spans() lexes this one. Editing the acceptance
# program must re-run the test that reads it.
$(TESTS)/lexer_test/lexer_test: $(lexer_test_SRCS) $(lexer_test_HDRS) \
                                $(LEXER_TEST_SRCS) $(WORDS)/words.def $(HDRS) \
                                example/hello_world.satl .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I$(TESTS)/lexer_test -o $@ \
	    $(lexer_test_SRCS) $(LEXER_TEST_SRCS)

# parser_test LINKS FIVE MODULE SOURCES, which is more than any test before it
# and is a property of the subject rather than of the test: a parser runs over a
# token stream, so it needs the lexer, the lexer needs the alphabet, and the
# tree it builds has a printer. Named one by one for the reason the two rules
# above give -- $(SATL_OBJS) would drag main.o and its window handover in, and a
# test that starts by deciding whether to open a GUI hangs on a build machine.
PARSER_TEST_SRCS = $(PARSER)/parser.cpp \
                   $(PARSER)/parser_declarations.cpp \
                   $(PARSER)/parser_statements.cpp \
                   $(PARSER)/parser_control_flow.cpp \
                   $(PARSER)/parser_expressions.cpp \
                   $(PARSER)/parser_types.cpp \
                   $(TREE)/ast.cpp \
                   $(TREE)/unparse.cpp \
                   $(LEXER)/lexer.cpp \
                   $(STRING)/satellite_string.cpp

# AND ON ALL SIX PROGRAMS IN example/, which is the same argument the two rules
# above make for WORD_NUMBERS.md and hello_world.satl and is the strongest form
# of it yet: section_roundtrip() reads four of them as the milestone's done-when
# and the other two as files that must NOT parse, with the reason pinned to a
# line number. Editing any of the six must re-run the test that reads it --
# including the two that fail, because a file that starts parsing is a finding
# and not a pass.
$(TESTS)/parser_test/parser_test: $(parser_test_SRCS) $(parser_test_HDRS) \
                                  $(PARSER_TEST_SRCS) $(WORDS)/words.def $(HDRS) \
                                  $(wildcard example/*.satl) .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I$(TESTS)/parser_test -o $@ \
	    $(parser_test_SRCS) $(PARSER_TEST_SRCS)

# satc_test LINKS THE MOST OF ANY TEST SO FAR, and for the reason parser_test
# gives one rule up carried one milestone on: a `.satc` writer runs over a
# parse tree, so it needs the parser, the parser needs the lexer, the lexer
# needs the alphabet -- and unparse.cpp is here because the writer is that
# printer with the paths substituted, and the two are compared.
#
# NOT $(SATL_OBJS), for the third time and for the same reason: linking the
# interpreter's objects would drag main.o and its window handover into a test
# binary, and a test that starts by deciding whether to open a GUI hangs on a
# build machine.
SATC_TEST_SRCS = $(CACHE)/paths.cpp \
                 $(CACHE)/write.cpp \
                 $(CACHE)/write_declarations.cpp \
                 $(CACHE)/write_expressions.cpp \
                 $(CACHE)/read.cpp \
                 $(CACHE)/unnumber.cpp \
                 $(CACHE)/save.cpp \
                 $(CACHE)/file.cpp \
                 $(PARSER)/parser.cpp \
                 $(PARSER)/parser_declarations.cpp \
                 $(PARSER)/parser_statements.cpp \
                 $(PARSER)/parser_control_flow.cpp \
                 $(PARSER)/parser_expressions.cpp \
                 $(PARSER)/parser_types.cpp \
                 $(TREE)/ast.cpp \
                 $(TREE)/unparse.cpp \
                 $(LEXER)/lexer.cpp \
                 $(STRING)/satellite_string.cpp

# AND ON THE PROGRAMS IN example/, the same argument the three rules above make
# and the fourth place it is made. section_examples() writes all four
# acceptance programs as `.satc` files and checks the comment column against
# the trie; editing one must re-run the test that reads it.
$(TESTS)/satc_test/satc_test: $(satc_test_SRCS) $(satc_test_HDRS) \
                              $(SATC_TEST_SRCS) $(WORDS)/words.def $(HDRS) \
                              $(wildcard example/*.satl) .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I$(TESTS)/satc_test -o $@ \
	    $(satc_test_SRCS) $(SATC_TEST_SRCS)

# The run list is written out rather than derived, because it is an ORDER and
# not a set. What it can no longer do is run a binary nobody built.
#
# WORD_NUMBERS.md IS PASSED AS AN ARGUMENT rather than found. The test defaults
# to the tree root, which is where make runs, and taking the path lets the
# binary be run from anywhere -- including from an editor, which is where a
# failing transcription is most likely to be looked at.
test: $(TESTBINS)
	./$(TESTS)/words_test/words_test WORD_NUMBERS.md
	./$(TESTS)/lexer_test/lexer_test example/hello_world.satl
	./$(TESTS)/parser_test/parser_test example
	./$(TESTS)/satc_test/satc_test example

# Keeps `make words_test` working, which is what fingers type.
words_test: $(TESTS)/words_test/words_test
lexer_test: $(TESTS)/lexer_test/lexer_test
parser_test: $(TESTS)/parser_test/parser_test
satc_test: $(TESTS)/satc_test/satc_test

TESTALIASES = words_test lexer_test parser_test satc_test

.PHONY: test $(TESTALIASES)
