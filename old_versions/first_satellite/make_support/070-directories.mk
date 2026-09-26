# satellite -- one variable per module directory, and the tests root.
#
# Every path in every fragment below is spelled through one of these names, so
# a directory that moves is one edit here. TESTNAMES is the single place a test
# is declared to exist; 120-tests.mk builds everything else from it.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# Every module lives in its own directory under src/, named for the job the
# module does rather than for the abbreviation its files still use. The FILE
# names are deliberately unchanged -- ast.cpp is still ast.cpp -- so the
# hundreds of references in DESIGN.md that name a file still name the right
# file, and only the directory in front of it is new.
#
# These are variables rather than spelled-out paths so that the explicit source
# lists below stay one file per name and readable at a glance, which is the
# property the note on ENV_SRCS is about.
SRC      = src
AST      = $(SRC)/abstract_syntax_tree
NUMBER   = $(SRC)/satellite_number
STRING   = $(SRC)/satellite_string
VALUE    = $(SRC)/satellite_value
LIBRARY  = $(SRC)/satellite_library
LEXER    = $(SRC)/lexical_analyzer
PARSER   = $(SRC)/syntax_parser
ENV      = $(SRC)/environment
EVAL     = $(SRC)/evaluator
INTERP   = $(SRC)/interpreter
LOADER   = $(SRC)/spaceship_loader
CONSOLE  = $(SRC)/console_output
INPUT    = $(SRC)/console_input
RANDOM   = $(SRC)/random_numbers
SYSTEM   = $(SRC)/system_facts
FORMAT   = $(SRC)/bytecode_format
REG      = $(SRC)/register_file
PROGRAMS = $(SRC)/programs

# Satellite Orbit -- the five-phase resolution pipeline over the search power.
# The ONE module directory that is not under $(SRC), and deliberately: src/ is
# the interpreter, and orbit is a consumer of the search power rather than part
# of evaluating an expression. satellite_orbit_search/orbit_plan.txt, DECISION 1a.
ORBIT    = satellite_orbit_search

# The tests root, and the one directory variable that is NOT under $(SRC).
# Every test lives in satellite_system/tests/<test_name>/, one folder per test
# binary, because a test that has been split into six files needs somewhere to
# put them that is not the module it tests. -I$(SRC) is what still makes their
# includes root-relative, so nothing about how a test includes changed.
TESTS    = satellite_system/tests

# One name per test binary, and the single place a test is declared to exist.
# TESTBINS, the aliases, `test`, `clean` and the per-test source wildcards are
# all derived from this list, so adding a test means adding one word here.
TESTNAMES = library_test satellite_string_test lexer_test ast_test parser_test \
            eval_test interp_test loader_test env_test spacesuit_test \
            bignum_test random_test format_test console_test reg_test \
            console_input_test search_test orbit_test
