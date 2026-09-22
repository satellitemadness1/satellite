# satellite 004 -- the checks, the harnesses and the races.
#
#     make check        check.sh: every example and test, and satl's command line
#     make test         check.sh, the installer's checks and the string checks, all even when one fails
#     make race         satellite.console.display against std::cout, 10,000,000 lines
#     make number-race  i = i + 1 against signed long long int
#
# THE HARNESSES ARE NOT THE APPLICATION: they do not depend on the build stamp,
# and building one does not raise the build number. Each is one command, as the
# old Makefile had them. check.sh takes SATL=<path> to check an installed satl.
#
# THE STRING CHECKS RUN 003's satl (old_versions/second_satellite/satl) for their
# answers, so that binary stays built -- but never with a bare `make` in 003's
# tree, which re-installs 003 into ~/.satl.

STRING_HARNESSES = $(BUILD)/string_cases $(BUILD)/string_methods $(BUILD)/string16_cases $(BUILD)/string_table_check

check: all
	./check.sh

test: all $(STRING_HARNESSES)
	@failed=0; \
	 ./check.sh || failed=1; \
	 sh satellite_enterprise/check_install.sh || failed=1; \
	 python3 $(STRINGS32)/check_strings.py || failed=1; \
	 python3 $(STRINGS32)/check_string_methods.py || failed=1; \
	 python3 $(STRING16)/check_strings16.py || failed=1; \
	 [ $$failed = 0 ] && echo "make test: every check passed" || { echo "make test: a check FAILED (above)"; exit 1; }

race: all $(BUILD)/race
	./$(SATELLITE)/race/race.sh

number-race: $(BUILD)/number_race
	./$(NUMBER)/number_race.sh

# The exit status of a machine code that does not fit 8 bits (machine/exit_status.hpp):
# no program can stop on 256 yet, so the mapping is checked here, and check.sh runs it.
$(BUILD)/exit_status_cases: $(MACHINE)/exit_status_cases.cpp $(MACHINE)/exit_status.hpp $(MACHINE)/machine_state.cpp \
                            $(MACHINE)/machine_state.hpp $(MACHINE)/machine_codes.hpp $(MACHINE)/shown.hpp
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(MACHINE)/exit_status_cases.cpp $(MACHINE)/machine_state.cpp -o $@

# A [COUNTED] token's count, written by put_count and read by count_at, over counts
# no program could carry (bytecode/count_cases.cpp). check.sh runs it, and refuses
# one older than these prerequisites. Under -fsanitize=undefined: a shift past 63
# gives the right answer here by luck, and only the sanitizer says so.
#
# NOT satellite_config.hpp, which it never includes: a make that raises the build
# number rewrites that row while this links, so it came out older than the header
# after most builds, and check.sh called it stale.
COUNT_CASES_SOURCES = $(BYTECODE)/count_cases.cpp $(BYTECODE)/bytecode_registry.cpp $(BYTECODE)/cascade_convert.cpp \
                      $(SATELLITE)/threads/startup_threads.cpp $(MACHINE)/machine_state.cpp
$(BUILD)/count_cases: $(COUNT_CASES_SOURCES) $(filter-out $(SATELLITE)/config/satellite_config.hpp,$(HEADERS))
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) -fsanitize=undefined -fno-sanitize-recover=undefined $(LDFLAGS) \
	    $(COUNT_CASES_SOURCES) -o $@

# satellite/prompt, M0.6's terminal layer, alone: its cases with no terminal, and a
# reader with no language behind it, which check_prompt.py types at through a real
# terminal. check.sh runs both. They join satl when the session reads its lines.
PROMPT_SOURCES = $(PROMPT)/raw_mode.cpp $(PROMPT)/keys.cpp $(PROMPT)/editor.cpp $(PROMPT)/history.cpp \
                 $(PROMPT)/render.cpp $(PROMPT)/line_reader.cpp
PROMPT_HEADERS = $(PROMPT_SOURCES:.cpp=.hpp) $(MACHINE)/shown.hpp

