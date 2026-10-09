# satellite 004 -- the default goal and the binaries.
#
# .DEFAULT_GOAL IS WHAT MAKES `all` THE DEFAULT, and it is load-bearing: 047 declares
# `window` first (2026-09-23), so a plain make without this line would build GTK.
#
# ONE BINARY SINCE 2026-10-07: every word is built into build/satl (satellite-numbers/
# word_table.hpp), and nothing is loaded from beside it. Until that day the numbered
# libraries were built into build/satellite-numbers/ beside it (PLAN M0.5); satl-term went
# on 2026-09-22, and satl opens its own console (047-window.mk says so at the top).
.DEFAULT_GOAL := all

# A BARE `make` IS A BUILD EVERY TIME (the author, 2026-10-07: "plain make should build the
# program" -- a make that found nothing to compile had re-run the installer and shown the same
# number, and read as make doing something else). --always: the number rises whether or not an
# input changed, the new row recompiles the two objects that read it, and satl is linked and BOLTed
# again -- the exe every time, with a number of its own. The PGO profile is kept (045-optimise.mk),
# so a make with nothing else changed is about a minute, not the whole build. AND THEN IT ASKS
# whether to install, installing unless told n (the same evening; the recipe under `all` says how).
# ONLY the make that was typed with no goal, or `all`, at the top, with no INSTALL_AFTER_BUILD on
# its line: `make install` and `make check` build what changed and raise nothing of their own,
# and neither do the makes inside a make or a script -- the installer's (which says
# INSTALL_AFTER_BUILD=no), the training build's, one processor's. ABOVE `all`, because its recipe
# asks this in an ifneq, which make answers as it reads.
BARE_MAKE = $(if $(filter-out all,$(MAKECMDGOALS))$(filter-out 0,$(MAKELEVEL))$(CPU)$(PGO_STAGE)$(findstring command,$(origin INSTALL_AFTER_BUILD)),,yes)

# The harnesses check.sh runs are built too, so ./check.sh works after a plain
# make or an install. They do not depend on the build stamp and raise no number.
ALL_TARGETS = $(BUILD)/satl $(BUILD)/satellite-004 $(BUILD)/exit_status_cases $(BUILD)/arguments_cases \
              $(BUILD)/missing_cases $(BUILD)/count_cases $(BUILD)/prompt_cases $(BUILD)/prompt_reader $(BUILD)/directory_cases \
              $(BUILD)/file_cases $(BUILD)/infinity_cases $(BUILD)/random_cases $(BUILD)/satl-cpu-level

# AN EARLIER BUILD'S satl-term IS REMOVED, with its objects: it would run the
# newer satl beside it and be installed with it, and its .d files would name
# headers that no longer exist to check.sh's fingerprint row.
# ONE PROCESSOR'S BUILD IS satl (its words inside it), and what it was compiled to need
# (055-cpus.mk): the harnesses test the language, which one build already does.
ifneq ($(CPU),)
ALL_TARGETS = $(BUILD)/satl $(BUILD)/satellite.help $(BUILD)/needs
# AND THE CHOOSER BEFORE ITS LINK, which asks it whether an illegal instruction was this
# machine's lack or the build's defect (shows_the_build_row).
$(BUILD)/satl: | $(CPU_LEVEL)
endif

all: $(ALL_TARGETS)
	@if [ -e $(BUILD)/satl-term ] || [ -d $(OBJECTS)/satl-term ]; then \
	     rm -rf $(BUILD)/satl-term $(OBJECTS)/satl-term && \
	     echo "removed $(BUILD)/satl-term, which an earlier build made -- satl opens its own console now"; fi
# AND THE LIBRARY FOLDER AN EARLIER BUILD MADE (2026-10-07): every word is inside satl, nothing reads it.
	@if [ -e $(BUILD)/satellite-numbers ] || [ -L $(TRAIN_BUILD)/satellite-numbers ]; then \
	     rm -rf $(BUILD)/satellite-numbers $(TRAIN_BUILD)/satellite-numbers && \
	     echo "removed $(BUILD)/satellite-numbers, which an earlier build made -- every word is inside satl now"; fi
