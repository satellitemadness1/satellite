# satellite 004 -- what satl is compiled from.
#
# BY NAME, NEVER A WHOLE FOLDER, so an editor's swap or lock file is not a source
# and not a build input. A new .cpp is one line in INTERPRETER_SOURCES; its
# headers need no line, because 060-compile.mk has the compiler write each
# object's real dependencies. HEADERS is kept for the build fingerprint: a header
# that changes is a new build.

# THE FOUR TYPES OF 2026-09-22, each with a list of its own so four builders adding
# files at once each touch a different line. A type's object file answers what the
# value does; its _values file answers the checker, the walker and the expression.
FLOAT_SOURCES    = $(OBJECT)/object_float.cpp $(BYTECODE)/float_values.cpp $(FLOAT_DIR)/float_scaled.cpp
HEX_SOURCES      = $(OBJECT)/object_hexadecimal.cpp $(BYTECODE)/hexadecimal_values.cpp
COLOR_SOURCES    = $(OBJECT)/object_color.cpp $(BYTECODE)/color_values.cpp $(BYTECODE)/color_check.cpp
FRACTION_SOURCES = $(OBJECT)/object_fraction.cpp $(BYTECODE)/fraction_values.cpp
FLOAT_HEADERS    = $(FLOAT_DIR)/satellite_float.hpp $(OBJECT)/object_float.hpp $(BYTECODE)/float_values.hpp \
                   $(FLOAT_DIR)/float_scaled.hpp $(FLOAT_DIR)/float_precision.hpp
HEX_HEADERS      = $(HEX_DIR)/satellite_hexadecimal_number.hpp $(OBJECT)/object_hexadecimal.hpp \
                   $(BYTECODE)/hexadecimal_values.hpp
COLOR_HEADERS    = $(COLOR_DIR)/satellite_color.hpp $(OBJECT)/object_color.hpp $(BYTECODE)/color_values.hpp \
                   $(BYTECODE)/color_reading.hpp
FRACTION_HEADERS = $(FRACTION_DIR)/satellite_fraction.hpp $(OBJECT)/object_fraction.hpp \
                   $(BYTECODE)/fraction_values.hpp