$(BUILD)/prompt_cases: $(PROMPT)/prompt_cases.cpp $(PROMPT_SOURCES) $(PROMPT_HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(PROMPT)/prompt_cases.cpp $(PROMPT_SOURCES) -o $@

$(BUILD)/prompt_reader: $(PROMPT)/prompt_reader.cpp $(PROMPT_SOURCES) $(PROMPT_HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(PROMPT)/prompt_reader.cpp $(PROMPT_SOURCES) -o $@

# satellite.directory's three words with no interpreter around them: the header all
# three libraries are built from (PLAN M0.6). It takes the room to work in, so the
# checks never write beside the tree.
$(BUILD)/directory_cases: $(SATELLITE)/satl/directory_cases.cpp $(NUMBERS)/directory_words.hpp $(NUMBERS)/number_row.hpp \
                          $(MACHINE)/machine_codes.hpp
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(SATELLITE)/satl/directory_cases.cpp -o $@

# satellite.variable.file's handle with no interpreter around it (SATELLITE_FILE_OPERATIONS
# Part 3): the list of lines, the endings, strict UTF-8 and the two saves. It takes the
# room to work in, as directory_cases does. satellite_string.cpp is linked because a
# file's idea of UTF-8 text is the language's own from_utf8.
FILE_CASES_SOURCES = $(SATELLITE)/satellite_variable_file/file_cases.cpp \
                     $(SATELLITE)/satellite_variable_file/satellite_file.cpp $(STRING16)/satellite_string.cpp
FILE_CASES_HEADERS = $(SATELLITE)/satellite_variable_file/satellite_file.hpp $(STRING16)/satellite_string.hpp \
                     $(STRING16)/string_overwrite.hpp $(STRING16)/character_table.hpp \
                     $(STRING16)/conversion_loops.hpp $(MACHINE)/machine_codes.hpp
$(BUILD)/file_cases: $(FILE_CASES_SOURCES) $(FILE_CASES_HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(FILE_CASES_SOURCES) -o $@

# The build fingerprint's inputs, one a line: check.sh compares them with the
# headers the compiler says satl is made from.
build-inputs:
	@printf '%s\n' $(BUILD_INPUTS)

$(BUILD)/race: $(SATELLITE)/race/race.cpp $(NUMBERS)/call_number.satellite.cpp $(MACHINE)/machine_state.cpp $(HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(SATELLITE)/race/race.cpp $(NUMBERS)/call_number.satellite.cpp \
	    $(MACHINE)/machine_state.cpp -o $@ -ldl

$(BUILD)/string_methods: $(STRINGS32)/test_string_methods.cpp $(STRINGS32)/satellite_string.cpp $(STRINGS32)/satellite_string.hpp \
                         $(NUMBERS)/call_number.satellite.cpp $(MACHINE)/machine_state.cpp $(HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) -I. $(STRINGS32)/test_string_methods.cpp $(STRINGS32)/satellite_string.cpp \
	    $(NUMBERS)/call_number.satellite.cpp $(MACHINE)/machine_state.cpp -o $@ -ldl

$(BUILD)/string_cases: $(STRINGS32)/string_cases.cpp $(STRINGS32)/satellite_string.cpp $(STRINGS32)/satellite_string.hpp \
                       $(MACHINE)/machine_codes.hpp
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(STRINGS32)/string_cases.cpp $(STRINGS32)/satellite_string.cpp -o $@

# satellite.variable.number and satellite.variable.string: their own harnesses,
# checked against Python.
NUMBER_SOURCES = $(NUMBER)/satellite_number.cpp $(NUMBER)/satellite_number_divide.cpp \
                 $(NUMBER)/satellite_number_text.cpp $(NUMBER)/satellite_number_power.cpp
NUMBER_HEADERS = $(NUMBER)/satellite_number.hpp $(NUMBER)/satellite_number_limbs.hpp \
                 $(NUMBER)/number_arithmetic.hpp $(NUMBER)/number_conversions.hpp $(MACHINE)/machine_codes.hpp

# Every name satl fills in is refused as a config row, and a real gather adds no
# name outside that list (arguments.cpp filled_in_by_satl). check.sh runs it.
ARGUMENTS_CASES_SOURCES = $(ARGUMENTS)/arguments_cases.cpp $(ARGUMENTS)/arguments.cpp $(ARGUMENTS)/command_line.cpp \
                          $(MACHINE)/machine_state.cpp $(NUMBER_SOURCES)
$(BUILD)/arguments_cases: $(ARGUMENTS_CASES_SOURCES) $(HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(ARGUMENTS_CASES_SOURCES) -o $@

$(BUILD)/number_cases: $(NUMBER)/number_cases.cpp $(NUMBER_SOURCES) $(NUMBER_HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(NUMBER)/number_cases.cpp $(NUMBER_SOURCES) -o $@

# satellite.variable.infinity with no interpreter around it (SATELLITE_INFINITY.md,
# INF-2): the display and the order, held to infinity_oracle.py by check_infinity.py,
# which check.sh runs. It refuses one older than these prerequisites.
INFINITY_CASES_SOURCES = $(INFINITY)/infinity_cases.cpp $(INFINITY)/satellite_infinity.cpp $(NUMBER_SOURCES)
$(BUILD)/infinity_cases: $(INFINITY_CASES_SOURCES) $(INFINITY)/satellite_infinity.hpp $(NUMBER_HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(INFINITY_CASES_SOURCES) -o $@

$(BUILD)/number_race: $(NUMBER)/number_race.cpp $(NUMBER_SOURCES) $(NUMBER_HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(NUMBER)/number_race.cpp $(NUMBER_SOURCES) -o $@

STRING16_SOURCES = $(STRING16)/satellite_string.cpp $(STRING16)/satellite_string.hpp $(STRING16)/string_overwrite.hpp \
                   $(STRING16)/character_table.hpp $(STRING16)/conversion_loops.hpp $(MACHINE)/machine_codes.hpp

$(BUILD)/string16_cases: $(STRING16)/string16_cases.cpp $(STRING16_SOURCES)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(STRING16)/string16_cases.cpp $(STRING16)/satellite_string.cpp -o $@

$(BUILD)/string_table_check: $(STRING16)/string_table_check.cpp $(STRING16_SOURCES)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(STRING16)/string_table_check.cpp $(STRING16)/satellite_string.cpp -o $@

$(BUILD)/string_race: $(STRING16)/string_race.cpp $(STRING16)/plain_conversions.hpp $(STRING16)/plain_like_satellite.hpp \
                      $(STRING16_SOURCES) $(STRINGS32)/satellite_string.cpp $(STRINGS32)/satellite_string.hpp
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(STRING16)/string_race.cpp $(STRING16)/satellite_string.cpp \
	    $(STRINGS32)/satellite_string.cpp -o $@

.PHONY: check test race number-race build-inputs