# WHAT A BARE make MADE, SAID LAST (2026-10-07): the build by its number -- AND THEN IT ASKS
# (the author, the evening of 2026-10-07: "we need plain make to run make install by default, and
# plain make to ask if you want to install satellite or just build it"). The question goes to the
# terminal itself, /dev/tty, so a make whose output is piped still asks the person. Enter, y, or a
# minute's silence installs; n keeps the build where it is. With no terminal at all -- a script, a
# job -- it installs, which is the default his words give. `make INSTALL_AFTER_BUILD=no` builds and
# asks nothing; `make INSTALL_AFTER_BUILD=yes` installs and asks nothing; `make install` is the
# install by name (080-install.mk, which also holds the installer's recipe, run_the_installer).
# THE tty IS TRIED IN A SUBSHELL FIRST: a failed redirection on `exec` ends a non-interactive sh.
ifneq ($(BARE_MAKE),)
	@n=$$(python3 $(SATELLITE)/config/build_number.py --print arguments.build); while [ $${#n} -lt 4 ]; do n=0$$n; done; \
	 echo "built $(BUILD)/satl -- BUILD $$n"; \
	 answer=; \
	 if ( : <>/dev/tty ) 2>/dev/null; then \
	     printf 'Install it into ~/.satl? [Y/n] (Enter installs; so does a minute of silence) ' > /dev/tty; \
	     read -r -t 60 answer < /dev/tty || { answer=; echo > /dev/tty; }; \
	 fi; \
	 case "$$answer" in \
	     [nN]*) echo "not installed -- make install puts it in ~/.satl" ;; \
	     *) $(call run_the_installer) ;; \
	 esac
endif
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
# read the rows (060-compile.mk) and so relinks. A BARE make RAISES IT EVERY TIME: --always,
# BARE_MAKE at the top of this file.
#
# ONE PROCESSOR'S BUILD (010-compiler.mk's CPU) ASKS AND NEVER RAISES: it is the ordinary
# BUILD N for that processor, so it refuses when a source changed since BUILD N was made.
# AND ITS LIST OF NEEDS GOES FIRST (055-cpus.mk writes it last), so a build that fails or
# is stopped half way is not a build satl-cpu-level offers.
$(BUILD_STAMP): FORCE
	@$(if $(CPU),rm -f $(BUILD)/needs && )python3 $(SATELLITE)/config/build_number.py $@ $(if $(CPU)$(PGO_STAGE),--same) $(if $(BARE_MAKE),--always) --also "$(BUILD_DESCRIPTION)" -- $(BUILD_INPUTS)

$(LINK_STAMP): FORCE
	@mkdir -p $(BUILD)
	@printf '%s' '$(LINK_ENV) $(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(OPTIMISE_LINK_FLAGS) $(LDFLAGS)' | cmp -s - $@ || \
	    printf '%s' '$(LINK_ENV) $(CXX) [$(CXX_VERSION)] $(CXXFLAGS) $(OPTIMISE_LINK_FLAGS) $(LDFLAGS)' > $@

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
# full device report success (047-window.mk has the whole finding). That day came on
# 2026-10-07 -- the words are inside satl -- so taking them off, with -static-libstdc++, is
# the author's decision now (GTK_AND_NO_DEPENDENCIES.md DEP-2), not a defect.
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
	@echo "linking $@ -- $(GTK_KIND) -- $(OPTIMISE_SAID)"
	$(LINK_ENV) $(CXX) $(CXXFLAGS) $(OPTIMISE_LINK_FLAGS) $(LDFLAGS) $(GTK_LINK_FLAGS) $(INTERPRETER_OBJECTS) $(GTK_OBJECTS) -o $@ -ldl $(GTK_LIBS)
ifneq ($(BOLT),)
	$(call bolt_it,$@)
endif
	$(call shows_the_build_row,$@)
ifeq ($(GTK)$(HAVE_GTK),vendoryes)
	$(call carries_its_own_gtk,$@)
endif

# THE OLD NAME IS A LINK TO THE NEW ONE. build/satellite-004 was the binary until
# M0.5, and the author's `satl` alias still names it; a link keeps that alias
# running THIS build rather than the last one made under the old name -- this build's
# satl, every word inside it.
$(BUILD)/satellite-004: FORCE | $(BUILD)/satl
	@[ -L $@ ] && [ "$$(readlink $@)" = satl ] || { ln -sfn satl $@ && echo "$@ -> satl (the binary's name since M0.5)"; }

# NO `libraries` TARGET SINCE 2026-10-07: the words are sources of satl's own (040-sources.mk's
# WORD_SOURCES), compiled by 060-compile.mk's rule like every other and linked above. The script
# that built a .so a word, satellite-numbers/build_libraries.py, went with them.

FORCE:

.PHONY: all FORCE
