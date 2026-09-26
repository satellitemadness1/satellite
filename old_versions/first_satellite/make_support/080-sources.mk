# satellite -- what gets compiled, what gets linked, and what a test links.
#
# Reads the directory names from 070-directories.mk. Nearly every list here is
# a wildcard, which is the whole reason the 2026-08-24 restructure could split
# fifty source files without touching the build. Read the note over $(PROGRAMS)
# before adding a wildcard: it is the one list that must stay spelled out.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# eval.cpp was 2208 lines and was split into evaluator/ at the seams the code
# already had. The 2026-08-24 restructure then put a 325-line ceiling on every
# file and that directory went from twelve files to thirty-one.
#
# These WERE explicit lists, so that a file added and forgotten here would fail
# to link rather than be silently dropped. That reasoning inverted at thirty-one
# files: the list itself became the thing that gets forgotten, and a forgotten
# entry is the same silent drop it was meant to prevent. A wildcard cannot be
# forgotten. $(sort) because $(wildcard) returns directory order, and a link
# line that reorders between machines is one that cannot be compared.
ENV_SRCS = $(sort $(wildcard $(ENV)/*.cpp))

ENV_OBJS = $(ENV_SRCS:.cpp=.o)

BIGNUM_SRCS = $(sort $(wildcard $(NUMBER)/*.cpp))

BIGNUM_OBJS = $(BIGNUM_SRCS:.cpp=.o)

PARSER_SRCS = $(sort $(wildcard $(PARSER)/*.cpp))

PARSER_OBJS = $(PARSER_SRCS:.cpp=.o)

# Every .cpp in the module's folder, for the same reason the tests are a
# wildcard: the 2026-08-24 restructure put a 325-line ceiling on every file, so
# these lists gained ten entries in one afternoon and will gain more. A
# hand-written list is a list that can be forgotten, and a forgotten entry here
# is an undefined reference at link time in the best case and a silently
# unbuilt arm in the worst.
#
# $(sort) because $(wildcard) returns directory order, and a build whose link
# line changes order between machines is a build that cannot be compared.
EVAL_SRCS = $(sort $(wildcard $(EVAL)/*.cpp))
EVAL_OBJS = $(EVAL_SRCS:.cpp=.o)

# Every module that is WHOLLY part of the interpreter is a wildcard, so that
# splitting one of its files needs no edit here. The directories are named one
# per line rather than globbed as src/*/ for the reason immediately below.
LEAF_SRCS = $(sort $(wildcard $(LIBRARY)/*.cpp) $(wildcard $(STRING)/*.cpp) \
                   $(wildcard $(SYSTEM)/*.cpp) $(wildcard $(LEXER)/*.cpp) \
                   $(wildcard $(AST)/*.cpp) $(wildcard $(VALUE)/*.cpp) \
                   $(wildcard $(LOADER)/*.cpp) $(wildcard $(INTERP)/*.cpp) \
                   $(wildcard $(RANDOM)/*.cpp) $(wildcard $(CONSOLE)/*.cpp) \
                   $(wildcard $(INPUT)/*.cpp) $(wildcard $(ORBIT)/*.cpp))
LEAF_OBJS = $(LEAF_SRCS:.cpp=.o)

# $(PROGRAMS) IS NOT A WILDCARD, and must never become one. That directory holds
# main.cpp AND window.cpp, and window.cpp is satl-term -- the GTK/VTE binary.
# §9 measured what linking the GUI costs the interpreter (23.4 ms of startup)
# and the answer was to keep them two binaries; M7's done-when asserts the
# result literally, "ldd satl still lists six objects". A wildcard here would
# put GTK into satl silently, and the only symptom would be a number in a test
# nobody reads until it is far too late.
# main.cpp was 565 lines and is now four files. They are named ONE BY ONE
# rather than wildcarded, for the reason in the paragraph above: this directory
# also holds window.cpp, and a wildcard here would link GTK into satl. A new
# piece of the interpreter's front end has to be added to this line by hand --
# which is the cost of keeping window.cpp out, and it is worth paying.
SATL_MAIN = $(PROGRAMS)/main.o $(PROGRAMS)/main_prompt.o \
            $(PROGRAMS)/main_run.o $(PROGRAMS)/main_repl.o

OBJS      = $(SATL_MAIN) $(LEAF_OBJS) \
            $(EVAL_OBJS) $(PARSER_OBJS) $(BIGNUM_OBJS) $(ENV_OBJS)
HDRS      = $(LIBRARY)/library.hpp $(VALUE)/value.hpp \
            $(STRING)/satellite_string.hpp $(SYSTEM)/system.hpp \
            $(NUMBER)/bignum.hpp $(LEXER)/lexer.hpp $(AST)/ast.hpp \
            $(PARSER)/parser.hpp $(ENV)/env.hpp $(EVAL)/eval.hpp \
            $(LOADER)/loader.hpp $(INTERP)/interp.hpp $(RANDOM)/random.hpp \
            $(CONSOLE)/console.hpp $(INPUT)/console_input.hpp \
            $(INPUT)/keys.hpp $(INPUT)/editor.hpp $(INPUT)/history.hpp \
            $(INPUT)/raw_mode.hpp $(INPUT)/render.hpp \
            $(SYSTEM)/interrupt.hpp $(EVAL)/eval_internal.hpp \
            $(PARSER)/parser_internal.hpp $(NUMBER)/bignum_internal.hpp \
            $(ENV)/env_internal.hpp $(INTERP)/interp_internal.hpp \
            $(SYSTEM)/system_internal.hpp $(EVAL)/helpers_file_facts.hpp \
            $(PROGRAMS)/main_internal.hpp
# version.hpp is not in HDRS either, and for the OPPOSITE reason to format.hpp
# below: three objects DO include it -- main.o, window.o and help.o -- and those
# three name it as a prerequisite on their own rules instead. Every object
# depends on HDRS, so listing it here would rebuild forty-three of them to
# change a string three of them read, which is the same waste that keeps
# VERSION_DEFS off CXXFLAGS twenty lines up. What must NOT happen is what was
# true until this line was written: the header in no rule at all, so editing it
# rebuilt nothing and the binary went on reporting the version it was built
# with. That is format.def's defect exactly, and §17 spends a section on it.
#
# format.hpp and format.def are deliberately NOT in HDRS. Every object depends on
# HDRS, and no object includes either file — there is no VM yet — so listing them
# would make one edit to format.def rebuild the whole interpreter for nothing.
# The format_test rule below names them itself, which is the dependency that is
# actually real. Add them here when a translation unit in OBJS includes them.
# Every source LIBOBJS is built from, as sources rather than objects -- the tsan
# build compiles them itself with -fsanitize=thread rather than reusing the
# ordinary objects. Derived from the same wildcards OBJS uses and NOT restated:
# it was restated once, and when the 325-line split added four files to
# system_facts/ this list did not grow with it and library_test_tsan failed to
# link against a dozen symbols. Same class of drift as .gitignore's, same fix.
TESTSRCS  = $(LEAF_SRCS) $(EVAL_SRCS) $(PARSER_SRCS) $(BIGNUM_SRCS) $(ENV_SRCS)
TESTFLAGS = -std=c++20 -Wall -Wextra -pthread

# Every test binary used to recompile every source. That was tolerable while
# there were eleven of them; splitting eval.cpp into twelve took a serial
# `make test` past ten minutes, because the cost is (test binaries) x (sources)
# and the split doubled the second term. Linking the objects the binary already
# built makes it (sources) + (test binaries).
#
# main.o is excluded because it defines main() and so does every test.
# library_test_tsan is NOT converted: -fsanitize=thread has to be on every
# translation unit it links, and these objects are not built with it.
# The interpreter minus its front end -- what a test binary links. Filters the
# WHOLE of SATL_MAIN and not just main.o: main.cpp's split gave the front end
# three more objects, and a test that linked them would get a second main().
LIBOBJS   = $(filter-out $(SATL_MAIN),$(OBJS))

# Both, on a machine that can build both. On one that cannot, the interpreter
# and an explanation -- see the note on MISSING_PKGS at the top.
ifeq ($(MISSING_PKGS),)
  GUI_TARGET = satl-term
else
  GUI_TARGET = gui-skipped
endif
