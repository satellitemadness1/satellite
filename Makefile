# satellite-004 -- its own build, into build/, separate from the satellite tree's.
#
#     make            the interpreter and every numbered library
#     make check      run the example programs and check their machine codes
#     make build/string_cases build/string_methods   the 32-bit string harnesses (strings/check_*.py)
#     make build/number_cases   satellite_number's harness (satellite/satellite_variable_number/check_numbers.py)
#     make number-race          the M4 race: i = i + 1 against signed long long int
#     make build/string16_cases build/string_table_check   satellite_string's checks
#                               (python3 satellite/satellite_variable_string/check_strings16.py)
#     make build/string_race    satellite_string's speed against plain C++
#     make race       satellite-004's display against std::cout, 10,000,000 lines
#     make clean
#
# The interpreter's sources are under satellite/ (the author, 2026-09-15), one
# folder a subject: arguments, config, machine, race, satl, threads, version.
#
# DYNAMIC ON PURPOSE. The libraries and the interpreter must share ONE copy of
# libstdc++, or each would have its own std::cout. The satellite tree links
# statically; this folder cannot, for that reason.
#
# THE BUILD NUMBER (the author, 2026-09-15). A make whose inputs changed -- the
# files below, every library source, or the compiler and flags -- raises
# arguments.build in satellite/config/satellite_config.hpp by one BEFORE anything
# compiles; any other make leaves it alone, even one that rebuilds what is missing.
# satellite/config/build_number.py says exactly when. Its stamp, .satellite_build
# (not in git), holds the last build's number and a fingerprint of its inputs.

CXX ?= g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra
BUILD = build
BUILD_STAMP = .satellite_build

# The build machine's operating system for the start-up block's third line
# ("CLANG++ 24 ALMALINUX 10.2"): NAME and VERSION_ID from /etc/os-release, upper
# case, letters, digits, spaces, dots and dashes only.
BUILD_OS := $(shell (. /etc/os-release 2>/dev/null && echo "$$NAME $$VERSION_ID" || uname -sr) | tr a-z A-Z | tr -cd 'A-Z0-9 .-')
OS_DEFINE = -DSATELLITE_BUILD_OS='"$(BUILD_OS)"'

INTERPRETER_SOURCES = satellite/structured-library.cpp satellite/arguments/arguments.cpp \
                      satellite/machine/machine_state.cpp satellite/satl/satl_file.cpp \
                      satellite/threads/startup_threads.cpp satellite/bytecode/bytecode_registry.cpp \
                      satellite-numbers/call_number.satellite.cpp
HEADERS = satellite/arguments/arguments.hpp satellite/config/satellite_config.hpp \
          satellite/machine/machine_codes.hpp satellite/machine/machine_state.hpp \
          satellite/satl/satl_file.hpp satellite/threads/startup_threads.hpp satellite/version/version.hpp \
          satellite/bytecode/bytecode_registry.hpp satellite/bytecode/token_codes.hpp \
          satellite-numbers/call_number.hpp satellite-numbers/number_row.hpp strings/string_method.hpp
MACHINE_STATE = satellite/machine/machine_state.cpp

# The files the application (the interpreter and its libraries) is made from, by
# name: never a whole folder, so an editor's swap or lock file is not an input.
# build_number.py adds every satellite-numbers/*/*.satellite.cpp itself (their
# names hold brackets and spaces, which make cannot list).
BUILD_INPUTS = $(INTERPRETER_SOURCES) $(HEADERS) Makefile satellite/config/build_number.py \
               words/words.tsv satellite-numbers/build_libraries.py

# Every numbered library is built by satellite-numbers/build_libraries.py, which
# names each .so by its numbers from words/words.tsv (make cannot: word names have
# brackets, which make reads as archive members).
all: $(BUILD)/satellite-004 libraries

