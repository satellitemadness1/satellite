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
#
# AND THE REPORTER, FROM M5 ON. A Token carries an errors::Code and
# diagnostics_of() turns an Error token into a rendered block, so the lexer no
# longer links alone -- which is the same thing that happened to the parser when
# it gained the tree. suggest.cpp is in the list because report.hpp includes it;
# no lexical diagnostic offers a suggestion, and linking only what is called is
# how a test starts failing on an unrelated edit.
LEXER_TEST_SRCS = $(LEXER)/lexer.cpp $(STRING)/satellite_string.cpp \
                  $(ERRORS)/report.cpp $(ERRORS)/suggest.cpp

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
PARSER_TEST_SRCS = $(ERRORS)/report.cpp \
                   $(ERRORS)/suggest.cpp \
                   $(PARSER)/parser.cpp \
                   $(PARSER)/parser_declarations.cpp \
                   $(PARSER)/parser_statements.cpp \
                   $(PARSER)/parser_control_flow.cpp \
                   $(PARSER)/parser_expressions.cpp \
                   $(PARSER)/parser_types.cpp \
                   $(TREE)/ast.cpp \
                   $(TREE)/unparse.cpp \
                   $(TREE)/unparse_declarations.cpp \
                   $(TREE)/unparse_expressions.cpp \
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
SATC_TEST_SRCS = $(ERRORS)/report.cpp \
                 $(ERRORS)/suggest.cpp \
                 $(CACHE)/paths.cpp \
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
                 $(TREE)/unparse_declarations.cpp \
                 $(TREE)/unparse_expressions.cpp \
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

# reporter_test LINKS THE THREE THINGS A MESSAGE IS MADE OF and nothing else,
# and the short list is the subject showing through: DESIGN §9's reporter takes
# a Span, which is three integers, and the text those integers index -- it knows
# nothing about a token or a tree. So this test builds diagnostics by hand and
# renders them, which is the only way to reach the arms no pass produces yet:
# a call stack (M9's), a note with no span, and a span past the end of its text.
#
# AND IT LINKS THE LEXER AND THE PARSER TOO, because half of what M5 is for is
# that the two of them now report through this module -- a reporter that is only
# ever tested against diagnostics the test wrote itself is a reporter tested
# against nobody's real output.
REPORTER_TEST_SRCS = $(ERRORS)/report.cpp \
                     $(ERRORS)/suggest.cpp \
                     $(ERRORS)/dump.cpp \
                     $(PARSER)/parser.cpp \
                     $(PARSER)/parser_declarations.cpp \
                     $(PARSER)/parser_statements.cpp \
                     $(PARSER)/parser_control_flow.cpp \
                     $(PARSER)/parser_expressions.cpp \
                     $(PARSER)/parser_types.cpp \
                     $(TREE)/ast.cpp \
                     $(TREE)/unparse.cpp \
                     $(TREE)/unparse_declarations.cpp \
                     $(TREE)/unparse_expressions.cpp \
                     $(LEXER)/lexer.cpp \
                     $(STRING)/satellite_string.cpp

