# satellite 004 -- what satl is compiled from.
#
# BY NAME, NEVER A WHOLE FOLDER, so an editor's swap or lock file is not a source
# and not a build input. A new .cpp is one line in INTERPRETER_SOURCES; its
# headers need no line, because 060-compile.mk has the compiler write each
# object's real dependencies. HEADERS is kept for the build fingerprint: a header
# that changes is a new build.

INTERPRETER_SOURCES = $(SATELLITE)/structured-library.cpp \
                      $(ARGUMENTS)/arguments.cpp $(ARGUMENTS)/command_line.cpp \
                      $(SATELLITE)/licenses/licenses.cpp \
                      $(MACHINE)/machine_state.cpp $(SATELLITE)/satl/satl_file.cpp \
                      $(SATELLITE)/satl/session.cpp $(SATELLITE)/satl/listing.cpp \
                      $(PROMPT)/raw_mode.cpp $(PROMPT)/keys.cpp $(PROMPT)/editor.cpp \
                      $(PROMPT)/history.cpp $(PROMPT)/render.cpp $(PROMPT)/line_reader.cpp \
                      $(SATELLITE)/threads/startup_threads.cpp \
                      $(BYTECODE)/bytecode_registry.cpp $(BYTECODE)/cascade_convert.cpp \
                      $(BYTECODE)/function_table.cpp $(BYTECODE)/include_shape.cpp \
                      $(BYTECODE)/program_walk.cpp $(BYTECODE)/program_check.cpp $(BYTECODE)/type_shape.cpp \
                      $(BYTECODE)/expression.cpp $(BYTECODE)/sate_file.cpp \
                      $(BYTECODE)/file_calls.cpp $(BYTECODE)/container_calls.cpp $(SATELLITE)/satellite_variable_file/satellite_file.cpp \
                      $(BYTECODE)/infinity_calls.cpp $(INFINITY)/satellite_infinity.cpp \
                      $(BYTECODE)/window_calls.cpp $(BYTECODE)/window_readers.cpp $(BYTECODE)/window_shapes.cpp \
                      $(BYTECODE)/window_questions.cpp $(BYTECODE)/window_methods.cpp $(BYTECODE)/window_run.cpp \
                      $(OBJECT)/satellite_object.cpp \
                      $(OBJECT)/str_add_str.cpp $(OBJECT)/str_minus_str.cpp $(OBJECT)/str_find_str.cpp \
                      $(OBJECT)/num_add_num.cpp $(OBJECT)/num_sub_num.cpp $(OBJECT)/num_div_num.cpp \
                      $(OBJECT)/object_convert.cpp $(OBJECT)/object_percentage.cpp \
                      $(NUMBER)/satellite_number.cpp $(NUMBER)/satellite_number_divide.cpp \
                      $(NUMBER)/satellite_number_text.cpp $(NUMBER)/satellite_number_power.cpp \
                      $(STRING16)/satellite_string.cpp \
                      $(NUMBERS)/call_number.satellite.cpp