# Runs on every make; build_number.py decides whether this make is a build, and
# rewrites the stamp only when it is -- which is what relinks the interpreter.
$(BUILD_STAMP): FORCE
	@python3 satellite/config/build_number.py $@ --also "$(CXX) $(CXXFLAGS) $(BUILD_OS)" -- $(BUILD_INPUTS)

libraries: $(BUILD_STAMP)
	python3 satellite-numbers/build_libraries.py

$(BUILD)/satellite-004: $(BUILD_STAMP) $(INTERPRETER_SOURCES) $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(OS_DEFINE) $(INTERPRETER_SOURCES) -o $@ -ldl
	@python3 satellite/config/build_number.py $(BUILD_STAMP) --verify

$(BUILD)/race: satellite/race/race.cpp satellite-numbers/call_number.satellite.cpp $(MACHINE_STATE) $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) satellite/race/race.cpp satellite-numbers/call_number.satellite.cpp $(MACHINE_STATE) -o $@ -ldl

$(BUILD)/string_methods: strings/test_string_methods.cpp strings/satellite_string.cpp satellite-numbers/call_number.satellite.cpp $(MACHINE_STATE) $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I. strings/test_string_methods.cpp strings/satellite_string.cpp satellite-numbers/call_number.satellite.cpp $(MACHINE_STATE) -o $@ -ldl

$(BUILD)/string_cases: strings/string_cases.cpp strings/satellite_string.cpp strings/satellite_string.hpp satellite/machine/machine_codes.hpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) strings/string_cases.cpp strings/satellite_string.cpp -o $@

# satellite.variable.number and satellite.variable.string: their own harnesses,
# checked against Python. They are not the application, so they do not depend on
# the build stamp and building one does not raise the build number.
NUMBER = satellite/satellite_variable_number
NUMBER_SOURCES = $(NUMBER)/satellite_number.cpp $(NUMBER)/satellite_number_divide.cpp $(NUMBER)/satellite_number_text.cpp
NUMBER_HEADERS = $(NUMBER)/satellite_number.hpp $(NUMBER)/satellite_number_limbs.hpp satellite/machine/machine_codes.hpp

$(BUILD)/number_cases: $(NUMBER)/number_cases.cpp $(NUMBER_SOURCES) $(NUMBER_HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(NUMBER)/number_cases.cpp $(NUMBER_SOURCES) -o $@

$(BUILD)/number_race: $(NUMBER)/number_race.cpp $(NUMBER_SOURCES) $(NUMBER_HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(NUMBER)/number_race.cpp $(NUMBER_SOURCES) -o $@

number-race: $(BUILD)/number_race
	./$(NUMBER)/number_race.sh

STRING16 = satellite/satellite_variable_string
STRING16_SOURCES = $(STRING16)/satellite_string.cpp $(STRING16)/satellite_string.hpp $(STRING16)/string_overwrite.hpp \
                   $(STRING16)/character_table.hpp $(STRING16)/conversion_loops.hpp satellite/machine/machine_codes.hpp

$(BUILD)/string16_cases: $(STRING16)/string16_cases.cpp $(STRING16_SOURCES)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(STRING16)/string16_cases.cpp $(STRING16)/satellite_string.cpp -o $@

$(BUILD)/string_table_check: $(STRING16)/string_table_check.cpp $(STRING16_SOURCES)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(STRING16)/string_table_check.cpp $(STRING16)/satellite_string.cpp -o $@

$(BUILD)/string_race: $(STRING16)/string_race.cpp $(STRING16)/plain_conversions.hpp $(STRING16)/plain_like_satellite.hpp \
                      $(STRING16_SOURCES) strings/satellite_string.cpp strings/satellite_string.hpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(STRING16)/string_race.cpp $(STRING16)/satellite_string.cpp strings/satellite_string.cpp -o $@

check: all
	./check.sh

race: all $(BUILD)/race
	./satellite/race/race.sh

clean:
	rm -rf $(BUILD)

FORCE:

.PHONY: all libraries check race number-race clean FORCE
