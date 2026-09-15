# satellite-004 -- its own build, into build/, separate from the satellite tree's.
#
#     make            the interpreter and every numbered library
#     make check      run the example programs and check their machine codes
#     make build/string_cases build/string_methods   the string harnesses (strings/check_*.py)
#     make race       satellite-004's display against std::cout, 10,000,000 lines
#     make clean
#
# DYNAMIC ON PURPOSE. The libraries and the interpreter must share ONE copy of
# libstdc++, or each would have its own std::cout. The satellite tree links
# statically; this folder cannot, for that reason.

CXX ?= g++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra
BUILD = build

INTERPRETER_SOURCES = structured-library.cpp arguments.cpp machine_state.cpp satl_file.cpp \
                      satellite-numbers/call_number.satellite.cpp
HEADERS = arguments.hpp machine_codes.hpp machine_state.hpp satl_file.hpp version.hpp \
          satellite-numbers/call_number.hpp satellite-numbers/number_row.hpp strings/string_method.hpp

# Every numbered library is built by satellite-numbers/build_libraries.py, which
# names each .so by its numbers from words/words.tsv (make cannot: word names have
# brackets, which make reads as archive members).
all: $(BUILD)/satellite-004 libraries

libraries:
	python3 satellite-numbers/build_libraries.py

$(BUILD)/satellite-004: $(INTERPRETER_SOURCES) $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I. $(INTERPRETER_SOURCES) -o $@ -ldl

$(BUILD)/race: race.cpp satellite-numbers/call_number.satellite.cpp machine_state.cpp $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I. race.cpp satellite-numbers/call_number.satellite.cpp machine_state.cpp -o $@ -ldl

$(BUILD)/string_methods: strings/test_string_methods.cpp strings/satellite_string.cpp satellite-numbers/call_number.satellite.cpp machine_state.cpp $(HEADERS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -I. strings/test_string_methods.cpp strings/satellite_string.cpp satellite-numbers/call_number.satellite.cpp machine_state.cpp -o $@ -ldl

$(BUILD)/string_cases: strings/string_cases.cpp strings/satellite_string.cpp strings/satellite_string.hpp machine_codes.hpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) strings/string_cases.cpp strings/satellite_string.cpp -o $@

check: all
	./check.sh

race: all $(BUILD)/race
	./race.sh

clean:
	rm -rf $(BUILD)

.PHONY: all libraries check race clean
