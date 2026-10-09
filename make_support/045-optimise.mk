# satellite 004 -- PGO, ThinLTO and BOLT: satl laid out by what a run of it does.
#
# THE AUTHOR, 2026-09-23: "let's build LTO, PGO and BOLT into the regular make". What
# each was worth was measured first (CPU-PLAN.md, "Measured" and CPU-7): PGO took
# 10.5-13.0% off his race and 7.7-9.6% off everything.satl, every PGO run beating every
# other run; ThinLTO and BOLT on top could not be told apart from PGO alone, beyond
# about 2-3% on the race. -march=haswell and -O3 were noise.
#
# THREE STEPS, ALL IN A PLAIN `make`:
#
#   1. PGO. satl is built a second time, instrumented, into build/pgo-train/ -- a make
#      inside this one, PGO_STAGE=pgo-train -- and runs make_support/training/*.satl;
#      llvm-profdata merges what it counted into build/pgo/satl.profdata. ONCE, AND KEPT
#      (2026-10-07): the profile is made when it is missing or a training program changed,
#      and again by `make train`; a plain make compiles a changed file against the profile
#      there is, and leaves the rest standing.
#   2. satl's own objects are compiled with that profile and -flto=thin, and lld links
#      them.
#   3. BOLT. The linked satl is instrumented, runs the training again, and llvm-bolt
#      rewrites it with the code that ran laid out together (build/bolt/).
#
# EVERY OBJECT IS satl'S OWN SINCE 2026-10-07, the words included (satellite-numbers/words/);
# the word libraries of before were 0.00% of both races' instructions and were built plain,
# beside the training satl, which loaded them through a link.
#
# THE TRAINING IS NEVER A RACE PROGRAM (haswell_test/, time_test/): a build
# trained on the program that times it would be timing its own training.
#
# `make OPTIMISE=no` IS TODAY'S PLAIN -O2. Until 2026-10-07 it was the quick way to work,
# because with the three on any source change re-trained and then recompiled every object;
# now the profile is kept (step 1), and a plain make compiles what changed and links. SWITCHING
# still costs a whole build each way and a build number each time (the two share build/objects,
# and the fingerprint says which it is).
#
# WHAT THE TRAINING NEVER RUNS IS LAID OUT AS COLD -- floats, fractions, files, the
# window, the prompt. The four programs are what satl's time was measured going to; a
# program that lives in something else is the reason to add a fifth.
#
# EACH STEP TURNS ITSELF OFF WHERE ITS TOOL IS NOT BESIDE THE COMPILER, and the link
# line says which: g++ (the PGO here is clang's), no llvm-profdata (AlmaLinux: the llvm
# package), no ld.lld (the lld package), no llvm-bolt (AlmaLinux ships none). A fresh
# clone with only g++ builds exactly as it did before this file.
#
# BESIDE THE COMPILER, NEVER FROM PATH: a profile written by one LLVM is not read by
# another, and this machine has clang 24 in ~/opt and clang 21 in /usr/bin. "Beside" is
# the folder clang itself names (`clang++ -v`, InstalledDir), which is right through a
# wrapper (CXX="ccache clang++") too; `-print-prog-name` is not, because it falls back to
# PATH and offered clang 21 the clang 24 in ~/opt (the review, 2026-09-23).
#
# NEVER FOR ONE PROCESSOR'S BUILD (CPU=, 055-cpus.mk): a machine cannot train a build it
# may not be able to run.

OPTIMISE ?= yes

# PGO_STAGE FROM THE COMMAND LINE ONLY, as CPU is (010-compiler.mk): exported in a shell,
# it would turn every make into a training build that installs nothing.
ifeq ($(origin PGO_STAGE),environment)
override PGO_STAGE :=
endif
PGO_STAGE ?=

