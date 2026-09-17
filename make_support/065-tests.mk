# satellite 004 -- the checks, the harnesses and the races.
#
#     make check        check.sh: every example and test, and satl's command line
#     make test         check.sh and the string checks, all of them even when one fails
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

check: all $(BUILD)/exit_status_cases
	./check.sh

test: all $(BUILD)/exit_status_cases $(STRING_HARNESSES)
	@failed=0; \
	 ./check.sh || failed=1; \
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
                            $(MACHINE)/machine_codes.hpp $(MACHINE)/shown.hpp
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(MACHINE)/exit_status_cases.cpp $(MACHINE)/machine_state.cpp -o $@

$(BUILD)/race: $(SATELLITE)/race/race.cpp $(NUMBERS)/call_number.satellite.cpp $(MACHINE)/machine_state.cpp $(HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(SATELLITE)/race/race.cpp $(NUMBERS)/call_number.satellite.cpp \
	    $(MACHINE)/machine_state.cpp -o $@ -ldl

$(BUILD)/string_methods: $(STRINGS32)/test_string_methods.cpp $(STRINGS32)/satellite_string.cpp \
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

$(BUILD)/number_cases: $(NUMBER)/number_cases.cpp $(NUMBER_SOURCES) $(NUMBER_HEADERS)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(NUMBER)/number_cases.cpp $(NUMBER_SOURCES) -o $@

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

.PHONY: check test race number-race
