# satellite 004 -- satl for every processor clang can build it for, and the program
# that says which of them a machine runs.
#
# THE AUTHOR, 2026-09-23: "let's build more satl.cpu things for cpu's that I don't
# currently have ... we just can't test the latest stuff, but we can still compile for
# it", and then "if there are 137 targets, we could technically just build all targets,
# and alter satl-cpu-level to check for them". 003 built two (baseline and haswell) and
# its satl-cpu-level chose between them; 004 built one until now (the Makefile's note).
#
#     make cpus            the ordinary build, then every processor below, then
#                          build/satl-cpu-level -- about half an hour at -j24
#     make CPU=haswell     one processor: build/cpu/haswell/satl and its libraries
#     build/satl-cpu-level [--explain]    which of build/cpu/ this machine runs best
#
# WHICH PROCESSORS, AND WHY 53. clang 24 names 136 (--print-supported-cpus). 57 of them
# are not a 64-bit target at all -- the 32-bit chips (i486, pentium, athlon, k6 ...) and
# the underscore spellings, which are __builtin_cpu_is's names and not -march's. Of the
# 79 left, the instruction sets the compiler is then allowed (its -dM macros) come to 54
# distinct ones: the rest are another name for one of them. One of the 54 is plain
# x86-64, which is the ordinary build. So 53, one per set, named by the level where
# there is one (x86-64-v2), by the author's own processors (haswell, raptorlake), and
# otherwise by the chip and not its alias. Measured 2026-09-23 with clang 24. The names
# folded in, each giving the same build as the one before its arrow:
#
#     x86-64-v2 <- corei7 nehalem         haswell <- core-avx2      raptorlake <- alderlake
#     gracemont meteorlake                sandybridge <- corei7-avx ivybridge <- core-avx-i
#     skylake-avx512 <- skx               sapphirerapids <- emeraldrapids
#     arrowlake-s <- lunarlake            pantherlake <- wildcatlake
#     sierraforest <- grandridge          silvermont <- slm         bonnell <- atom
#     k8 <- athlon64 athlon-fx opteron    k8-sse3 <- athlon64-sse3 opteron-sse3
#     amdfam10 <- barcelona btver1        znver1 <- c86-4g-m4 c86-4g-m6
#     c86-4g-m7 <- c86-4g-m8
#
# A COMPILER THAT DOES NOT KNOW ONE SKIPS IT, and says so: g++ 17 and clang 24 do not
# name the same processors, and a make that failed on the first unknown name would build
# none of the rest.
#
# EACH BUILD IS SATL AND ITS LIBRARIES, IN build/cpu/<processor>/ (030-directories.mk),
# beside `needs`: what -march=<processor> let the compiler use, as the macros it defined
# over the baseline's. satl-cpu-level chooses by that list and never by a name -- its own
# header says why. A build this machine cannot run is kept (050-build.mk), because
# building for processors nobody here owns is the point.
#
# NONE OF THEM IS INSTALLED: `make` installs the ordinary build, as it always has.
CPUS = amdfam10 arrowlake arrowlake-s bdver1 bdver2 bdver3 bdver4 bonnell broadwell btver2 \
       c86-4g-m7 cannonlake cascadelake clearwaterforest cooperlake core2 diamondrapids \
       goldmont goldmont-plus graniterapids graniterapids-d haswell icelake-client \
       icelake-server ivybridge k8 k8-sse3 knl knm nocona novalake pantherlake penryn \
       raptorlake rocketlake sandybridge sapphirerapids sierraforest silvermont skylake \
       skylake-avx512 tigerlake tremont westmere x86-64-v2 x86-64-v3 x86-64-v4 znver1 \
       znver2 znver3 znver4 znver5 znver6

# THE CHOOSER, AT ONE PLACE WHATEVER CPU SAYS: the ordinary build's folder. A processor's
# build asks it (050-build.mk) and never makes a second one inside build/cpu/.
CPU_LEVEL = build/satl-cpu-level

.PHONY: cpus
cpus: all $(CPU_LEVEL)
	@for cpu in $(CPUS); do \
	    if ! echo 'int x;' | $(CXX) -march=$$cpu -x c++ -c - -o /dev/null 2>/dev/null; then \
	        echo "cpus: $(CXX) does not know -march=$$cpu -- skipped"; continue; fi; \
	    echo "cpus: $$cpu"; \
	    $(MAKE) --no-print-directory CPU=$$cpu INSTALL_AFTER_BUILD=no || exit 1; \
	done
	@$(CPU_LEVEL) --explain | tail -n 1

# WHAT A PROCESSOR'S BUILD NEEDS: every macro -march=$(CPU) defines that the baseline does
# not, one a line -- the feature macros (__AVX2__), and CMPXCHG16B's, which is shaped
# differently (__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16) and was missed until the review.
# EVERY STEP'S STATUS IS KEPT, not the last pipe's: a compiler that failed once wrote an
# empty list, and satl-cpu-level ran that build anywhere (the review, 2026-09-23). An empty
# list is refused here too -- no processor's build needs nothing. LC_ALL=C, because comm
# compares in the order sort made. $(call write_needs,<file>).
NEEDS_MACROS = s/^\#define \(__[A-Z0-9_]*__\) 1$$/\1/p; s/^\#define \(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16\) 1$$/\1/p
define write_needs
$(CXX) -dM -E -x c++ /dev/null > $(1).base && $(CXX) -march=$(CPU) -dM -E -x c++ /dev/null > $(1).cpu && \
 sed -n '$(NEEDS_MACROS)' $(1).base | LC_ALL=C sort > $(1).b && sed -n '$(NEEDS_MACROS)' $(1).cpu | LC_ALL=C sort > $(1).c && \
 LC_ALL=C comm -23 $(1).c $(1).b > $(1).tmp && [ -s $(1).tmp ] && mv $(1).tmp $(1); \
 written=$$?; rm -f $(1).base $(1).cpu $(1).b $(1).c $(1).tmp; \
 [ $$written = 0 ] || { echo "$(1): the compiler could not say what -march=$(CPU) needs" >&2; exit 1; }
endef

# WRITTEN LAST, after satl, its libraries and its help link, and taken away when a
# processor's make begins (050-build.mk's stamp rule): satl-cpu-level offers only a folder
# with a list, so a build that failed or was stopped half way is never offered.
$(BUILD)/needs: $(BUILD)/satl libraries $(BUILD)/satellite.help
	@mkdir -p $(BUILD)
	@$(call write_needs,$@)

# THE HELP FILES BESIDE THIS satl: satellite.help() reads them beside satl or one folder up
# (satellite/satl/prompt_help.cpp). build/satl finds the tree's one folder up; a
# processor's satl is two folders further down, so it is given a link to them.
$(BUILD)/satellite.help:
	@mkdir -p $(BUILD)
	@ln -sfn ../../../satellite.help $@

# THE CHOOSER IS COMPILED AT THE BASELINE, WHATEVER CPU SAYS: it runs before anything is
# known about the machine (satellite/cpu_level/cpu_level.cpp). Relinked when the compiler
# or the flags change, which the link stamp says; the ordinary folder's own stamp, since
# it is the ordinary folder's program. A processor's make, which has no rule for that
# stamp, relinks it only when its source changes.
$(CPU_LEVEL): $(SATELLITE)/cpu_level/cpu_level.cpp $(if $(CPU),,$(LINK_STAMP))
	@mkdir -p $(dir $@)
	$(LINK_ENV) $(CXX) -std=c++20 $(OPT) -Wall -Wextra $(LDFLAGS) $< -o $@
