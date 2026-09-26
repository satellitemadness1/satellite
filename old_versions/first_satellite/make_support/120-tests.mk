# satellite -- the fifteen test binaries, plus the sanitized sixteenth.
#
# Every per-test source list is a wildcard over $(TESTS)/<name>/, driven by
# TESTNAMES in 070-directories.mk, so splitting a test into more files needs no
# edit here. TESTALIASES is defined at the bottom and read by .PHONY in
# 160-uninstall-clean.mk, which is why that fragment is included after this one.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# --- what a test is built from ----------------------------------------------
# EVERY .cpp in a test's folder is part of that test. That is the rule, so it is
# a wildcard rather than fifteen hand-written lists: a test that gets split into
# six files needs no Makefile edit, and -- more to the point -- a new piece
# CANNOT be forgotten. .gitignore's note records what the hand-copied list cost
# twice; this is the same lesson applied one level up.
#
# $(wildcard) expands when the Makefile is read, so a file added during a build
# is picked up by the next one. That is the only cost and it is the right trade.
$(foreach t,$(TESTNAMES),$(eval $(t)_SRCS = $$(wildcard $$(TESTS)/$(t)/*.cpp)))

# The HEADERS are a separate list because they are a DEPENDENCY and not an
# input: a split test grew a <name>.hpp holding the harness declarations and the
# section prototypes, and without this a change to that header would not relink
# the binary. Found by the audit of the 2026-08-24 split, which is exactly the
# kind of stale-build hazard that only shows up as a confusing test result later.
$(foreach t,$(TESTNAMES),$(eval $(t)_HDRS = $$(wildcard $$(TESTS)/$(t)/*.hpp)))

$(TESTS)/library_test/library_test: $(library_test_SRCS) $(library_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(library_test_SRCS) $(LIBOBJS)

# The one test that does NOT follow $(OPT), and deliberately: a sanitizer build
# wants -O1 and -g, because the instrumentation is what is being run and a
# report without line numbers is not one. $(OPT) is the knob for the code that
# ships, and this binary does not ship.
$(TESTS)/library_test/library_test_tsan: $(library_test_SRCS) $(TESTSRCS) $(HDRS)
	$(TSAN_CXX) $(TESTFLAGS) $(PCGFLAGS) -I$(SRC) -I. -O1 -g -fsanitize=thread \
	    -o $@ $(library_test_SRCS) $(TESTSRCS)

$(TESTS)/satellite_string_test/satellite_string_test: $(satellite_string_test_SRCS) $(satellite_string_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(satellite_string_test_SRCS) $(LIBOBJS)

$(TESTS)/lexer_test/lexer_test: $(lexer_test_SRCS) $(lexer_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(lexer_test_SRCS) $(LIBOBJS)

$(TESTS)/ast_test/ast_test: $(ast_test_SRCS) $(ast_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(ast_test_SRCS) $(LIBOBJS)

$(TESTS)/parser_test/parser_test: $(parser_test_SRCS) $(parser_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(parser_test_SRCS) $(LIBOBJS)

$(TESTS)/eval_test/eval_test: $(eval_test_SRCS) $(eval_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(eval_test_SRCS) $(LIBOBJS)

$(TESTS)/search_test/search_test: $(search_test_SRCS) $(search_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(search_test_SRCS) $(LIBOBJS)

# The one test that needs -I. as well, for the same reason $(ORBIT)'s compile
# rule does: orbit is not under src/, so its headers are reached from the repo
# root. Everything else about the recipe is the others' verbatim.
$(TESTS)/orbit_test/orbit_test: $(orbit_test_SRCS) $(orbit_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) -I. $(OPT) -o $@ $(orbit_test_SRCS) $(LIBOBJS)

$(TESTS)/interp_test/interp_test: $(interp_test_SRCS) $(interp_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(interp_test_SRCS) $(LIBOBJS)

$(TESTS)/loader_test/loader_test: $(loader_test_SRCS) $(loader_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(loader_test_SRCS) $(LIBOBJS)

$(TESTS)/env_test/env_test: $(env_test_SRCS) $(env_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(env_test_SRCS) $(LIBOBJS)

$(TESTS)/spacesuit_test/spacesuit_test: $(spacesuit_test_SRCS) $(spacesuit_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(spacesuit_test_SRCS) $(LIBOBJS)

$(TESTS)/bignum_test/bignum_test: $(bignum_test_SRCS) $(bignum_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(bignum_test_SRCS) $(LIBOBJS)

# The one test binary whose runtime is a design parameter rather than an
# accident: every end-to-end case spends its tier's throwaway window before it
# answers, so the cases here are on `fast` (50-100 ms) and the sampler itself is
# tested through a stub generator that does not spin at all.
$(TESTS)/random_test/random_test: $(random_test_SRCS) $(random_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(random_test_SRCS) $(LIBOBJS)

# format.hpp links against NOTHING — it includes only <cstdint> and <cstddef>,
# so this is the one test binary that needs no objects at all. That is a
# property of the format and worth keeping: the registry must be readable by a
# disassembler, a loader, or the bootstrap's generated C, none of which should
# have to drag the interpreter in to learn what id 7 is.
#
# Most of this test runs at COMPILE time. format.hpp ends in static_asserts over
# the X-macro lists, so a duplicate id or a hole in the registry fails right
# here rather than in the binary.
$(TESTS)/format_test/format_test: $(format_test_SRCS) $(format_test_HDRS) $(FORMAT)/format.hpp \
                    $(FORMAT)/format.def
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(format_test_SRCS)

$(TESTS)/console_test/console_test: $(console_test_SRCS) $(console_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(console_test_SRCS) $(LIBOBJS)

# console_output's other half. Every piece of console_input/ except the read
# loop itself is a pure function -- bytes to keys, keys to a line, a line to
# escape sequences -- which is exactly so that this binary can drive all of it
# with no terminal anywhere. The one file it cannot reach is line_reader.cpp,
# and that is the point of the split rather than a gap in the test.
$(TESTS)/console_input_test/console_input_test: $(console_input_test_SRCS) $(console_input_test_HDRS) $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(console_input_test_SRCS) $(LIBOBJS)

# reg.hpp is not linked into satl: there is no VM yet, and nothing in the
# interpreter includes it. This binary is the only consumer. (This comment sat
# over console_test until the move; it was always describing this recipe.)
$(TESTS)/reg_test/reg_test: $(reg_test_SRCS) $(reg_test_HDRS) $(REG)/reg.hpp $(LIBOBJS)
	$(CXX) $(TESTFLAGS) -I$(SRC) $(OPT) -o $@ $(reg_test_SRCS) $(LIBOBJS)

# EVERY test binary, DERIVED FROM TESTNAMES and never spelled out again.
#
# It WAS spelled out, and it cost exactly what .gitignore's own note says a
# hand-copied list costs. search_test and orbit_test were both added to the run
# list below and to neither this one -- so `make test` RAN them without ever
# BUILDING them, and reported PASS from a binary compiled before the change
# under test. That is the worst failure a suite has: not a red line, a green one
# that is out of date. Found on 2026-08-24 when a renamed field should have
# broken orbit_test and the suite went on passing.
#
# TESTNAMES in 070-directories.mk is the single place a test is declared to
# exist, and this is now one of the things derived from it, like the per-test
# source wildcards and the aliases. A test that exists is a test that is built.
TESTBINS = $(foreach t,$(TESTNAMES),$(TESTS)/$(t)/$(t))

# The run order is still written out, because it is an ORDER and not a set:
# library_test goes first and its sanitized twin second, and a foreach cannot
# say that. What it can no longer do is run a binary nobody built.
test: $(TESTBINS) $(TSAN_TEST)
	./$(TESTS)/library_test/library_test
	$(if $(TSAN_TEST),./$(TSAN_TEST))
	./$(TESTS)/satellite_string_test/satellite_string_test
	./$(TESTS)/bignum_test/bignum_test
	./$(TESTS)/random_test/random_test
	./$(TESTS)/format_test/format_test
	./$(TESTS)/reg_test/reg_test
	./$(TESTS)/console_test/console_test
	./$(TESTS)/console_input_test/console_input_test
	./$(TESTS)/lexer_test/lexer_test
	./$(TESTS)/ast_test/ast_test
	./$(TESTS)/parser_test/parser_test
	./$(TESTS)/env_test/env_test
	./$(TESTS)/eval_test/eval_test
	./$(TESTS)/search_test/search_test
	./$(TESTS)/orbit_test/orbit_test
	./$(TESTS)/interp_test/interp_test
	./$(TESTS)/loader_test/loader_test
	./$(TESTS)/spacesuit_test/spacesuit_test

# A test binary lives under $(TESTS)/<name>/, so its path is no longer its name.
# These keep `make ast_test` working, which is what fingers already type and
# what every note in DESIGN.md that mentions running one still says. Each is
# phony and does nothing but ask for the real path.
ast_test:              $(TESTS)/ast_test/ast_test
bignum_test:           $(TESTS)/bignum_test/bignum_test
console_test:          $(TESTS)/console_test/console_test
console_input_test:    $(TESTS)/console_input_test/console_input_test
env_test:              $(TESTS)/env_test/env_test
eval_test:             $(TESTS)/eval_test/eval_test
format_test:           $(TESTS)/format_test/format_test
interp_test:           $(TESTS)/interp_test/interp_test
lexer_test:            $(TESTS)/lexer_test/lexer_test
library_test:          $(TESTS)/library_test/library_test
library_test_tsan:     $(TESTS)/library_test/library_test_tsan
loader_test:           $(TESTS)/loader_test/loader_test
parser_test:           $(TESTS)/parser_test/parser_test
search_test:           $(TESTS)/search_test/search_test
orbit_test:            $(TESTS)/orbit_test/orbit_test
random_test:           $(TESTS)/random_test/random_test
reg_test:              $(TESTS)/reg_test/reg_test
satellite_string_test: $(TESTS)/satellite_string_test/satellite_string_test
spacesuit_test:        $(TESTS)/spacesuit_test/spacesuit_test

TESTALIASES = ast_test bignum_test console_test console_input_test env_test eval_test \
              format_test interp_test lexer_test library_test \
              library_test_tsan loader_test parser_test random_test \
              reg_test satellite_string_test spacesuit_test search_test \
              orbit_test