# AND ON errors.def, which is the unusual prerequisite and is the same argument
# words_test makes for WORD_NUMBERS.md. This test's subject is the message
# registry, so the registry is an input to it in exactly the way a source file
# is: adding a code must re-run the test that counts them.
$(TESTS)/reporter_test/reporter_test: $(reporter_test_SRCS) $(reporter_test_HDRS) \
                                      $(REPORTER_TEST_SRCS) $(ERRORS)/errors.def \
                                      $(WORDS)/words.def $(HDRS) \
                                      $(wildcard example/*.satl) .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I$(TESTS)/reporter_test -o $@ \
	    $(reporter_test_SRCS) $(REPORTER_TEST_SRCS)

# limits_test LINKS TWO MODULES AND NO PARSER, which is the shortest link list
# since words_test and is the subject showing through again: PLAN M6's seam is
# "the readers are here, and satellite.system's twenty-eight paths are M20", so
# nothing under machine_limits/ or system_facts/ knows what a satellite program
# is. The reporter is here because a malformed config is refused through M5's
# codes, and satellite_words/ arrives as headers alone -- the four dial names
# come out of the constexpr node table, which is the property 040-sources.mk
# keeps that module for.
#
# $(PROGRAMS)/check_command.cpp IS THE ONE THAT LOOKS WRONG AND IS NOT.
# limits.cpp calls open_source() to read a config, because "what to say when a
# file will not open" is one fact and S0401 is where it lives; check_command.cpp
# is where that function is defined. It drags the parser in behind it, which is
# why THIS list has a parser in it while the paragraph above says the subject
# does not -- a seam in the code that the linker does not see. Named rather than
# tidied, because the alternative is a second sentence about an unreadable file.
LIMITS_TEST_SRCS = $(LIMITS)/limits.cpp \
                   $(LIMITS)/config.cpp \
                   $(LIMITS)/pool.cpp \
                   $(LIMITS)/watchdog.cpp \
                   $(LIMITS)/dump.cpp \
                   $(SYSTEM)/memory_facts.cpp \
                   $(SYSTEM)/host_facts.cpp \
                   $(SYSTEM)/stack_facts.cpp \
                   $(ERRORS)/report.cpp \
                   $(ERRORS)/suggest.cpp \
                   $(PROGRAMS)/check_command.cpp \
                   $(PROGRAMS)/source_file.cpp \
                   $(PARSER)/parser.cpp \
                   $(PARSER)/parser_declarations.cpp \
                   $(PARSER)/parser_statements.cpp \
                   $(PARSER)/parser_control_flow.cpp \
                   $(PARSER)/parser_expressions.cpp \
                   $(PARSER)/parser_types.cpp \
                   $(TREE)/ast.cpp \
                   $(TREE)/unparse.cpp \
                   $(TREE)/unparse_declarations.cpp \
                   $(TREE)/unparse_expressions.cpp \
                   $(LEXER)/lexer.cpp \
                   $(STRING)/satellite_string.cpp

# AND ON errors.def AND ON BOTH CONFIG FILES IN example/, which is the same
# argument words_test makes for WORD_NUMBERS.md and parser_test for its six
# programs -- and the second of the two is the strong one. section_examples
# reads example/satellite_config.ini as the milestone's done-when and
# example/broken_config.ini as a file that must NOT read, with one assertion per
# line of it. Editing either must re-run the test, INCLUDING the one that fails,
# because a file that starts reading is a finding and not a pass.
$(TESTS)/limits_test/limits_test: $(limits_test_SRCS) $(limits_test_HDRS) \
                                  $(LIMITS_TEST_SRCS) $(ERRORS)/errors.def \
                                  $(WORDS)/words.def $(HDRS) \
                                  example/satellite_config.ini \
                                  example/broken_config.ini .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I$(TESTS)/limits_test -o $@ \
	    $(limits_test_SRCS) $(LIMITS_TEST_SRCS)

# resolve_test LINKS THE MOST OF ANY TEST, and every one of the additions is a
# property of the subject rather than of the test. Resolve runs over a parse
# tree, so it needs the parser, the parser needs the lexer, the lexer needs the
# alphabet -- that much satc_test already had. What it adds is the `.satc`
# module and the reporter's suggester, and the two are there for the same
# reason: this pass READS what M4.5 wrote.
#
# THE CACHE IS LINKED BECAUSE THE SKIP IS THE MILESTONE'S MEASURED CLAUSE.
# MILESTONES/M4.5.md §5 says "M7's resolve has to learn to skip a path the
# `.satc` has already numbered", and tests/resolve_test/cache.cpp proves it by
# writing a `.satc` from a tree, reading it back, and counting the walks the
# marks saved -- which cannot be done without the writer and the reader in the
# same binary. satellite_cache/paths.cpp is linked for a second reason besides:
# it is the trie walk itself, which resolve reads rather than writing again.
#
# NOT $(SATL_OBJS), for the fifth time and for the same reason: linking the
# interpreter's objects would drag main.o and its window handover into a test
# binary, and a test that starts by deciding whether to open a GUI hangs on a
# build machine.
RESOLVE_TEST_SRCS = $(RESOLVE)/resolve.cpp \
                    $(RESOLVE)/scopes.cpp \
                    $(RESOLVE)/walk.cpp \
                    $(RESOLVE)/names.cpp \
                    $(RESOLVE)/numbers.cpp \
                    $(RESOLVE)/dump.cpp \
                    $(ERRORS)/report.cpp \
                    $(ERRORS)/suggest.cpp \
                    $(CACHE)/paths.cpp \
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
                    $(TREE)/unparse_declarations.cpp \
                    $(TREE)/unparse_expressions.cpp \
                    $(LEXER)/lexer.cpp \
                    $(STRING)/satellite_string.cpp

# AND ON errors.def AND ON THE PROGRAMS IN example/, which is the same argument
# the five rules above make and the sixth place it is made. section_examples()
# resolves all five acceptance programs and reads example/frames.satl clause by
# clause as this milestone's done-when; section_frames() and the rest assert one
# code per row of errors.def's S05xx block. Editing either must re-run the test.
$(TESTS)/resolve_test/resolve_test: $(resolve_test_SRCS) $(resolve_test_HDRS) \
                                    $(RESOLVE_TEST_SRCS) $(ERRORS)/errors.def \
                                    $(WORDS)/words.def $(HDRS) \
                                    $(wildcard example/*.satl) .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I$(TESTS)/resolve_test -o $@ \
	    $(resolve_test_SRCS) $(RESOLVE_TEST_SRCS)

# eval_test LINKS THE MOST OF ANY SUITE, AND ONE MODULE IT DELIBERATELY DOES
# NOT. An evaluator runs over a compiled tree, which needs the parser, which
# needs the lexer and the alphabet, which needs resolve for the slots and the
# numbers -- so the list below is resolve_test's plus satellite_value/,
# satellite_number/ and evaluator/. That is a property of the SUBJECT, the same
# way words_test linking nothing is a property of the registry.
#
# WHAT IS MISSING IS machine_limits, AND IT IS THE MOST IMPORTANT LINE IN THIS
# RULE. MILESTONES/M8.5.md §4.1 is the receipt: a 20,000-deep resolve fixture
# passed for a day against a raised RLIMIT_STACK it had never been given,
# because resolve_test does not link the module that raises one, and the note on
# the fixture claimed otherwise. tests/eval_test/depth.cpp recurses 1,000,000
# frames deep and that number is worth nothing unless the C++ stack is the 8 MiB
# a login shell hands out. evaluator/machine.hpp's Policy is what makes the
# omission possible: the ceiling and the division digits are HANDED IN, so the
# evaluator never reads machine_limits and a test can pass its own.
#
# IT ALSO LINKS NO satellite_cache, AND THAT IS THE OTHER HALF OF THE SAME CARE.
# programs/evaluate_commands.cpp explains why a command that reports at RUN time
# parses the source rather than reading a `.satc`: a cached tree's spans index
# the cache, where line 11 of hello_world.satl is line 10. A test that fed the
# evaluator a cached tree would be checking carets against the wrong file.
EVAL_TEST_SRCS = $(EVAL)/evaluate.cpp \
                 $(EVAL)/compile.cpp \
                 $(EVAL)/compile_expressions.cpp \
                 $(EVAL)/compile_statements.cpp \
                 $(EVAL)/machine.cpp \
                 $(EVAL)/operations.cpp \
                 $(EVAL)/operations_control.cpp \
                 $(EVAL)/operations_dispatch.cpp \
                 $(EVAL)/dispatch.cpp \
                 $(SCALARS)/handlers.cpp \
                 $(SCALARS)/string_methods.cpp \
                 $(SCALARS)/number_methods.cpp \
                 $(SCALARS)/variant_methods.cpp \
                 $(EVAL)/dump.cpp \
                 $(VALUE)/value.cpp \
                 $(VALUE)/render.cpp \
                 $(NUMBER)/limbs.cpp \
                 $(NUMBER)/number_core.cpp \
                 $(NUMBER)/number_query.cpp \
                 $(NUMBER)/number_arith.cpp \
                 $(NUMBER)/render.cpp \
                 $(NUMBER)/random.cpp \
                 $(SYSTEM)/host_facts.cpp \
                 $(SYSTEM)/memory_facts.cpp \
                 $(SYSTEM)/user_facts.cpp \
                 $(RESOLVE)/resolve.cpp \
                 $(RESOLVE)/scopes.cpp \
                 $(RESOLVE)/walk.cpp \
                 $(RESOLVE)/names.cpp \
                 $(RESOLVE)/numbers.cpp \
                 $(ERRORS)/report.cpp \
                 $(ERRORS)/suggest.cpp \
                 $(CACHE)/paths.cpp \
                 $(PARSER)/parser.cpp \
                 $(PARSER)/parser_declarations.cpp \
                 $(PARSER)/parser_statements.cpp \
                 $(PARSER)/parser_control_flow.cpp \
                 $(PARSER)/parser_expressions.cpp \
                 $(PARSER)/parser_types.cpp \
                 $(TREE)/ast.cpp \
                 $(LEXER)/lexer.cpp \
                 $(STRING)/satellite_string.cpp

# AND ON errors.def, which is the seventh place this argument is made:
# tests/eval_test asserts one program per row of the S07xx block this milestone
# populated, so editing a sentence must re-run the test that raises it.
$(TESTS)/eval_test/eval_test: $(eval_test_SRCS) $(eval_test_HDRS) \
                              $(EVAL_TEST_SRCS) $(ERRORS)/errors.def \
                              $(WORDS)/words.def $(HDRS) .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I$(TESTS)/eval_test -o $@ \
	    $(eval_test_SRCS) $(EVAL_TEST_SRCS)

# console_test LINKS eval_test's LIST PLUS THE CONSOLE, AND ONE THING THAT SUITE
# LEAVES OUT. M10's row in `handlers[path_id]` is reached the way a program
# reaches it -- parse, resolve, compile, dispatch -- so everything the evaluator
# needs is here too; what is added is satellite_console/, which is the subject.
#
# IT LINKS NO machine_limits EITHER, and for once that is not the reason
# eval_test has. Nothing here is about depth, so M8.5 §4.1's receipt does not
# apply; the console module simply does not obey a limit -- DESIGN §10.1's queue
# is unbounded on purpose and "the bound is memory, the way a list's is". A
# console that had to be told a ceiling would be the hidden constant PLAN §8's
# M10 entry refuses.
#
# AND IT REDIRECTS ITS OWN STDOUT, which is why this suite exists rather than
# six more sections in eval_test. A test binary that starts a printer thread and
# writes to descriptor 1 has to take that descriptor away and give it back
# around every fixture; mixing that with a suite whose other sections print
# nothing is how one failing section makes another look broken.
CONSOLE_TEST_SRCS = $(CONSOLE)/console.cpp \
                    $(CONSOLE)/handlers.cpp \
                    $(EVAL_TEST_SRCS)

$(TESTS)/console_test/console_test: $(console_test_SRCS) $(console_test_HDRS) \
                                    $(CONSOLE_TEST_SRCS) $(ERRORS)/errors.def \
                                    $(WORDS)/words.def $(HDRS) .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -I$(TESTS)/console_test -o $@ \
	    $(console_test_SRCS) $(CONSOLE_TEST_SRCS)

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
	./$(TESTS)/reporter_test/reporter_test example
	./$(TESTS)/limits_test/limits_test example
	./$(TESTS)/resolve_test/resolve_test example
	./$(TESTS)/number_test/number_test
	./$(TESTS)/eval_test/eval_test example
	./$(TESTS)/console_test/console_test

# Keeps `make words_test` working, which is what fingers type.
words_test: $(TESTS)/words_test/words_test
lexer_test: $(TESTS)/lexer_test/lexer_test
parser_test: $(TESTS)/parser_test/parser_test
satc_test: $(TESTS)/satc_test/satc_test
reporter_test: $(TESTS)/reporter_test/reporter_test
limits_test: $(TESTS)/limits_test/limits_test
resolve_test: $(TESTS)/resolve_test/resolve_test

# number_test -- PLAN M8. DESIGN §8.1's exact decimal, and the sign the port
# did not bring with it.
#
# IT LINKS $(RANDOM)/random.cpp AND IT IS THE FIRST THING IN THE TREE TO. That
# module has been "the one module with no consumer" in LAYOUT.md since it landed
# at M2 -- compiled by `all`, linked into nothing -- because the thing that would
# call it needed the arbitrary-precision half M8 has now ported. This rule is
# what closes that sentence, and tests/number_test/draw.cpp is the section.
#
# -isystem pcg/include COMES WITH IT, and it is the same one 060-compile.mk puts
# on that file's own explicit rule. pcg_extras.hpp warns under this build's
# -Wall -Wextra and the headers are somebody else's; -isystem is what stops a
# vendored warning failing a build nobody can fix. The path is on THIS rule and
# not on CXXFLAGS for the same reason -I$(SRC) is not: it belongs to the one
# translation unit that needs it.
#
# WHY random.cpp AT ALL WHEN THE TEST DRIVES THE SEAM WITH ITS OWN STUB.
# Bits32's destructor is out of line -- random.cpp's own comment says why, "so
# the vtable has one home rather than one per translation unit" -- so a binary
# holding any Bits32 needs that object even when no PCG is ever constructed.
NUMBER_TEST_SRCS = $(NUMBER)/limbs.cpp \
                   $(NUMBER)/number_core.cpp \
                   $(NUMBER)/number_query.cpp \
                   $(NUMBER)/number_arith.cpp \
                   $(NUMBER)/render.cpp \
                   $(NUMBER)/random.cpp \
                   $(RANDOM)/random.cpp \
                   $(LIMITS)/limits.cpp \
                   $(LIMITS)/config.cpp \
                   $(LIMITS)/pool.cpp \
                   $(LIMITS)/watchdog.cpp \
                   $(LIMITS)/dump.cpp \
                   $(ERRORS)/report.cpp \
                   $(ERRORS)/suggest.cpp \
                   $(SYSTEM)/memory_facts.cpp \
                   $(SYSTEM)/host_facts.cpp \
                   $(SYSTEM)/stack_facts.cpp \
                   $(PROGRAMS)/check_command.cpp \
                   $(PROGRAMS)/source_file.cpp \
                   $(PROGRAMS)/opening.cpp \
                   $(LEXER)/lexer.cpp \
                   $(STRING)/satellite_string.cpp \
                   $(PARSER)/parser.cpp \
                   $(PARSER)/parser_declarations.cpp \
                   $(PARSER)/parser_statements.cpp \
                   $(PARSER)/parser_control_flow.cpp \
                   $(PARSER)/parser_expressions.cpp \
                   $(PARSER)/parser_types.cpp \
                   $(TREE)/ast.cpp \
                   $(TREE)/unparse.cpp \
                   $(TREE)/unparse_declarations.cpp \
                   $(TREE)/unparse_expressions.cpp \
                   $(CACHE)/paths.cpp

$(TESTS)/number_test/number_test: $(number_test_SRCS) $(number_test_HDRS) \
                                  $(NUMBER_TEST_SRCS) $(ERRORS)/errors.def \
                                  $(WORDS)/words.def $(HDRS) .cxxflags-stamp
	$(CXX) $(CXXFLAGS) -I$(SRC) -isystem pcg/include \
	    -I$(TESTS)/number_test -o $@ \
	    $(number_test_SRCS) $(NUMBER_TEST_SRCS)

number_test: $(TESTS)/number_test/number_test

TESTALIASES = words_test lexer_test parser_test satc_test reporter_test \
              limits_test resolve_test number_test eval_test

.PHONY: test $(TESTALIASES)
