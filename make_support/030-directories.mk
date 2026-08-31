# satellite -- one variable per module directory.
#
# Every path in every fragment below is spelled through one of these names, so a
# directory that moves is one edit here.
#
# Every module lives in its own directory under src/, named for the job the
# module does rather than for the abbreviation its files use: abstract_syntax_tree
# and not ast, lexical_analyzer and not lex. Spell it out, or put satellite_ in
# front of it. FILE names inside a module are free to stay short.

SRC      = src
SYSTEM   = $(SRC)/system_facts
PROGRAMS = $(SRC)/programs
ERRORS   = $(SRC)/error_reporter
LEXER    = $(SRC)/lexical_analyzer
CACHE    = $(SRC)/satellite_cache
LIMITS   = $(SRC)/machine_limits
PARSER   = $(SRC)/parser
RANDOM   = $(SRC)/satellite_random
STRING   = $(SRC)/satellite_string
TREE     = $(SRC)/abstract_syntax_tree
WORDS    = $(SRC)/satellite_words

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
            limits_test