PGO_DIR      = build/pgo
TRAIN_BUILD  = build/pgo-train
BOLT_DIR     = build/bolt
TRAINING     = $(sort $(wildcard make_support/training/*.satl))

# EVERY ONE OF THESE IS SET HERE, so a PGO, LTO or BOLT exported in a shell never turns
# a step on in a build meant to be plain (the review, 2026-09-23).
PROFDATA :=
LLD :=
LLVM_BOLT :=
MERGE_FDATA :=
HAS_PGO :=
HAS_LTO :=
HAS_BOLT :=
OPTIMISE_SKIPPED :=
ifeq ($(OPTIMISE),yes)
ifneq ($(findstring clang version,$(CXX_VERSION)),)
  OPTIMISE_TOOLS := $(shell $(CXX) -v 2>&1 | sed -n 's/^InstalledDir: //p')
  PROFDATA       := $(if $(OPTIMISE_TOOLS),$(wildcard $(OPTIMISE_TOOLS)/llvm-profdata))
  LLD            := $(if $(OPTIMISE_TOOLS),$(wildcard $(OPTIMISE_TOOLS)/ld.lld))
  LLVM_BOLT      := $(if $(OPTIMISE_TOOLS),$(wildcard $(OPTIMISE_TOOLS)/llvm-bolt))
  MERGE_FDATA    := $(if $(OPTIMISE_TOOLS),$(wildcard $(OPTIMISE_TOOLS)/merge-fdata))
  HAS_PGO        := $(if $(PROFDATA),yes)
  HAS_LTO        := $(if $(LLD),yes)
  HAS_BOLT       := $(if $(and $(LLVM_BOLT),$(MERGE_FDATA)),yes)
  OPTIMISE_SKIPPED := $(strip $(if $(HAS_PGO),,no llvm-profdata beside $(CXX): no PGO) \
                              $(if $(HAS_LTO),,no ld.lld beside $(CXX): no ThinLTO) \
                              $(if $(HAS_BOLT),,no llvm-bolt beside $(CXX): no BOLT))
else
  OPTIMISE_SKIPPED := PGO, ThinLTO and BOLT are clang's and this is not clang
endif
endif

# WHAT THE ORDINARY BUILD IS -- in the build fingerprint (020-version.mk) and on the link
# line -- AND THE SAME WORD IN EVERY BUILD MADE FROM IT. One processor's build (CPU=) and
# the training build are BUILD N (build_number.py --same), so each must describe itself
# as the ordinary BUILD N was described, or --same refuses: before this, `make CPU=haswell`
# after any plain make said "plain" against a stamp saying "PGO ThinLTO BOLT" and was
# refused (the review, 2026-09-23).
override OPTIMISE_KIND := $(strip $(if $(HAS_PGO),PGO) $(if $(HAS_LTO),ThinLTO) $(if $(HAS_BOLT),BOLT))

# WHICH STEPS THIS BUILD TAKES: all that the tools allow, in the ordinary build only --
# and what the link line says, which is THIS build's steps, not the fingerprint's word.
ifeq ($(CPU)$(PGO_STAGE),)
PGO  := $(HAS_PGO)
LTO  := $(HAS_LTO)
BOLT := $(HAS_BOLT)
OPTIMISE_SAID = $(or $(OPTIMISE_KIND),plain)$(if $(OPTIMISE_SKIPPED), ($(OPTIMISE_SKIPPED)))
else
PGO  :=
LTO  :=
BOLT :=
OPTIMISE_SAID = $(if $(PGO_STAGE),instrumented to train PGO,plain -- one processor's build is never optimised)
endif

ifneq ($(PGO_STAGE),)
# THE TRAINING BUILD: counters in every function, and no LTO -- clang's counters are
# matched by name and shape, not by how the objects were linked. `satl --version`, which
# 050's link check runs, writes a profile too; it goes where nothing reads it.
OPTIMISE_FLAGS      = -fprofile-instr-generate
OPTIMISE_LINK_FLAGS = -fprofile-instr-generate
export LLVM_PROFILE_FILE = $(CURDIR)/$(PGO_DIR)/discard/%p.profraw
else
PGO_PROFILE         = $(if $(PGO),$(PGO_DIR)/satl.profdata)
OPTIMISE_FLAGS      = $(if $(PGO),-fprofile-instr-use=$(CURDIR)/$(PGO_PROFILE) -Wno-profile-instr-unprofiled \
                          -Wno-profile-instr-out-of-date) $(if $(LTO),-flto=thin)
# NAMED, because a comma inside $(if ...) ends its argument: `$(if $(BOLT),-Wl,--emit-relocs)`
# passed a bare -Wl, and BOLT refused a satl with no relocations (2026-09-23).
LTO_LINK_FLAGS      = -flto=thin -fuse-ld=lld -Wl,--thinlto-cache-dir=$(BUILD)/lto-cache
BOLT_LINK_FLAGS     = -Wl,--emit-relocs
OPTIMISE_LINK_FLAGS = $(if $(LTO),$(LTO_LINK_FLAGS)) $(if $(BOLT),$(BOLT_LINK_FLAGS))
endif

# RUNS EVERY TRAINING PROGRAM: $(1) the satl, $(2) what goes in its environment, $(3) the
# folder it runs in. Copies, so nothing a run leaves lands beside the training; its own
# HOME, so no ~/.satl is read or written; no window, and no terminal to wait on. A
# subshell, so a failure's `exit` ends the training and not the caller's `|| ...`.
define run_training
( rm -rf $(3) && mkdir -p $(3)/home && cp $(TRAINING) $(3)/ && \
  for program in $(3)/*.satl; do \
      env -u DISPLAY -u WAYLAND_DISPLAY HOME=$(CURDIR)/$(3)/home SATL_NO_WINDOW=1 $(2) $(1) $$program \
          </dev/null > $$program.out 2>&1 || \
      { status=$$?; echo "training: $$program stopped with $$status -- $$program.out ends:" >&2; \
        tail -n 5 $$program.out >&2; exit 1; }; \
  done )
endef

ifneq ($(PGO),)
# THE PROFILE IS MADE ONCE, AND KEPT (the author, 2026-10-07: "if there's a file change
# somewhere ... build that object that changed, but nothing else"). Until this day the training
# satl was a prerequisite asked every make: any source change relinked it, re-ran the training,
# wrote a new profile, and so recompiled every object -- the whole build for one line. Now the
# profile depends on the training programs alone and is made when it is missing or one of them
# changed; the training satl is built INSIDE this recipe, by a make of its own (PGO_STAGE=pgo-train),
# and is asked for by nothing else. A changed object is compiled against the profile there is
# (-Wno-profile-instr-out-of-date: a function whose shape changed is unprofiled until the next
# training), and the rest stand. ORDER-ONLY ON THE BUILD STAMP, so the inner make's --same reads
# the stamp this make has already written, with -j as without.
$(PGO_PROFILE): $(TRAINING) | $(BUILD_STAMP)
	@mkdir -p $(TRAIN_BUILD)
	@$(MAKE) --no-print-directory PGO_STAGE=pgo-train INSTALL_AFTER_BUILD=no $(TRAIN_BUILD)/satl
	@echo "PGO: $(TRAIN_BUILD)/satl runs $(words $(TRAINING)) training programs (make_support/training/)"
	@rm -rf $(PGO_DIR)/raw && mkdir -p $(PGO_DIR)/raw
	@$(call run_training,$(TRAIN_BUILD)/satl,LLVM_PROFILE_FILE=$(CURDIR)/$(PGO_DIR)/raw/%p.profraw,$(PGO_DIR)/run)
	@$(PROFDATA) merge -o $@.tmp $(PGO_DIR)/raw/*.profraw && mv $@.tmp $@ && rm -rf $(PGO_DIR)/raw $(PGO_DIR)/discard

# `make train`: the profile again, from the sources as they are now -- and then every object
# is compiled against it by the next make, since each depends on the profile.
.PHONY: train
train:
	@rm -f $(PGO_PROFILE)
	@$(MAKE) --no-print-directory $(PGO_PROFILE)
	@echo "PGO: the profile is new; the next make compiles every object against it"
endif

ifneq ($(BOLT),)
# $(1) is the satl just linked, PGO and ThinLTO already in it. BOLT WRITES A NEW FILE and
# moves it over $(1) only when every step worked, so A FAILED BOLT KEEPS THE satl AS LINKED
# and says why, and the make goes on: an llvm-bolt too old for one of these flags, or one
# that meets a binary it cannot read, must not stop a plain `make` for good (the review,
# 2026-09-23 -- "pull, make, it compiles" is the author's rule). 050's link check then
# runs whichever satl is there. The logs stay in build/bolt/.
define bolt_it
@echo "BOLT: $(1) is instrumented, runs the training, and is laid out by what ran"
@rm -rf $(BOLT_DIR) && mkdir -p $(BOLT_DIR) && \
 { $(LLVM_BOLT) $(1) -instrument -instrumentation-file=$(CURDIR)/$(BOLT_DIR)/prof -instrumentation-file-append-pid \
       -o $(BOLT_DIR)/satl > $(BOLT_DIR)/instrument.log 2>&1 || { echo "instrument.log" > $(BOLT_DIR)/failed; false; }; } && \
 { $(call run_training,$(BOLT_DIR)/satl,,$(BOLT_DIR)/run) || { echo "run/ (a training program)" > $(BOLT_DIR)/failed; false; }; } && \
 { $(MERGE_FDATA) $(BOLT_DIR)/prof.*.fdata > $(BOLT_DIR)/merged.fdata 2> $(BOLT_DIR)/merge.log || { echo "merge.log" > $(BOLT_DIR)/failed; false; }; } && \
 { $(LLVM_BOLT) $(1) -o $(1).bolted -data=$(BOLT_DIR)/merged.fdata -reorder-blocks=ext-tsp \
       -reorder-functions=cdsort -split-functions -split-all-cold -update-debug-sections \
       > $(BOLT_DIR)/optimise.log 2>&1 || { echo "optimise.log" > $(BOLT_DIR)/failed; false; }; } && \
 mv $(1).bolted $(1) || \
 { rm -f $(1).bolted; echo "note: BOLT failed ($(BOLT_DIR)/$$(cat $(BOLT_DIR)/failed 2>/dev/null)) -- $(1) is kept as linked, with PGO and ThinLTO" >&2; }
endef
endif
