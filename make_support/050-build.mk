# satellite 004 -- the default goal and the binaries.
#
# THE FIRST FRAGMENT THAT DECLARES A TARGET; .DEFAULT_GOAL says so outright anyway.
#
# THE THREE ARE BUILT INTO ONE FOLDER (PLAN M0.5): build/satl loads the libraries
# in the satellite-numbers/ beside its own path, and build/satl-term runs the satl
# beside its own. So they are built together and installed together.
.DEFAULT_GOAL := all

# The three harnesses check.sh runs are built too, so ./check.sh works after a plain
# make or an install. They do not depend on the build stamp and raise no number.
ALL_TARGETS = $(BUILD)/satl libraries $(BUILD)/satellite-004 $(BUILD)/exit_status_cases $(BUILD)/arguments_cases \
              $(BUILD)/count_cases $(BUILD)/prompt_cases $(BUILD)/prompt_reader $(BUILD)/directory_cases \
              $(BUILD)/file_cases $(BUILD)/infinity_cases
ifeq ($(HAVE_WINDOW),yes)
  ALL_TARGETS += $(BUILD)/satl-term
endif

# WITHOUT THE WINDOW, AN EARLIER satl-term IS REMOVED: the three are one set, and
# a satl-term from an older build would run the newer satl beside it and be
# installed with it.
all: $(ALL_TARGETS)
ifneq ($(HAVE_WINDOW),yes)
	@if [ -e $(BUILD)/satl-term ]; then rm -f $(BUILD)/satl-term && echo "removed $(BUILD)/satl-term, which an earlier build made"; fi
	@echo "note: satl-term not built -- pkg-config finds no $(WINDOW_PKGS). satl is unaffected."
endif

# Runs on every make; build_number.py decides whether this make is a build, and
# rewrites the stamp only when it is -- which is what recompiles the objects that
# read the rows (060-compile.mk) and so relinks.
$(BUILD_STAMP): FORCE
	@python3 $(SATELLITE)/config/build_number.py $@ --also "$(BUILD_DESCRIPTION)" -- $(BUILD_INPUTS)

$(LINK_STAMP): FORCE
	@mkdir -p $(BUILD)
	@printf '%s' '$(LINK_ENV) $(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(LDFLAGS)' | cmp -s - $@ || \
	    printf '%s' '$(LINK_ENV) $(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(LDFLAGS)' > $@

# THE NUMBER THE BINARY SHOWS IS CHECKED AGAINST THE ROW, after every link. A
# dependency that misses the rows (060-compile.mk's ROW_READERS) would leave the
# old number compiled in with nothing saying so; this says so and fails.
# --verify refuses a row that an editor changed while the build ran. THE LINKS
# DEPEND ON THE BUILD STAMP, so this runs after every raise, not only when an
# object changed; the row is padded in the shell, because printf %04d stops at
# 2^63-1 and a row in quotes has no ceiling (review of M0.5).
define shows_the_build_row
@python3 $(SATELLITE)/config/build_number.py $(BUILD_STAMP) --verify
@row=$$(python3 $(SATELLITE)/config/build_number.py --print arguments.build) && \
 want=$$row && while [ $${#want} -lt 4 ]; do want=0$$want; done && \
 lines=$$($(1) --version) || { echo "$(1) --version failed" >&2; rm -f $(1); exit 1; }; \
 shown=$$(printf '%s\n' "$$lines" | sed -n 2p); \
 case "$$shown" in *" BUILD $$want") ;; \
 *) echo "$(1) shows \"$$shown\" but arguments.build is $$row: an object that reads the rows was not rebuilt (060-compile.mk ROW_READERS)" >&2; \
    rm -f $(1); exit 1 ;; esac
endef

