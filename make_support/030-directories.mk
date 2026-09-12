# satellite -- one variable per module directory.
#
# Every path in every fragment below is spelled through one of these names, so a
# directory that moves is one edit here.
#
# Every module lives in its own directory under src/, named for the job the
# module does rather than for the abbreviation its files use: abstract_syntax_tree
# and not ast, lexical_analyzer and not lex. Spell it out, or put satellite_ in
# front of it. FILE names inside a module are free to stay short.
#
# TERM_DIR IS THE ONE DIRECTORY INSIDE ANOTHER, and it is a program rather
# than a module: src/programs/satl-term/ holds the satl-term binary and nothing
# else -- the command line and the GtkApplication, the VTE widget and its child,
# and what the keyboard means. It is NAMED FOR THE BINARY IT BUILDS, which is
# the rule above applied to a program: `satl-term` and not `term`, so that the
# folder and the thing it produces cannot be told apart. Spelled through
# $(PROGRAMS) rather than $(SRC), so moving programs/ moves it too.
#
# TERM_DIR AND NOT TERM. Every login shell exports TERM, and a makefile
# assignment beats the environment -- except under `make -e`, where it does not,
# and this directory would quietly become `xterm-256color`. A build variable
# does not get to share a name with one the terminal already owns.
#
# SATFILE AND NOT FILE, DIRECTRY AND NOT DIRECTORY, and both are the same rule
# one step further out. `FILE` is a C standard library type and `DIRECTORY` is
# close enough to `DIR` and to the shell's own vocabulary to be worth avoiding;
# neither is exported by a login shell the way TERM is, so neither is the exact
# hazard above -- these are named for legibility at a `make -p` dump, where a
# variable called FILE beside a rule about files reads as a pattern stem. The
# directories themselves are spelled out in full, which is the rule that
# matters: src/satellite_file/ and src/satellite_directory/.

SRC      = src
SYSTEM   = $(SRC)/system_facts
PROGRAMS = $(SRC)/programs
TERM_DIR = $(PROGRAMS)/satl-term
ARGS     = $(SRC)/satellite_arguments
ERRORS   = $(SRC)/error_reporter
EVAL     = $(SRC)/evaluator
DIAGNOSE = $(SRC)/program_diagnostics
LEXER    = $(SRC)/lexical_analyzer
BITS     = $(SRC)/satellite_bits
CACHE    = $(SRC)/satellite_cache
CONSOLE  = $(SRC)/satellite_console
CONTAIN  = $(SRC)/satellite_containers
DIRECTRY = $(SRC)/satellite_directory
SATFILE  = $(SRC)/satellite_file
FLOAT    = $(SRC)/satellite_float
HELP     = $(SRC)/satellite_help
SCALARS  = $(SRC)/satellite_scalars
LIMITS   = $(SRC)/machine_limits
NUMBER   = $(SRC)/satellite_number
PARSER   = $(SRC)/parser
PROMPT   = $(SRC)/satellite_prompt
RESOLVE  = $(SRC)/name_resolver
RANDOM   = $(SRC)/satellite_random
STRING   = $(SRC)/satellite_string
SYSLIB   = $(SRC)/satellite_system
THREAD   = $(SRC)/satellite_thread
TIME     = $(SRC)/satellite_time
VALUE    = $(SRC)/satellite_value
TREE     = $(SRC)/abstract_syntax_tree
WORDS    = $(SRC)/satellite_words

# NOT UNDER $(SRC), AND THAT IS THE POINT. help_lines/ is the GENERATOR for
# HELP.md and src/satellite_help/help.def; nothing in it is compiled into satl.
# `make nodes` (065-tests.mk) is the one target that reaches in here.
HELP_LINES = help_lines

# The tests root, and TESTNAMES is THE SINGLE PLACE A TEST IS DECLARED TO EXIST.
# 065-tests.mk derives everything else from it -- the per-test source lists, the
# binaries, the run list, the aliases and what `clean` removes.
#
# DERIVED AND NEVER HAND-COPIED, and the first satellite paid for that rule
# twice: its test binary list WAS written out by hand, two tests were added to
# the run list and to neither build list, and `make test` ran binaries nobody
# had built -- reporting PASS from objects compiled before the change under
# test. That is the worst failure a suite has: not a red line, a green one that
# is out of date.
TESTS     = tests
TESTNAMES = words_test lexer_test parser_test satc_test reporter_test \
            limits_test resolve_test number_test float_test eval_test \
            console_test prompt_test help_test file_test