# THE WORDS BUILT INTO satl (the author, 2026-10-07: "built the satellite-numbers directly into
# the interpreter"): one file a word under satellite-numbers/words/, named by its numbers, and
# the table that calls each one. A new word is a new file, a row in word_table.cpp, and a line here.
WORD_SOURCES = $(NUMBERS)/word_table.cpp \
               $(NUMBERS)/words/1.4.1.1.cpp $(NUMBERS)/words/1.4.1.2.cpp $(NUMBERS)/words/1.4.1.3.cpp \
               $(NUMBERS)/words/1.4.1.4.cpp $(NUMBERS)/words/1.4.1.5.cpp $(NUMBERS)/words/1.4.1.6.cpp \
               $(NUMBERS)/words/1.4.1.7.cpp $(NUMBERS)/words/1.4.1.8.cpp $(NUMBERS)/words/1.4.1.9.cpp \
               $(NUMBERS)/words/1.4.1.11.cpp $(NUMBERS)/words/1.4.1.12.cpp $(NUMBERS)/words/1.4.1.13.cpp \
               $(NUMBERS)/words/1.4.1.14.cpp $(NUMBERS)/words/1.4.1.15.cpp $(NUMBERS)/words/1.4.1.16.cpp \
               $(NUMBERS)/words/1.4.2.1.cpp $(NUMBERS)/words/1.4.2.2.cpp $(NUMBERS)/words/1.4.2.3.cpp \
               $(NUMBERS)/words/1.4.2.8.cpp $(NUMBERS)/words/1.4.2.9.cpp $(NUMBERS)/words/1.4.2.10.cpp \
               $(NUMBERS)/words/1.4.2.11.cpp $(NUMBERS)/words/1.4.2.12.cpp $(NUMBERS)/words/1.4.2.13.cpp \
               $(NUMBERS)/words/1.4.2.14.cpp $(NUMBERS)/words/1.4.2.15.cpp $(NUMBERS)/words/1.4.2.16.cpp \
               $(NUMBERS)/words/1.4.2.17.cpp $(NUMBERS)/words/1.4.2.18.cpp $(NUMBERS)/words/1.4.2.19.cpp \
               $(NUMBERS)/words/1.4.2.20.cpp $(NUMBERS)/words/1.4.2.21.cpp $(NUMBERS)/words/1.4.2.22.cpp \
               $(NUMBERS)/words/1.4.2.23.cpp $(NUMBERS)/words/1.4.2.24.cpp $(NUMBERS)/words/1.4.2.25.cpp \
               $(NUMBERS)/words/1.4.2.26.cpp $(NUMBERS)/words/1.4.2.27.cpp $(NUMBERS)/words/1.4.2.28.cpp \
               $(NUMBERS)/words/1.5.1.cpp $(NUMBERS)/words/1.5.1.1.cpp $(NUMBERS)/words/1.5.1.2.cpp \
               $(NUMBERS)/words/1.6.1.0.cpp $(NUMBERS)/words/1.6.1.1.cpp \
               $(NUMBERS)/words/1.6.1.2.cpp $(NUMBERS)/words/1.6.1.3.cpp $(NUMBERS)/words/1.6.1.4.cpp \
               $(NUMBERS)/words/1.6.1.5.cpp $(NUMBERS)/words/1.6.1.6.cpp $(NUMBERS)/words/1.6.1.7.cpp \
               $(NUMBERS)/words/1.6.1.8.cpp $(NUMBERS)/words/1.6.1.9.cpp $(NUMBERS)/words/1.6.1.10.cpp \
               $(NUMBERS)/words/1.6.1.11.cpp $(NUMBERS)/words/1.6.1.12.cpp $(NUMBERS)/words/1.6.1.13.cpp \
               $(NUMBERS)/words/1.6.1.14.cpp $(NUMBERS)/words/1.6.1.15.cpp $(NUMBERS)/words/1.6.1.16.cpp \
               $(NUMBERS)/words/1.6.1.17.cpp $(NUMBERS)/words/1.6.1.18.cpp $(NUMBERS)/words/1.6.1.19.cpp \
               $(NUMBERS)/words/1.6.1.20.cpp $(NUMBERS)/words/1.6.1.21.cpp $(NUMBERS)/words/1.6.1.22.cpp \
               $(NUMBERS)/words/1.6.1.23.cpp $(NUMBERS)/words/1.6.1.24.cpp $(NUMBERS)/words/1.6.1.25.cpp \
               $(NUMBERS)/words/1.6.1.26.cpp \
               $(NUMBERS)/words/1.6.2.2.cpp $(NUMBERS)/words/1.6.2.5.cpp $(NUMBERS)/words/1.6.2.6.cpp \
               $(NUMBERS)/words/1.6.2.7.cpp $(NUMBERS)/words/1.6.2.8.cpp $(NUMBERS)/words/1.6.2.9.cpp \
               $(NUMBERS)/words/1.6.2.10.cpp $(NUMBERS)/words/1.6.2.12.cpp $(NUMBERS)/words/1.6.2.13.cpp \
               $(NUMBERS)/words/1.6.2.14.cpp $(NUMBERS)/words/1.6.2.15.cpp $(NUMBERS)/words/1.6.2.16.cpp \
               $(NUMBERS)/words/1.6.2.17.cpp $(NUMBERS)/words/1.6.2.18.cpp $(NUMBERS)/words/1.6.2.19.cpp \
               $(NUMBERS)/words/1.6.2.20.cpp $(NUMBERS)/words/1.6.2.21.cpp $(NUMBERS)/words/1.6.2.22.cpp \
               $(NUMBERS)/words/1.6.2.23.cpp $(NUMBERS)/words/1.6.2.24.cpp $(NUMBERS)/words/1.6.2.25.cpp \
               $(NUMBERS)/words/1.6.2.26.cpp $(NUMBERS)/words/1.6.2.27.cpp $(NUMBERS)/words/1.6.2.28.cpp \
               $(NUMBERS)/words/1.6.2.29.cpp $(NUMBERS)/words/1.6.2.30.cpp \
               $(NUMBERS)/words/1.6.13.1.cpp $(NUMBERS)/words/1.6.13.2.cpp $(NUMBERS)/words/1.6.13.3.cpp \
               $(NUMBERS)/words/1.6.13.4.cpp $(NUMBERS)/words/1.6.23.1.cpp $(NUMBERS)/words/1.6.23.2.cpp \
               $(NUMBERS)/words/1.6.23.3.cpp $(NUMBERS)/words/1.6.23.4.cpp $(NUMBERS)/words/1.6.23.5.cpp \
               $(NUMBERS)/words/1.6.23.6.cpp $(NUMBERS)/words/1.6.23.7.cpp $(NUMBERS)/words/1.6.23.8.cpp \
               $(NUMBERS)/words/1.6.23.9.cpp $(NUMBERS)/words/1.6.23.10.cpp $(NUMBERS)/words/1.6.23.11.cpp \
               $(NUMBERS)/words/1.6.23.12.cpp $(NUMBERS)/words/1.6.24.1.cpp $(NUMBERS)/words/1.6.24.2.cpp \
               $(NUMBERS)/words/1.6.24.3.cpp $(NUMBERS)/words/1.6.24.4.cpp $(NUMBERS)/words/1.6.24.5.cpp \
               $(NUMBERS)/words/1.6.24.6.cpp $(NUMBERS)/words/1.6.24.7.cpp $(NUMBERS)/words/1.6.24.8.cpp \
               $(NUMBERS)/words/1.6.24.9.cpp $(NUMBERS)/words/1.6.24.10.cpp $(NUMBERS)/words/1.6.24.11.cpp \
               $(NUMBERS)/words/1.6.24.12.cpp \
               $(NUMBERS)/words/1.10.1.cpp $(NUMBERS)/words/1.10.2.cpp $(NUMBERS)/words/1.10.3.cpp \
               $(NUMBERS)/words/1.10.4.cpp $(NUMBERS)/words/1.10.5.cpp \
               $(NUMBERS)/words/1.6.18.1.cpp $(NUMBERS)/words/1.6.18.2.cpp $(NUMBERS)/words/1.6.18.3.cpp \
               $(NUMBERS)/words/1.6.18.4.cpp $(NUMBERS)/words/1.6.18.5.cpp $(NUMBERS)/words/1.6.18.6.cpp \
               $(NUMBERS)/words/1.6.18.7.cpp $(NUMBERS)/words/1.6.18.8.cpp $(NUMBERS)/words/1.6.18.9.cpp \
               $(NUMBERS)/words/1.6.18.10.cpp $(NUMBERS)/words/1.6.18.11.cpp $(NUMBERS)/words/1.6.18.12.cpp \
               $(NUMBERS)/words/1.6.18.13.cpp $(NUMBERS)/words/1.6.18.14.cpp $(NUMBERS)/words/1.6.18.15.cpp \
               $(NUMBERS)/words/1.6.18.16.cpp $(NUMBERS)/words/1.6.18.17.cpp $(NUMBERS)/words/1.6.18.18.cpp \
               $(NUMBERS)/words/1.6.18.19.cpp $(NUMBERS)/words/1.6.18.20.cpp $(NUMBERS)/words/1.6.18.21.cpp \
               $(NUMBERS)/words/1.6.18.22.cpp $(NUMBERS)/words/1.6.18.23.cpp $(NUMBERS)/words/1.6.18.24.cpp \
               $(NUMBERS)/words/1.6.18.25.cpp $(NUMBERS)/words/1.6.18.26.cpp $(NUMBERS)/words/1.6.18.27.cpp \
               $(NUMBERS)/words/1.6.18.28.cpp $(NUMBERS)/words/1.6.18.29.cpp $(NUMBERS)/words/1.6.18.30.cpp \
               $(NUMBERS)/words/1.6.18.31.cpp $(NUMBERS)/words/1.6.18.32.cpp $(NUMBERS)/words/1.6.18.33.cpp \
               $(NUMBERS)/words/1.6.18.34.cpp $(NUMBERS)/words/1.6.18.35.cpp $(NUMBERS)/words/1.6.18.36.cpp \
               $(NUMBERS)/words/1.6.18.37.cpp $(NUMBERS)/words/1.6.18.38.cpp $(NUMBERS)/words/1.6.18.39.cpp \
               $(NUMBERS)/words/1.6.18.40.cpp $(NUMBERS)/words/1.6.18.41.cpp $(NUMBERS)/words/1.6.18.42.cpp \
               $(NUMBERS)/words/1.6.18.43.cpp $(NUMBERS)/words/1.6.18.44.cpp $(NUMBERS)/words/1.6.18.45.cpp \
               $(NUMBERS)/words/1.6.18.46.cpp $(NUMBERS)/words/1.6.18.47.cpp $(NUMBERS)/words/1.6.18.48.1.cpp \
               $(NUMBERS)/words/1.6.18.48.2.cpp $(NUMBERS)/words/1.6.18.49.1.1.cpp $(NUMBERS)/words/1.6.18.49.1.2.cpp \
               $(NUMBERS)/words/1.6.18.50.cpp $(NUMBERS)/words/1.6.18.51.cpp $(NUMBERS)/words/1.6.18.52.cpp \
               $(NUMBERS)/words/1.6.18.53.cpp \
               $(NUMBERS)/words/1.6.3.1.cpp $(NUMBERS)/words/1.6.5.1.cpp $(NUMBERS)/words/1.6.5.2.cpp \
               $(NUMBERS)/words/1.6.5.3.cpp $(NUMBERS)/words/1.6.5.4.cpp $(NUMBERS)/words/1.6.5.5.cpp \
               $(NUMBERS)/words/1.6.5.6.cpp $(NUMBERS)/words/1.6.5.8.cpp $(NUMBERS)/words/1.6.5.9.cpp \
               $(NUMBERS)/words/1.6.5.10.cpp $(NUMBERS)/words/1.6.5.11.cpp $(NUMBERS)/words/1.6.5.12.cpp \
               $(NUMBERS)/words/1.6.5.13.cpp $(NUMBERS)/words/1.6.5.14.cpp $(NUMBERS)/words/1.6.6.1.cpp \
               $(NUMBERS)/words/1.6.6.2.cpp $(NUMBERS)/words/1.6.10.6.cpp $(NUMBERS)/words/1.6.20.4.cpp \
               $(NUMBERS)/words/1.6.10.1.cpp $(NUMBERS)/words/1.6.10.2.cpp $(NUMBERS)/words/1.6.10.3.cpp \
               $(NUMBERS)/words/1.6.10.4.cpp $(NUMBERS)/words/1.6.10.5.cpp $(NUMBERS)/words/1.6.11.1.cpp \
               $(NUMBERS)/words/1.6.11.2.cpp $(NUMBERS)/words/1.6.11.3.cpp $(NUMBERS)/words/1.6.11.4.cpp \
               $(NUMBERS)/words/1.6.11.5.cpp $(NUMBERS)/words/1.6.11.7.cpp $(NUMBERS)/words/1.6.11.8.cpp \
               $(NUMBERS)/words/1.6.11.9.cpp $(NUMBERS)/words/1.6.11.10.cpp $(NUMBERS)/words/1.6.11.11.cpp \
               $(NUMBERS)/words/1.6.11.12.cpp $(NUMBERS)/words/1.6.16.1.cpp $(NUMBERS)/words/1.6.16.2.cpp \
               $(NUMBERS)/words/1.6.19.1.cpp $(NUMBERS)/words/1.6.19.2.cpp $(NUMBERS)/words/1.6.19.3.cpp \
               $(NUMBERS)/words/1.6.19.4.cpp $(NUMBERS)/words/1.6.19.5.cpp $(NUMBERS)/words/1.6.20.1.cpp \
               $(NUMBERS)/words/1.6.20.2.cpp $(NUMBERS)/words/1.6.20.3.cpp \
               $(NUMBERS)/words/1.6.4.1.cpp $(NUMBERS)/words/1.6.4.2.cpp $(NUMBERS)/words/1.6.4.3.cpp \
               $(NUMBERS)/words/1.6.4.4.cpp $(NUMBERS)/words/1.6.4.5.cpp $(NUMBERS)/words/1.6.4.6.cpp \
               $(NUMBERS)/words/1.6.4.7.cpp $(NUMBERS)/words/1.6.4.8.cpp $(NUMBERS)/words/1.6.4.9.cpp \
               $(NUMBERS)/words/1.6.4.10.cpp $(NUMBERS)/words/1.6.4.11.cpp $(NUMBERS)/words/1.6.4.12.cpp \
               $(NUMBERS)/words/1.6.4.13.cpp $(NUMBERS)/words/1.6.4.14.cpp $(NUMBERS)/words/1.6.4.15.cpp \
               $(NUMBERS)/words/1.6.4.17.cpp $(NUMBERS)/words/1.6.4.18.cpp $(NUMBERS)/words/1.6.4.19.cpp \
               $(NUMBERS)/words/1.6.4.20.cpp $(NUMBERS)/words/1.6.4.21.cpp $(NUMBERS)/words/1.6.4.22.cpp \
               $(NUMBERS)/words/1.6.4.23.cpp $(NUMBERS)/words/1.6.17.1.cpp $(NUMBERS)/words/1.6.17.2.cpp \
               $(NUMBERS)/words/1.6.17.3.cpp $(NUMBERS)/words/1.6.17.4.cpp \
               $(NUMBERS)/words/1.18.1.cpp $(NUMBERS)/words/1.18.4.cpp $(NUMBERS)/words/1.18.5.cpp \
               $(NUMBERS)/words/1.18.6.cpp $(NUMBERS)/words/1.18.8.cpp $(NUMBERS)/words/1.25.cpp $(NUMBERS)/words/1.25.1.cpp