# WHAT satl IS ALLOWED TO NEED AT RUN TIME, WHEN IT CARRIES GTK. Exactly these
# SEVEN -- six was the target, and DEP-2 (deferred by the author) is the way
# back to it. Nobody installs any of them: a machine without them cannot draw
# anything at all. PROVED 2026-09-22 (GTK_AND_NO_DEPENDENCIES.md DEP-3):
# satellite/satellite_variable_window/prove-bare-machine.sh binds exactly these
# into an EMPTY root -- plus libffi.so.8, which is libwayland-client's own NEEDED
# and not satl's -- and satl draws a window, runs a capsule and finds its
# carried font. With the GPU driver bound in too, and with none at all.
#
# libresolv WAS THE EIGHTH, and it was never a dependency (DEP-4, 2026-09-22):
# 047-window.mk passed -lresolv, so the linker wrote a NEEDED entry for it, and
# satl bound not one symbol to it -- gio's resolver symbols live in libc.so.6
# since glibc 2.34, below satl's floor. A fresh reader caught the first version
# of this comment calling it "a measurement"; the measurement was `readelf -V`,
# which had no version-needs for libresolv at all. The flag is gone.
#
# THE VERSION FLOOR is GLIBC_2.38 and GLIBCXX_3.4.32 (check.sh asserts both):
# AlmaLinux 10's /lib64 runs this binary, AlmaLinux 9's does not.
#
# THIS GATE IS WHY NO GTK HAS TO BE UNINSTALLED from a development machine. The
# author proposed removing it to force the vendored stack; that would have taken
# gnome-shell and mutter with it (38 packages) AND still not proved anything,
# because glib2 cannot be removed at all -- 261 packages need it, NetworkManager
# among them -- so a satl built there would link the system's gio and pass. A
# build that FAILS on an unexpected NEEDED entry is the stronger thing, and it
# holds on every machine and after every update rather than once here.
#
# readelf -d AND NOT ldd, which prints the whole transitive closure: that is what
# made libffi look like a leak until WIN-7 corrected it.
# libstdc++ AND libgcc_s ARE HERE ON A MEASUREMENT, not a shrug: -static-libstdc++
# removes them and gives satl a SECOND std::cout, which made a failed write to a
# full device report success (047-window.mk has the whole finding). They come off
# this list the day the word libraries stop being dlopened.
ALLOWED_NEEDED = libm libwayland-client libwayland-egl libc ld-linux \
                 libstdc++ libgcc_s

define carries_its_own_gtk
@found=$$(readelf -d $(1) | sed -n 's/.*Shared library: \[\([^]]*\)\].*/\1/p'); \
 bad=""; \
 for lib in $$found; do \
     base=$$(echo "$$lib" | sed 's/\.so.*//; s/-x86-64//'); \
     case " $(ALLOWED_NEEDED) " in *" $$base "*) ;; *) bad="$$bad $$lib" ;; esac; \
 done; \
 if [ -n "$$bad" ]; then \
     echo "$(1) still needs:$$bad" >&2; \
     echo "  GTK=vendor must carry its whole stack. Allowed: $(ALLOWED_NEEDED)" >&2; \
     echo "  (make_support/050-build.mk -- ALLOWED_NEEDED says why each one is allowed)" >&2; \
     rm -f $(1); exit 1; \
 fi; \
 echo "$(1): carries GTK -- needs only$$(for l in $$found; do printf ' %s' $$l; done)"
endef

# $(GTK_LIBS) AFTER the objects, and for the same reason satl-term's are: a
# linker resolves an -l only against the symbols it has already been asked for.
# Both are empty when pkg-config found no gtk4, and this is then exactly the link
# line it was before the window (047-window.mk).
$(BUILD)/satl: $(INTERPRETER_OBJECTS) $(GTK_OBJECTS) $(LINK_STAMP) $(BUILD_STAMP)
	@echo "linking $@ -- $(GTK_KIND)"
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(GTK_LINK_FLAGS) $(INTERPRETER_OBJECTS) $(GTK_OBJECTS) -o $@ -ldl $(GTK_LIBS)
	$(call shows_the_build_row,$@)
ifeq ($(GTK),vendor)
	$(call carries_its_own_gtk,$@)
endif

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
# SATELLITE_JOBS is this make's -j: `make -j4` compiles four libraries at once.
libraries: $(BUILD_STAMP) $(LINK_STAMP)
	@SATELLITE_CXX="$(CXX)" SATELLITE_CXX_VERSION="$(CXX_VERSION)" SATELLITE_CXXFLAGS="$(CXXFLAGS)" \
	    SATELLITE_LDFLAGS="$(LDFLAGS)" SATELLITE_JOBS="$(patsubst -j%,%,$(filter -j%,$(MAKEFLAGS)))" \
	    $(LINK_ENV) python3 $(NUMBERS)/build_libraries.py

# $(WINDOW_LIBS) AFTER the objects: a linker resolves an -l only against the
# symbols it has already been asked for.
ifeq ($(HAVE_WINDOW),yes)

$(BUILD)/satl-term: $(TERM_OBJECTS) $(LINK_STAMP) $(BUILD_STAMP)
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
