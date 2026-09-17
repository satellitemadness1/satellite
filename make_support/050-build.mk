# satellite 004 -- the default goal and the binaries.
#
# THE FIRST FRAGMENT THAT DECLARES A TARGET; .DEFAULT_GOAL says so outright anyway.
#
# THE THREE ARE BUILT INTO ONE FOLDER (PLAN M0.5): build/satl loads the libraries
# in the satellite-numbers/ beside its own path, and build/satl-term runs the satl
# beside its own. So they are built together and installed together.
.DEFAULT_GOAL := all

ALL_TARGETS = $(BUILD)/satl libraries $(BUILD)/satellite-004
ifeq ($(HAVE_WINDOW),yes)
  ALL_TARGETS += $(BUILD)/satl-term
endif

all: $(ALL_TARGETS)
ifneq ($(HAVE_WINDOW),yes)
	@echo "note: satl-term not built -- pkg-config finds no $(WINDOW_PKGS). satl is unaffected."
endif

# Runs on every make; build_number.py decides whether this make is a build, and
# rewrites the stamp only when it is -- which is what recompiles the objects that
# read the rows (060-compile.mk) and so relinks.
$(BUILD_STAMP): FORCE
	@python3 $(SATELLITE)/config/build_number.py $@ --also "$(BUILD_DESCRIPTION)" -- $(BUILD_INPUTS)

$(LINK_STAMP): FORCE
	@mkdir -p $(BUILD)
	@printf '%s' '$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS)' | cmp -s - $@ || \
	    printf '%s' '$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS)' > $@

# THE NUMBER THE BINARY SHOWS IS CHECKED AGAINST THE ROW, after every link. A
# dependency that misses the rows (060-compile.mk's ROW_READERS) would leave the
# old number compiled in with nothing saying so; this says so and fails.
# --verify refuses a row that an editor changed while the build ran.
define shows_the_build_row
@python3 $(SATELLITE)/config/build_number.py $(BUILD_STAMP) --verify
@row=$$(python3 $(SATELLITE)/config/build_number.py --print arguments.build) && \
 shown=$$($(1) --version | sed -n 2p) && \
 case "$$shown" in *" BUILD $$(printf %04d "$$row")") ;; \
 *) echo "$(1) shows \"$$shown\" but arguments.build is $$row: an object that reads the rows was not rebuilt (060-compile.mk ROW_READERS)" >&2; \
    rm -f $(1); exit 1 ;; esac
endef

$(BUILD)/satl: $(INTERPRETER_OBJECTS) $(LINK_STAMP)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(INTERPRETER_OBJECTS) -o $@ -ldl
	$(call shows_the_build_row,$@)

# THE OLD NAME IS A LINK TO THE NEW ONE. build/satellite-004 was the binary until
# M0.5, and the author's `satl` alias still names it; a link keeps that alias
# running THIS build rather than the last one made under the old name. satl loads
# its libraries beside /proc/self/exe, which is build/satl, so the link runs.
$(BUILD)/satellite-004: FORCE | $(BUILD)/satl
	@[ -L $@ ] && [ "$$(readlink $@)" = satl ] || { ln -sfn satl $@ && echo "$@ -> satl (the binary's name since M0.5)"; }

# Every numbered library, by satellite-numbers/build_libraries.py: make cannot
# name them, because a word with arguments has brackets and make reads
# name(member) as an archive. It is handed THIS make's compiler and flags, and
# rebuilds every library when they differ from what built the ones there, so satl
# and its libraries never come from two compilers without anything saying so.
libraries: $(BUILD_STAMP) $(LINK_STAMP)
	@SATELLITE_CXX="$(CXX)" SATELLITE_CXXFLAGS="$(CXXFLAGS)" SATELLITE_LDFLAGS="$(LDFLAGS)" \
	    $(LINK_ENV) python3 $(NUMBERS)/build_libraries.py

# $(WINDOW_LIBS) AFTER the objects: a linker resolves an -l only against the
# symbols it has already been asked for.
ifeq ($(HAVE_WINDOW),yes)

$(BUILD)/satl-term: $(TERM_OBJECTS) $(LINK_STAMP)
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(TERM_OBJECTS) -o $@ $(WINDOW_LIBS)
	$(call shows_the_build_row,$@)

else

# NO PREREQUISITES, so nothing is compiled before the reason is given.
.PHONY: $(BUILD)/satl-term
$(BUILD)/satl-term:
	@echo "satl-term needs $(WINDOW_PKGS), which pkg-config cannot find." >&2
	@echo "  AlmaLinux/RHEL: dnf --enablerepo=crb install vte291-gtk4-devel" >&2
	@echo "  Debian/Ubuntu:  apt install libvte-2.91-gtk4-dev" >&2
	@echo "satl and its libraries do not need it." >&2
	@false

endif

FORCE:

.PHONY: all libraries FORCE
