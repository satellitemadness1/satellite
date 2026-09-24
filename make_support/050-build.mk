# satellite 004 -- the default goal and the binaries.
#
# .DEFAULT_GOAL IS WHAT MAKES `all` THE DEFAULT, and it is load-bearing: 047 declares
# `window` first (2026-09-23), so a plain make without this line would build GTK.
#
# THE TWO ARE BUILT INTO ONE FOLDER (PLAN M0.5): build/satl loads the libraries
# in the satellite-numbers/ beside its own path, so they are built together and
# installed together. There were three until 2026-09-22; satl-term is gone, and
# satl opens its own console (047-window.mk says so at the top).
.DEFAULT_GOAL := all

# The three harnesses check.sh runs are built too, so ./check.sh works after a plain
# make or an install. They do not depend on the build stamp and raise no number.
ALL_TARGETS = $(BUILD)/satl libraries $(BUILD)/satellite-004 $(BUILD)/exit_status_cases $(BUILD)/arguments_cases \
              $(BUILD)/count_cases $(BUILD)/prompt_cases $(BUILD)/prompt_reader $(BUILD)/directory_cases \
              $(BUILD)/file_cases $(BUILD)/infinity_cases $(BUILD)/satl-cpu-level

# AN EARLIER BUILD'S satl-term IS REMOVED, with its objects: it would run the
# newer satl beside it and be installed with it, and its .d files would name
# headers that no longer exist to check.sh's fingerprint row.
# ONE PROCESSOR'S BUILD IS satl AND ITS LIBRARIES, and what it was compiled to need
# (055-cpus.mk): the harnesses test the language, which one build already does.
ifneq ($(CPU),)
ALL_TARGETS = $(BUILD)/satl libraries $(BUILD)/satellite.help $(BUILD)/needs
# AND THE CHOOSER BEFORE ITS LINK, which asks it whether an illegal instruction was this
# machine's lack or the build's defect (shows_the_build_row).
$(BUILD)/satl: | $(CPU_LEVEL)
endif

all: $(ALL_TARGETS)
	@if [ -e $(BUILD)/satl-term ] || [ -d $(OBJECTS)/satl-term ]; then \
	     rm -rf $(BUILD)/satl-term $(OBJECTS)/satl-term && \
	     echo "removed $(BUILD)/satl-term, which an earlier build made -- satl opens its own console now"; fi
# SAID ONCE, BY THE MAKE THAT WAS TYPED: `make cpus` runs a make per processor inside it.
ifeq ($(HAVE_GTK)$(MAKELEVEL),no0)
	@echo "" && \
	 echo "  THIS satl HAS NO WINDOW. It runs every program; the window and console words refuse by name." && \
	 echo "  $(if $(filter vendor,$(GTK)),The GTK it carries is built from vendor/new/ and is not built in this checkout yet:,GTK=system found no gtk4 on this machine.)" && \
	 echo "  $(if $(filter vendor,$(GTK)),make window    (about three minutes; make_support/047-window.mk says what it needs),make GTK=vendor window)" && \
	 echo ""
endif

# Runs on every make; build_number.py decides whether this make is a build, and
# rewrites the stamp only when it is -- which is what recompiles the objects that
# read the rows (060-compile.mk) and so relinks.
#
# ONE PROCESSOR'S BUILD (010-compiler.mk's CPU) ASKS AND NEVER RAISES: it is the ordinary
# BUILD N for that processor, so it refuses when a source changed since BUILD N was made.
# AND ITS LIST OF NEEDS GOES FIRST (055-cpus.mk writes it last), so a build that fails or
# is stopped half way is not a build satl-cpu-level offers.
$(BUILD_STAMP): FORCE
	@$(if $(CPU),rm -f $(BUILD)/needs && )python3 $(SATELLITE)/config/build_number.py $@ $(if $(CPU),--same) --also "$(BUILD_DESCRIPTION)" -- $(BUILD_INPUTS)

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
#
# A PROCESSOR'S BUILD THIS MACHINE CANNOT RUN (010-compiler.mk's CPU) dies of an illegal
# instruction, 132, before it can show anything: that one is kept and said, because
# building for processors nobody here owns is the point (055-cpus.mk). BUT ONLY WHEN
# satl-cpu-level CONFIRMS this machine lacks something the build was compiled to need --
# an illegal instruction on a machine that has all of it is a defect in the build (a
# compiler's trap, a bad flag), and is deleted like any other failure (the review,
# 2026-09-23: every 132 was kept, whatever raised it). Any other failure is a failure.
define shows_the_build_row
@python3 $(SATELLITE)/config/build_number.py $(BUILD_STAMP) --verify
@row=$$(python3 $(SATELLITE)/config/build_number.py --print arguments.build) && \
 want=$$row && while [ $${#want} -lt 4 ]; do want=0$$want; done && \
 lines=$$($(1) --version); status=$$?; \
 if [ $$status = 132 ] && [ -n "$(CPU)" ]; then \
     $(call write_needs,$(1).needs); \
     lacking=$$($(CPU_LEVEL) --runs $(1).needs); runs=$$?; rm -f $(1).needs; \
     if [ $$runs = 1 ]; then \
         echo "$(1): this processor lacks what a $(CPU) build needs ($$(echo $$lacking | cut -c1-60) ...), so it is kept without showing its BUILD row"; exit 0; fi; \
     echo "$(1) died of an illegal instruction on a processor satl-cpu-level says has all it needs -- deleted" >&2; \
     rm -f $(1); exit 1; fi; \
 [ $$status = 0 ] || { echo "$(1) --version failed" >&2; rm -f $(1); exit 1; }; \
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

# $(GTK_LIBS) AFTER the objects: a linker resolves an -l only against the
# symbols it has already been asked for.
# Both are empty when pkg-config found no gtk4, and this is then exactly the link
# line it was before the window (047-window.mk).
$(BUILD)/satl: $(INTERPRETER_OBJECTS) $(GTK_OBJECTS) $(LINK_STAMP) $(BUILD_STAMP)
	@echo "linking $@ -- $(GTK_KIND)"
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(LDFLAGS) $(GTK_LINK_FLAGS) $(INTERPRETER_OBJECTS) $(GTK_OBJECTS) -o $@ -ldl $(GTK_LIBS)
	$(call shows_the_build_row,$@)
ifeq ($(GTK)$(HAVE_GTK),vendoryes)
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
	    SATELLITE_NUMBERS_OUT="$(BUILD)/satellite-numbers" \
	    $(LINK_ENV) python3 $(NUMBERS)/build_libraries.py

FORCE:

.PHONY: all libraries FORCE
