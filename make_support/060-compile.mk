# satellite 004 -- how a .cpp becomes a .o.
#
# ONE OBJECT A SOURCE, under build/objects/ at the source's own path, so a change
# recompiles the files it touches and not all thirty (the old Makefile compiled
# every source in one command, every time), and nothing is written beside a
# source. -MMD -MP has the compiler write each object's real header dependencies
# into a .d beside it; -MP gives every header a rule of its own, so deleting a
# header is not "no rule to make" on the next build.
DEPENDENCY_FLAGS = -MMD -MP

# BEFORE THE RULES, because make expands a prerequisite list as it reads it.
# make recompiles when a PREREQUISITE changes, and CXXFLAGS is not one: without
# this, `make OPT=-O3` over a -O2 tree recompiles nothing and links a mixed binary.
COMPILE_STAMP = $(BUILD)/.compile-flags
$(COMPILE_STAMP): FORCE
	@mkdir -p $(BUILD)
	@printf '%s' '$(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(OS_DEFINE) $(WINDOW_DEFINE)' | cmp -s - $@ || \
	    printf '%s' '$(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(OS_DEFINE) $(WINDOW_DEFINE)' > $@

# The window's own stamp, so gtk's include paths appearing or vanishing recompiles
# the window and not all of satl.
TERM_COMPILE_STAMP = $(BUILD)/.compile-flags-window
$(TERM_COMPILE_STAMP): FORCE
	@mkdir -p $(BUILD)
	@printf '%s' '$(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(OS_DEFINE) $(WINDOW_CFLAGS)' | cmp -s - $@ || \
	    printf '%s' '$(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(OS_DEFINE) $(WINDOW_CFLAGS)' > $@

# ORDER-ONLY ON THE BUILD STAMP: every object waits until build_number.py has
# decided this build's number and written it into satellite_config.hpp, so none is
# compiled from the row it is about to replace.
$(OBJECTS)/%.o: %.cpp $(COMPILE_STAMP) | $(BUILD_STAMP)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OS_DEFINE) $(WINDOW_DEFINE) $(DEPENDENCY_FLAGS) -c $< -o $@

# THE WINDOW'S OBJECTS, which need gtk's include paths. The shorter stem wins, so
# this rule, and not the one above, compiles satl-term/*.cpp.
$(OBJECTS)/$(TERM_DIR)/%.o: $(TERM_DIR)/%.cpp $(TERM_COMPILE_STAMP) | $(BUILD_STAMP)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OS_DEFINE) $(WINDOW_CFLAGS) $(DEPENDENCY_FLAGS) -c $< -o $@

# THE INTERPRETER'S OWN WINDOW OBJECTS, which need GTK's include paths and not
# VTE's -- a longer stem than the plain rule above, so this one wins for them.
# They have their own stamp for the same reason satl-term's do: gtk4 appearing or
# vanishing must recompile the window and not all of satl.
GTK_COMPILE_STAMP = $(BUILD)/.compile-flags-gtk
$(GTK_COMPILE_STAMP): FORCE
	@mkdir -p $(BUILD)
	@printf '%s' '$(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(OS_DEFINE) $(GTK_CFLAGS)' | cmp -s - $@ || \
	    printf '%s' '$(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(OS_DEFINE) $(GTK_CFLAGS)' > $@

$(OBJECTS)/$(SATELLITE)/satellite_variable_window/%.o: $(SATELLITE)/satellite_variable_window/%.cpp \
                                                       $(GTK_COMPILE_STAMP) | $(BUILD_STAMP)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(OS_DEFINE) $(GTK_CFLAGS) $(DEPENDENCY_FLAGS) -c $< -o $@

# THE CARRIED DATA. glib-compile-resources writes the .c; this compiles it. It is
# C and not C++, and it is generated, so -Wall -Wextra would report other
# people's style -- the warnings that matter about it are in the script.
$(WINDOW_DATA_SOURCE): $(WINDOW_DATA_INPUTS)
	@mkdir -p $(dir $@)
	@SATL_GTK_BUILD=$(if $(filter vendor,$(GTK)),$(GTK_BUILD)) \
	 python3 $(SATELLITE)/satellite_variable_window/make_window_data.py

# THE LICENCES. Regenerated whenever a licences/ file changes, so a licence added
# to the folder is in the next binary without anybody remembering to say so.
$(LICENCE_DATA_SOURCE): $(LICENCE_DATA_INPUTS)
	@mkdir -p $(dir $@)
	@python3 $(SATELLITE)/licenses/make_license_data.py $@

# -w, like the window data: it is 284 KB of other people's licence text in raw
# string literals, and -Wall -Wextra has nothing useful to say about it.
$(LICENCE_DATA_OBJECT): $(LICENCE_DATA_SOURCE) | $(BUILD_STAMP)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -w $(DEPENDENCY_FLAGS) -c $< -o $@

$(WINDOW_DATA_OBJECT): $(WINDOW_DATA_SOURCE) $(GTK_COMPILE_STAMP) | $(BUILD_STAMP)
	@mkdir -p $(dir $@)
	$(CC) -std=c11 $(OPT) -w $(GTK_CFLAGS) -c $< -o $@

# THE OBJECTS THAT READ THE ROWS depend on the build stamp as a real prerequisite,
# and the .d files are not enough for them. make remembers a file's time from the
# first moment it looked, and it may look at satellite_config.hpp before
# build_number.py rewrites it -- so a new build number would be compiled into
# nothing. The stamp is a target with a recipe, which make looks at again after
# the recipe runs. 050-build.mk checks every link against the row, so a file that
# reads the rows and is missing here fails the build rather than lying.
ROW_READERS = $(OBJECTS)/$(ARGUMENTS)/arguments.o $(OBJECTS)/$(SATELLITE)/structured-library.o \
              $(OBJECTS)/$(TERM_DIR)/window.o
$(ROW_READERS): $(BUILD_STAMP)

-include $(INTERPRETER_OBJECTS:.o=.d) $(TERM_OBJECTS:.o=.d) $(GTK_OBJECTS:.o=.d)