INTERPRETER_SOURCES = $(SATELLITE)/structured-library.cpp \
                      $(ARGUMENTS)/arguments.cpp $(ARGUMENTS)/command_line.cpp \
                      $(SATELLITE)/licenses/licenses.cpp \
                      $(MACHINE)/machine_state.cpp $(MACHINE)/stack_share.cpp $(MACHINE)/own_environment.cpp $(SATELLITE)/satl/satl_file.cpp \
                      $(MACHINE)/satellite_of_its_own.cpp $(MACHINE)/ask_at_the_console.cpp $(MACHINE)/as_another_account.cpp \
                      $(SATELLITE)/satl/session.cpp $(SATELLITE)/satl/listing.cpp $(SATELLITE)/satl/listing_counts.cpp \
                      $(SATELLITE)/satl/listing_progress.cpp $(SATELLITE)/satl/drives.cpp $(SATELLITE)/satl/console_files.cpp \
                      $(SATELLITE)/display/printing_satellite.cpp $(SATELLITE)/display/display_stream.cpp \
                      $(SATELLITE)/satl/prompt_run.cpp $(SATELLITE)/satl/prompt_help.cpp \
                      $(BYTECODE)/main_arguments.cpp \
                      $(PROMPT)/raw_mode.cpp $(PROMPT)/keys.cpp $(PROMPT)/editor.cpp \
                      $(PROMPT)/render.cpp $(PROMPT)/line_reader.cpp $(PROMPT)/console_lines.cpp \
                      $(PROMPT)/earlier_consoles.cpp \
                      $(SATELLITE)/threads/startup_threads.cpp \
                      $(BYTECODE)/bytecode_registry.cpp $(BYTECODE)/cascade_convert.cpp \
                      $(BYTECODE)/function_table.cpp $(BYTECODE)/include_shape.cpp \
                      $(BYTECODE)/program_walk.cpp $(BYTECODE)/program_check.cpp $(BYTECODE)/type_shape.cpp \
                      $(BYTECODE)/capsule_scopes.cpp $(BYTECODE)/capsule_reach.cpp \
                      $(BYTECODE)/suit_scan.cpp $(BYTECODE)/suit_reach.cpp $(BYTECODE)/suit_run.cpp \
                      $(BYTECODE)/capsule_calls.cpp $(BYTECODE)/pointer_calls.cpp $(BYTECODE)/library_values.cpp \
                      $(BYTECODE)/argument_switches.cpp $(BYTECODE)/setting_writes.cpp \
                      $(BYTECODE)/expression.cpp \
                      $(BYTECODE)/file_calls.cpp $(BYTECODE)/info_calls.cpp $(BYTECODE)/convert_calls.cpp $(BYTECODE)/fraction_check.cpp $(BYTECODE)/access_calls.cpp $(BYTECODE)/access_words.cpp $(BYTECODE)/delete_calls.cpp $(BYTECODE)/satellite_legal.cpp $(BYTECODE)/container_calls.cpp \
                      $(BYTECODE)/string_calls.cpp $(BYTECODE)/string_check.cpp $(BYTECODE)/number_calls.cpp \
                      $(BYTECODE)/scalar_calls.cpp \
                      $(BYTECODE)/console_style.cpp $(BYTECODE)/console_calls.cpp $(SATELLITE)/satellite_variable_file/satellite_file.cpp \
                      $(SATELLITE)/satellite_variable_file/file_moves.cpp \
                      $(BYTECODE)/infinity_calls.cpp $(INFINITY)/satellite_infinity.cpp \
                      $(BYTECODE)/thread_calls.cpp $(OBJECT)/object_lock.cpp \
                      $(BYTECODE)/program_calls.cpp $(SATELLITE)/satellite_variable_program/program_spawn.cpp \
                      $(SATELLITE)/satellite_variable_program/program_watch.cpp \
                      $(SATELLITE)/satellite_variable_program/program_stop.cpp \
                      $(BYTECODE)/random_calls.cpp $(RANDOM_DIR)/random_source.cpp $(RANDOM_DIR)/random_draw.cpp \
                      $(RANDOM_DIR)/random_spin.cpp \
                      $(BYTECODE)/time_calls.cpp $(BYTECODE)/drive_calls.cpp $(MACHINE)/drive_mounts.cpp \
                      $(BYTECODE)/test_calls.cpp $(SATELLITE)/satellite_test/test_run.cpp \
                      $(SATELLITE)/satellite_test/test_programs.cpp \
                      $(SATELLITE)/satellite_test/speed_objects_twins.cpp $(SATELLITE)/satellite_test/speed_values_twins.cpp \
                      $(BYTECODE)/window_calls.cpp $(BYTECODE)/window_readers.cpp $(BYTECODE)/window_shapes.cpp \
                      $(BYTECODE)/window_methods.cpp $(BYTECODE)/window_run.cpp \
                      $(OBJECT)/satellite_object.cpp $(OBJECT)/object_copy.cpp \
                      $(OBJECT)/str_add_str.cpp $(OBJECT)/str_minus_str.cpp $(OBJECT)/str_find_str.cpp \
                      $(OBJECT)/num_add_num.cpp $(OBJECT)/num_sub_num.cpp $(OBJECT)/num_div_num.cpp \
                      $(OBJECT)/object_convert.cpp $(OBJECT)/object_percentage.cpp \
                      $(FLOAT_SOURCES) $(HEX_SOURCES) $(COLOR_SOURCES) $(FRACTION_SOURCES) \
                      $(NUMBER)/satellite_number.cpp $(NUMBER)/satellite_number_divide.cpp \
                      $(NUMBER)/satellite_number_text.cpp $(NUMBER)/satellite_number_power.cpp \
                      $(STRING16)/satellite_string.cpp \
                      $(NUMBERS)/call_number.satellite.cpp $(WORD_SOURCES)

