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

# EVERY HARNESS DEPENDS ON THE FLAGS IT IS BUILT WITH, THOUGH (open-work tools, 2026-10-25): one built
# at -O2 was current to `make -sq OPT=-O0` and to `CXX=g++`, and check.sh ran it as if those flags had
# made it -- CXXFLAGS is no prerequisite, and 060-compile.mk's stamp is satl's alone. THEIR OWN STAMP,
# AND NOT 060's PATTERN: that stamp hangs on FORCE, so make always calls it out of date, and check.sh's
# `make -sq build/<harness>` would call every harness stale. This one hangs on FORCE only while what
# it holds differs from the flags -- asked as make reads, so it is rewritten, and every harness that
# names it is built again, exactly when they change. check.sh asks with the HOME it was started with,
# which is where 010-compiler.mk finds the compiler.
HARNESS_FLAGS = $(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(LDFLAGS)
HARNESS_STAMP = $(BUILD)/.harness-flags
ifneq ($(file <$(HARNESS_STAMP)),$(HARNESS_FLAGS))
$(HARNESS_STAMP): FORCE
endif
$(HARNESS_STAMP):
	@mkdir -p $(BUILD)
	@printf '%s' '$(HARNESS_FLAGS)' > $@

STRING_HARNESSES = $(BUILD)/string_cases $(BUILD)/string16_cases $(BUILD)/string_table_check

check: all
	./check.sh

test: all $(STRING_HARNESSES)
	@failed=0; \
	 ./check.sh || failed=1; \
	 sh satellite_enterprise/check_install.sh || failed=1; \
	 python3 $(STRINGS32)/check_strings.py || failed=1; \
	 python3 $(STRING16)/check_strings16.py || failed=1; \
	 [ $$failed = 0 ] && echo "make test: every check passed" || { echo "make test: a check FAILED (above)"; exit 1; }

race: all $(BUILD)/race
	./$(SATELLITE)/race/race.sh

number-race: $(BUILD)/number_race
	./$(NUMBER)/number_race.sh

# The exit status of a machine code that does not fit 8 bits (machine/exit_status.hpp):
# no program can stop on 256 yet, so the mapping is checked here, and check.sh runs it.
$(BUILD)/exit_status_cases: $(MACHINE)/exit_status_cases.cpp $(MACHINE)/exit_status.hpp $(MACHINE)/machine_state.cpp \
                            $(MACHINE)/machine_state.hpp $(MACHINE)/machine_codes.hpp $(MACHINE)/shown.hpp $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(MACHINE)/exit_status_cases.cpp $(MACHINE)/machine_state.cpp -o $@

# THE MISSING FAMILY'S REGISTER (SCRATCH.md/MISSING_SYNTAX.md MS-3): every code's number, name and
# exit, and both shapes printing the lines a program is missing -- before any of them is raised.
# check.sh runs it, one row a code.
# s_codes.hpp reads the bytecode's headers through source_position.hpp, so every header but the
# build-numbered config is a prerequisite, as for count_cases.
$(BUILD)/missing_cases: $(MACHINE)/missing_cases.cpp $(MACHINE)/machine_state.cpp \
                        $(filter-out $(SATELLITE)/config/satellite_config.hpp,$(HEADERS)) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(MACHINE)/missing_cases.cpp $(MACHINE)/machine_state.cpp -o $@

# A [COUNTED] token's count, written by put_count and read by count_at, over counts
# no program could carry (bytecode/count_cases.cpp). check.sh runs it, and refuses
# one older than these prerequisites. Under -fsanitize=undefined: a shift past 63
# gives the right answer here by luck, and only the sanitizer says so.
#
# NOT satellite_config.hpp, which it never includes: a make that raises the build
# number rewrites that row while this links, so it came out older than the header
# after most builds, and check.sh called it stale.
#
# AND satellite_string.cpp, since bytecode_registry.cpp builds a string literal straight into
# its satellite_string (string_literal_at, FAST_PRINTING.md step 2, 2026-09-26).
COUNT_CASES_SOURCES = $(BYTECODE)/count_cases.cpp $(BYTECODE)/bytecode_registry.cpp $(BYTECODE)/cascade_convert.cpp \
                      $(SATELLITE)/threads/startup_threads.cpp $(MACHINE)/machine_state.cpp \
                      $(STRING16)/satellite_string.cpp
#
# THE SANITIZER ONLY WHERE THE COMPILER CAN LINK IT (the author's fresh AlmaLinux 10.2,
# 2026-09-23: `/usr/bin/ld: cannot find /usr/lib64/libubsan.so.1.0.0`). g++'s libubsan
# is its own package, and a machine without it failed the whole `make` on a test
# harness. Asked once, when this recipe runs: without it the cases are built and run
# all the same, and make says what they are missing.
# The probe links the way the recipe does -- the same flags, LDFLAGS and environment.
UBSAN_FLAGS = $(shell if printf 'int main(){}' | $(LINK_ENV) $(CXX) $(CXXFLAGS) -fsanitize=undefined -fno-sanitize-recover=undefined $(LDFLAGS) -x c++ - -o /dev/null >/dev/null 2>&1; \
                      then echo -fsanitize=undefined -fno-sanitize-recover=undefined; \
                      else echo "note: $(CXX) cannot link -fsanitize=undefined here (g++ needs the libubsan package, clang its sanitizer runtime), so build/count_cases is built without it" >&2; fi)
$(BUILD)/count_cases: $(COUNT_CASES_SOURCES) $(filter-out $(SATELLITE)/config/satellite_config.hpp,$(HEADERS)) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(UBSAN_FLAGS) $(LDFLAGS) \
	    $(COUNT_CASES_SOURCES) -o $@

# satellite/prompt, M0.6's terminal layer, alone: its cases with no terminal, and a
# reader with no language behind it, which check_prompt.py types at through a real
# terminal. check.sh runs both. They join satl when the session reads its lines.
# THE CONSOLE'S LINES ARE satellite.variable.strings (console_lines.hpp, KEY_PLAN.md KEY-3), so
# satellite_string.cpp is linked with them, and the headers it is made from are prerequisites. AND THE
# CONSOLES BEFORE THIS ONE (earlier_consoles.hpp, KEY-5 and KEY-6), which Left reaches; prompt_cases
# fills them by hand.
PROMPT_SOURCES = $(PROMPT)/raw_mode.cpp $(PROMPT)/keys.cpp $(PROMPT)/editor.cpp \
                 $(PROMPT)/render.cpp $(PROMPT)/line_reader.cpp \
                 $(PROMPT)/console_lines.cpp $(PROMPT)/earlier_consoles.cpp $(STRING16)/satellite_string.cpp
# AND THE CONSOLE'S HOOK the reader calls at an Enter (machine/console_store.hpp, KEY_PLAN.md KEY-1):
# a header alone, left null in both harnesses, so neither links anything of satl/.
PROMPT_HEADERS = $(PROMPT_SOURCES:.cpp=.hpp) $(MACHINE)/shown.hpp $(MACHINE)/console_store.hpp \
                 $(STRING16)/character_table.hpp $(STRING16)/conversion_loops.hpp $(STRING16)/string_overwrite.hpp \
                 $(MACHINE)/machine_codes.hpp

$(BUILD)/prompt_cases: $(PROMPT)/prompt_cases.cpp $(PROMPT_SOURCES) $(PROMPT_HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(PROMPT)/prompt_cases.cpp $(PROMPT_SOURCES) -o $@

$(BUILD)/prompt_reader: $(PROMPT)/prompt_reader.cpp $(PROMPT_SOURCES) $(PROMPT_HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(PROMPT)/prompt_reader.cpp $(PROMPT_SOURCES) -o $@

# satellite.directory's three words with no interpreter around them: the header all
# three words are built from (PLAN M0.6; libraries until 2026-10-07). It takes the room to work in, so the
# checks never write beside the tree.
$(BUILD)/directory_cases: $(SATELLITE)/satl/directory_cases.cpp $(NUMBERS)/directory_words.hpp $(NUMBERS)/number_row.hpp \
                          $(MACHINE)/machine_codes.hpp $(MACHINE)/filesystems.hpp $(HARNESS_STAMP)
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
$(BUILD)/file_cases: $(FILE_CASES_SOURCES) $(FILE_CASES_HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(FILE_CASES_SOURCES) -o $@

# satellite.file.delete AND WHAT IT ASKS WITH, with no interpreter around it (FILE_AND_DRIVE_PLAN.md FD-2): the
# delete, su on a pty of its own and sudo -S through stand-ins the cases write, and the hidden question and the
# yes-or-no on a pty. It takes the room to work in, as file_cases does, and never runs a real su or sudo.
FILE_MOVES_CASES_SOURCES = $(SATELLITE)/satellite_variable_file/file_moves_cases.cpp \
                           $(SATELLITE)/satellite_variable_file/file_moves.cpp \
                           $(MACHINE)/as_another_account.cpp $(MACHINE)/ask_at_the_console.cpp $(MACHINE)/stack_share.cpp
FILE_MOVES_CASES_HEADERS = $(SATELLITE)/satellite_variable_file/file_moves.hpp $(MACHINE)/as_another_account.hpp \
                           $(MACHINE)/ask_at_the_console.hpp $(MACHINE)/stack_share.hpp $(MACHINE)/stop_flag.hpp
$(BUILD)/file_moves_cases: $(FILE_MOVES_CASES_SOURCES) $(FILE_MOVES_CASES_HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(FILE_MOVES_CASES_SOURCES) -o $@

# satellite.drive.list()'s ROWS, from a fake /sys/class/block and /run/udev/data the cases make in the room it is
# given (FILE_AND_DRIVE_PLAN.md FD-4), and then this machine's, read without opening a device.
DRIVE_MOUNTS_CASES_SOURCES = $(MACHINE)/drive_mounts_cases.cpp $(MACHINE)/drive_mounts.cpp
DRIVE_MOUNTS_CASES_HEADERS = $(MACHINE)/drive_mounts.hpp $(MACHINE)/filesystems.hpp
$(BUILD)/drive_mounts_cases: $(DRIVE_MOUNTS_CASES_SOURCES) $(DRIVE_MOUNTS_CASES_HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(DRIVE_MOUNTS_CASES_SOURCES) -o $@

# THE CONSOLE'S STORE WITH NO CONSOLE AROUND IT (KEY_PLAN.md KEY-1 to KEY-3, KEY-5 and KEY-6): the day's
# folder, from a clock of its own, made at the first Enter, the console's file claimed in it, every Enter a
# line of it and of the console's lines, S017 counted where it would be said, and the walk of the consoles
# before it, with Left's list of the newest of them, each read back. It takes the room to work in, as
# file_cases does, and points HOME into it -- never a person's. s_codes.hpp reads the bytecode's headers
# through source_position.hpp, so every header but the build-numbered config is a prerequisite, as for
# missing_cases; it never includes satellite_config.hpp.
CONSOLE_FILES_CASES_SOURCES = $(SATELLITE)/satl/console_files_cases.cpp $(SATELLITE)/satl/console_files.cpp \
                              $(PROMPT)/console_lines.cpp $(PROMPT)/earlier_consoles.cpp $(STRING16)/satellite_string.cpp
$(BUILD)/console_files_cases: $(CONSOLE_FILES_CASES_SOURCES) $(filter-out $(SATELLITE)/config/satellite_config.hpp,$(HEADERS)) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(CONSOLE_FILES_CASES_SOURCES) -o $@

# `interpret <file>` AND `run <file>` WITH NO SCREEN AROUND THEM (the author, 2026-10-20: "Just create another
# satellite, for each program"): where the prompt runs a program -- here and waited for, or in a satellite of
# its own while satl's own console is open -- and how a satellite is started, which the console menu shares:
# its words, its folder, its session, its streams, no wait and nothing left to reap. It is its own stand-in
# for satl, and takes the room to work in, as file_cases does. prompt_run.cpp reads window_calls.hpp, and
# through it the bytecode's headers, so every header but the build-numbered config is a prerequisite, as
# for missing_cases.
PROMPT_RUN_CASES_SOURCES = $(SATELLITE)/satl/prompt_run_cases.cpp $(SATELLITE)/satl/prompt_run.cpp \
                           $(MACHINE)/satellite_of_its_own.cpp $(MACHINE)/stack_share.cpp
$(BUILD)/prompt_run_cases: $(PROMPT_RUN_CASES_SOURCES) $(filter-out $(SATELLITE)/config/satellite_config.hpp,$(HEADERS)) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(PROMPT_RUN_CASES_SOURCES) -o $@

# The build fingerprint's inputs, one a line: check.sh compares them with the
# headers the compiler says satl is made from.
build-inputs:
	@printf '%s\n' $(BUILD_INPUTS)

# DISPLAY'S OWN WORD AND NOTHING ELSE SINCE 2026-10-10 (race.cpp says why): a string's methods are
# numbered words that run satl's own strings, so WORD_SOURCES no longer links without the interpreter.
$(BUILD)/race: $(SATELLITE)/race/race.cpp $(NUMBERS)/words/1.5.1.cpp $(MACHINE)/machine_state.cpp $(HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(SATELLITE)/race/race.cpp $(NUMBERS)/words/1.5.1.cpp \
	    $(MACHINE)/machine_state.cpp -o $@

# build/string_methods IS GONE (2026-10-10). It called the 32-bit string-method libraries ported from
# 003 through the number index, and strings/check_string_methods.py held their answers against 003's
# satl. The author ruled that day that where a method had two copies, the one programs ran stays --
# so those copies are gone from satellite-numbers/words/, and what programs run is checked where it
# always was, by check.sh's M16 rows through build/satl. strings/test_string_methods.cpp and
# strings/check_string_methods.py were deleted the same day, at his word (copies in the backups).

$(BUILD)/string_cases: $(STRINGS32)/string_cases.cpp $(STRINGS32)/satellite_string.cpp $(STRINGS32)/satellite_string.hpp \
                       $(MACHINE)/machine_codes.hpp $(HARNESS_STAMP)
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
$(BUILD)/arguments_cases: $(ARGUMENTS_CASES_SOURCES) $(HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(ARGUMENTS_CASES_SOURCES) -o $@

$(BUILD)/number_cases: $(NUMBER)/number_cases.cpp $(NUMBER_SOURCES) $(NUMBER_HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(NUMBER)/number_cases.cpp $(NUMBER_SOURCES) -o $@

# satellite.variable.infinity with no interpreter around it (SATELLITE_INFINITY.md,
# INF-2): the display and the order, held to infinity_oracle.py by check_infinity.py,
# which check.sh runs. It refuses one older than these prerequisites.
INFINITY_CASES_SOURCES = $(INFINITY)/infinity_cases.cpp $(INFINITY)/satellite_infinity.cpp $(NUMBER_SOURCES)
$(BUILD)/infinity_cases: $(INFINITY_CASES_SOURCES) $(INFINITY)/satellite_infinity.hpp $(NUMBER_HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(INFINITY_CASES_SOURCES) -o $@

# satellite.random with no interpreter around it (SCRATCH.md/RANDOM.md): the vendored
# generators against upstream's own expected output, the 512-bit one's integer against
# unsigned __int128 and its inverse xorshift against pcg's own, the generator's balance,
# and the sampler's uniformity -- check.sh runs it, and refuses one older than these
# prerequisites. It reads the PCG headers the way the generator's folder does
# (060-compile.mk).
RANDOM_CASES_SOURCES = $(RANDOM_DIR)/random_cases.cpp $(RANDOM_DIR)/random_source.cpp $(RANDOM_DIR)/random_draw.cpp \
                       $(RANDOM_DIR)/random_spin.cpp $(NUMBER_SOURCES)
RANDOM_HEADERS = $(RANDOM_DIR)/random_source.hpp $(RANDOM_DIR)/random_draw.hpp $(RANDOM_DIR)/random_spin.hpp $(RANDOM_DIR)/pcg_512.hpp \
                 $(RANDOM_DIR)/wide_unsigned.hpp $(RANDOM_DIR)/random_constants.hpp $(PCG_HEADERS)
$(BUILD)/random_cases: $(RANDOM_CASES_SOURCES) $(RANDOM_HEADERS) $(NUMBER_HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) -isystem $(PCG_INCLUDE) $(RANDOM_CASES_SOURCES) -o $@

$(BUILD)/number_race: $(NUMBER)/number_race.cpp $(NUMBER_SOURCES) $(NUMBER_HEADERS) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(NUMBER)/number_race.cpp $(NUMBER_SOURCES) -o $@

STRING16_SOURCES = $(STRING16)/satellite_string.cpp $(STRING16)/satellite_string.hpp $(STRING16)/string_overwrite.hpp \
                   $(STRING16)/character_table.hpp $(STRING16)/conversion_loops.hpp $(MACHINE)/machine_codes.hpp

$(BUILD)/string16_cases: $(STRING16)/string16_cases.cpp $(STRING16_SOURCES) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(STRING16)/string16_cases.cpp $(STRING16)/satellite_string.cpp -o $@

$(BUILD)/string_table_check: $(STRING16)/string_table_check.cpp $(STRING16_SOURCES) $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(STRING16)/string_table_check.cpp $(STRING16)/satellite_string.cpp -o $@

$(BUILD)/string_race: $(STRING16)/string_race.cpp $(STRING16)/plain_conversions.hpp $(STRING16)/plain_like_satellite.hpp \
                      $(STRING16_SOURCES) $(STRINGS32)/satellite_string.cpp $(STRINGS32)/satellite_string.hpp $(HARNESS_STAMP)
	@mkdir -p $(BUILD)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(STRING16)/string_race.cpp $(STRING16)/satellite_string.cpp \
	    $(STRINGS32)/satellite_string.cpp -o $@

.PHONY: check test race number-race build-inputs