HEADERS = $(ARGUMENTS)/arguments.hpp $(ARGUMENTS)/command_line.hpp \
          $(ARGUMENTS)/argument_value.hpp $(ARGUMENTS)/argument_case.hpp \
          $(ARGUMENTS)/satellite_arguments.hpp $(SATELLITE)/config/satellite_config.hpp \
          $(SATELLITE)/config/config_file.hpp $(SATELLITE)/config/feature_register.hpp \
          $(SATELLITE)/config/feature_switch.hpp $(SATELLITE)/config/rebuild.hpp \
          $(SATELLITE)/config/machine_probe.hpp $(SATELLITE)/config/run_config.hpp \
          $(SATELLITE)/config/run_feedback.hpp $(NUMBERS)/feedback_book.hpp \
          $(MACHINE)/critical_report.hpp $(MACHINE)/source_position.hpp $(MACHINE)/s_codes.hpp \
          $(MACHINE)/machine_codes.hpp $(MACHINE)/machine_state.hpp $(MACHINE)/exit_status.hpp $(MACHINE)/shown.hpp \
          $(MACHINE)/stop_flag.hpp $(SATELLITE)/satl/session.hpp $(SATELLITE)/satl/listing.hpp \
          $(PROMPT)/raw_mode.hpp $(PROMPT)/keys.hpp $(PROMPT)/editor.hpp $(PROMPT)/history.hpp \
          $(PROMPT)/render.hpp $(PROMPT)/line_reader.hpp $(NUMBERS)/directory_words.hpp \
          $(SATELLITE)/satl/satl_file.hpp $(SATELLITE)/threads/startup_threads.hpp \
          $(VERSION_DIR)/version.hpp $(VERSION_DIR)/title_lines.hpp \
          $(BYTECODE)/bytecode_registry.hpp $(BYTECODE)/token_codes.hpp \
          $(BYTECODE)/word_codes.hpp $(BYTECODE)/word_counts.hpp $(BYTECODE)/statement_ring.hpp \
          $(BYTECODE)/function_table.hpp \
          $(BYTECODE)/include_shape.hpp $(BYTECODE)/program_walk.hpp \
          $(BYTECODE)/expression.hpp $(BYTECODE)/value.hpp \
          $(BYTECODE)/file_calls.hpp $(BYTECODE)/container_calls.hpp $(SATELLITE)/satellite_variable_file/satellite_file.hpp \
          $(BYTECODE)/infinity_calls.hpp $(INFINITY)/satellite_infinity.hpp \
          $(BYTECODE)/window_calls.hpp $(BYTECODE)/window_readers.hpp \
          $(WINDOW_DIR)/satellite_window.hpp $(WINDOW_DIR)/window_desk.hpp \
          $(WINDOW_DIR)/window_spill.hpp $(WINDOW_DIR)/window_menu.hpp $(WINDOW_DIR)/window_canvas.hpp \
          $(BYTECODE)/sate_file.hpp $(BYTECODE)/cascade_convert.hpp \
          $(NUMBER)/satellite_number.hpp $(NUMBER)/satellite_number_limbs.hpp \
          $(NUMBER)/number_arithmetic.hpp $(NUMBER)/number_conversions.hpp \
          $(STRING16)/satellite_string.hpp $(STRING16)/character_table.hpp \
          $(STRING16)/conversion_loops.hpp $(STRING16)/string_overwrite.hpp \
          $(BINARY)/satellite_binary_number.hpp \
          $(PERCENTAGE)/satellite_percentage.hpp \
          $(OBJECT)/satellite_object.hpp $(OBJECT)/satellite_spacesuit.hpp \
          $(OBJECT)/satellite_list.hpp $(OBJECT)/satellite_index.hpp \
          $(BYTECODE)/type_shape.hpp \
          $(OBJECT)/satellite_bytecode.hpp $(OBJECT)/satellite_capsule.hpp \
          $(OBJECT)/fast_paths.hpp $(OBJECT)/object_pair.hpp \
          $(wildcard $(OBJECT)/*_and_*.hpp) $(wildcard $(OBJECT)/*_to_*.hpp) \
          $(NUMBERS)/call_number.hpp $(NUMBERS)/number_row.hpp $(STRINGS32)/string_method.hpp \
          $(SATELLITE)/licenses/licenses.hpp

# EVERY LICENCE, COMPILED IN. licenses/<project>/license.txt is the source of
# truth -- the same files the website and THIRD-PARTY-NOTICES.txt come from -- and
# make_license_data.py turns them into one generated .cpp of raw string literals.
#
# PLAIN C++, NOT A GResource, unlike the window's fonts and keyboard data. That one
# may assume glib because it only exists when GTK is compiled in; this has to work
# in `make` as well as `make GTK=vendor`, so it depends on nothing.
#
# It is in INTERPRETER_OBJECTS rather than GTK_OBJECTS for the same reason: satl
# without a window still has licences, and still has to be able to show them.
LICENCE_DATA_SOURCE = $(BUILD)/generated/license_data.cpp
LICENCE_DATA_OBJECT = $(OBJECTS)/generated/license_data.o
LICENCE_DATA_INPUTS = $(SATELLITE)/licenses/make_license_data.py \
                      $(wildcard licenses/*/license.txt)

INTERPRETER_OBJECTS = $(INTERPRETER_SOURCES:%.cpp=$(OBJECTS)/%.o) $(LICENCE_DATA_OBJECT)

# THE FILES THE APPLICATION IS MADE FROM, for the build number (020-version.mk):
# satl, satl-term and their build. build_number.py adds every
# satellite-numbers/*/*.satellite.cpp itself -- their names hold brackets and
# spaces, which make cannot list. TERM_SOURCES is 047-window.mk's, read when this
# is expanded, and counted whether or not this machine can build the window.
MAKE_FRAGMENTS = make_support/005-jobs.mk make_support/010-compiler.mk make_support/020-version.mk \
                 make_support/030-directories.mk make_support/040-sources.mk make_support/047-window.mk \
                 make_support/048-link.mk make_support/050-build.mk make_support/060-compile.mk \
                 make_support/065-tests.mk make_support/070-clean.mk
BUILD_INPUTS = $(INTERPRETER_SOURCES) $(HEADERS) $(GTK_SOURCES) $(TERM_SOURCES) $(TERM_HEADERS) Makefile $(MAKE_FRAGMENTS) \
               $(SATELLITE)/config/build_number.py words/words.tsv $(NUMBERS)/build_libraries.py