HEADERS = $(ARGUMENTS)/arguments.hpp $(ARGUMENTS)/argument_words.hpp $(ARGUMENTS)/argument_settings.hpp \
          $(ARGUMENTS)/command_line.hpp $(ARGUMENTS)/cpu_facts.hpp \
          $(ARGUMENTS)/argument_value.hpp $(ARGUMENTS)/argument_case.hpp \
          $(ARGUMENTS)/satellite_arguments.hpp $(SATELLITE)/config/satellite_config.hpp \
          $(SATELLITE)/config/config_file.hpp $(SATELLITE)/config/feature_register.hpp \
          $(SATELLITE)/config/feature_switch.hpp $(SATELLITE)/config/rebuild.hpp \
          $(SATELLITE)/config/machine_probe.hpp $(SATELLITE)/config/run_config.hpp \
          $(SATELLITE)/config/run_feedback.hpp $(NUMBERS)/feedback_book.hpp \
          $(MACHINE)/critical_report.hpp $(MACHINE)/satellite_log.hpp $(MACHINE)/source_position.hpp $(MACHINE)/s_codes.hpp \
          $(MACHINE)/machine_codes.hpp $(MACHINE)/machine_state.hpp $(MACHINE)/exit_status.hpp $(MACHINE)/shown.hpp \
          $(MACHINE)/stop_flag.hpp $(MACHINE)/input_source.hpp $(MACHINE)/stack_share.hpp $(MACHINE)/satellite_of_its_own.hpp $(MACHINE)/ask_at_the_console.hpp $(MACHINE)/as_another_account.hpp $(MACHINE)/own_environment.hpp $(MACHINE)/stack_segments.hpp $(MACHINE)/run_state.hpp $(SATELLITE)/satl/session.hpp $(SATELLITE)/satl/listing.hpp $(SATELLITE)/satl/listing_counts.hpp \
          $(SATELLITE)/satl/listing_progress.hpp $(SATELLITE)/satl/drives.hpp $(MACHINE)/filesystems.hpp \
          $(SATELLITE)/satl/console_files.hpp $(MACHINE)/console_store.hpp \
          $(SATELLITE)/display/printing_satellite.hpp \
          $(SATELLITE)/satl/prompt_run.hpp $(SATELLITE)/satl/prompt_help.hpp $(SATELLITE)/satl/prompt_shorts.hpp \
          $(BYTECODE)/main_arguments.hpp \
          $(PROMPT)/raw_mode.hpp $(PROMPT)/keys.hpp $(PROMPT)/editor.hpp \
          $(PROMPT)/render.hpp $(PROMPT)/line_reader.hpp $(PROMPT)/console_lines.hpp $(PROMPT)/earlier_consoles.hpp \
          $(NUMBERS)/directory_words.hpp \
          $(SATELLITE)/satl/satl_file.hpp $(SATELLITE)/threads/startup_threads.hpp \
          $(VERSION_DIR)/version.hpp $(VERSION_DIR)/title_lines.hpp \
          $(BYTECODE)/bytecode_registry.hpp $(BYTECODE)/token_codes.hpp \
          $(BYTECODE)/word_codes.hpp $(BYTECODE)/word_counts.hpp $(BYTECODE)/statement_ring.hpp \
          $(BYTECODE)/function_table.hpp \
          $(BYTECODE)/include_shape.hpp $(BYTECODE)/program_walk.hpp \
          $(BYTECODE)/capsule_scopes.hpp $(BYTECODE)/capsule_key.hpp \
          $(BYTECODE)/capsule_scan.hpp $(BYTECODE)/suit_layout.hpp $(BYTECODE)/suit_run.hpp \
          $(BYTECODE)/capsule_calls.hpp $(BYTECODE)/library_values.hpp $(BYTECODE)/argument_switches.hpp \
          $(BYTECODE)/setting_writes.hpp \
          $(BYTECODE)/expression.hpp $(BYTECODE)/value.hpp \
          $(BYTECODE)/thread_calls.hpp $(SATELLITE)/satellite_variable_thread/satellite_thread.hpp \
          $(SATELLITE)/satellite_variable_thread/satellite_thread_handle.hpp \
          $(BYTECODE)/program_calls.hpp $(SATELLITE)/satellite_variable_program/satellite_program.hpp \
          $(SATELLITE)/satellite_variable_program/satellite_program_handle.hpp \
          $(SATELLITE)/satellite_variable_program/program_spawn.hpp \
          $(SATELLITE)/satellite_variable_program/program_watch.hpp \
          $(SATELLITE)/satellite_variable_program/program_stop.hpp \
          $(BYTECODE)/random_calls.hpp $(RANDOM_DIR)/random_source.hpp $(RANDOM_DIR)/random_draw.hpp $(RANDOM_DIR)/random_spin.hpp \
          $(BYTECODE)/time_calls.hpp $(SATELLITE)/satellite_variable_time/satellite_time.hpp \
          $(BYTECODE)/drive_calls.hpp $(MACHINE)/drive_mounts.hpp \
          $(BYTECODE)/test_calls.hpp $(SATELLITE)/satellite_test/test_run.hpp $(SATELLITE)/satellite_test/test_programs.hpp \
          $(SATELLITE)/satellite_test/speed_twins.hpp $(SATELLITE)/satellite_test/speed_objects_twins.hpp \
          $(SATELLITE)/satellite_test/speed_values_twins.hpp \
          $(RANDOM_DIR)/pcg_512.hpp $(RANDOM_DIR)/wide_unsigned.hpp $(RANDOM_DIR)/random_constants.hpp \
          $(MACHINE)/console_lock.hpp $(MACHINE)/thread_stop.hpp $(OBJECT)/object_lock.hpp \
          $(BYTECODE)/file_calls.hpp $(BYTECODE)/info_calls.hpp $(BYTECODE)/convert_calls.hpp $(BYTECODE)/fraction_check.hpp $(BYTECODE)/access_calls.hpp $(BYTECODE)/access_words.hpp $(BYTECODE)/delete_calls.hpp $(BYTECODE)/satellite_legal.hpp $(BYTECODE)/container_calls.hpp \
          $(BYTECODE)/string_calls.hpp $(BYTECODE)/number_calls.hpp $(BYTECODE)/method_periods.hpp $(OBJECT)/string_pieces.hpp \
          $(BYTECODE)/scalar_calls.hpp \
          $(BYTECODE)/console_style.hpp $(BYTECODE)/console_calls.hpp $(SATELLITE)/satellite_variable_file/satellite_file.hpp \
          $(SATELLITE)/satellite_variable_file/file_moves.hpp \
          $(BYTECODE)/infinity_calls.hpp $(INFINITY)/satellite_infinity.hpp \
          $(BYTECODE)/window_calls.hpp $(BYTECODE)/window_readers.hpp \
          $(WINDOW_DIR)/satellite_window.hpp $(WINDOW_DIR)/window_desk.hpp \
          $(WINDOW_DIR)/window_spill.hpp $(WINDOW_DIR)/window_menu.hpp $(WINDOW_DIR)/window_canvas.hpp \
          $(WINDOW_DIR)/window_frame.hpp $(WINDOW_DIR)/window_console.hpp $(WINDOW_DIR)/console_feed.hpp \
          $(WINDOW_DIR)/console_scrolling.hpp \
          $(WINDOW_DIR)/console_shadow.hpp \
          $(BYTECODE)/cascade_convert.hpp \
          $(NUMBER)/satellite_number.hpp $(NUMBER)/satellite_number_limbs.hpp \
          $(NUMBER)/number_arithmetic.hpp $(NUMBER)/number_conversions.hpp \
          $(STRING16)/satellite_string.hpp $(STRING16)/character_table.hpp \
          $(STRING16)/conversion_loops.hpp $(STRING16)/string_overwrite.hpp \
          $(BINARY)/satellite_binary_number.hpp \
          $(PERCENTAGE)/satellite_percentage.hpp \
          $(FLOAT_HEADERS) $(HEX_HEADERS) $(COLOR_HEADERS) $(FRACTION_HEADERS) \
          $(OBJECT)/satellite_object.hpp $(OBJECT)/satellite_spacesuit.hpp \
          $(OBJECT)/satellite_pointer.hpp $(OBJECT)/object_copy.hpp $(BYTECODE)/pointer_calls.hpp \
          $(OBJECT)/satellite_list.hpp $(OBJECT)/satellite_index.hpp $(OBJECT)/satellite_map_entry.hpp $(OBJECT)/satellite_map_sort.hpp \
          $(BYTECODE)/type_shape.hpp \
          $(OBJECT)/satellite_bytecode.hpp $(OBJECT)/satellite_capsule.hpp \
          $(OBJECT)/fast_paths.hpp $(OBJECT)/object_pair.hpp \
          $(wildcard $(OBJECT)/*_and_*.hpp) $(wildcard $(OBJECT)/*_to_*.hpp) \
          $(NUMBERS)/call_number.hpp $(NUMBERS)/number_row.hpp $(NUMBERS)/word_table.hpp $(NUMBERS)/machine_facts.hpp $(SATELLITE)/satellite_object/string_case.hpp $(STRINGS32)/string_method.hpp \
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
# satl and its build. The words are in INTERPRETER_SOURCES through WORD_SOURCES since
# 2026-10-07 (their old folders held brackets and spaces, which make cannot list, so
# build_number.py globbed them itself). GTK_SOURCES is 047-window.mk's, read when this
# is expanded.
MAKE_FRAGMENTS = make_support/005-jobs.mk make_support/010-compiler.mk make_support/020-version.mk \
                 make_support/030-directories.mk make_support/040-sources.mk make_support/045-optimise.mk make_support/047-window.mk \
                 make_support/048-link.mk make_support/050-build.mk make_support/060-compile.mk \
                 make_support/065-tests.mk make_support/070-clean.mk
#
# AND THE VENDORED PCG HEADERS satl's generators are compiled from (030-directories.mk):
# a system include to the compiler, so -MMD leaves them out of the .d files and check.sh's
# fingerprint row never asks for them -- they are named here instead, so that the next
# upstream release is a new build and not the same number over different code.
PCG_HEADERS = $(PCG_INCLUDE)/pcg_random.hpp $(PCG_INCLUDE)/pcg_extras.hpp $(PCG_INCLUDE)/pcg_uint128.hpp
BUILD_INPUTS = $(INTERPRETER_SOURCES) $(HEADERS) $(GTK_SOURCES) Makefile $(MAKE_FRAGMENTS) $(TRAINING) \
               $(SATELLITE)/config/build_number.py words/words.tsv $(PCG_HEADERS)
