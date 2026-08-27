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
RANDOM   = $(SRC)/satellite_random

# Declared here and empty until M2, so that adding the trie is one line in this
# file and one in 040-sources.mk rather than a hunt through the build:
#   WORDS  = $(SRC)/satellite_words
