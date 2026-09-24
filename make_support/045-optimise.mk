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
#      llvm-profdata merges what it counted into build/pgo/satl.profdata.
#   2. satl's own objects are compiled with that profile and -flto=thin, and lld links
#      them.
#   3. BOLT. The linked satl is instrumented, runs the training again, and llvm-bolt
#      rewrites it with the code that ran laid out together (build/bolt/).
#
# ONLY satl's OWN OBJECTS. The 62 word libraries were 0.00% of both races'
# instructions, so they stay exactly as they were -- and the training satl loads the
# real build's libraries through a link beside it.
#
# THE TRAINING IS NEVER A RACE PROGRAM (haswell_test/, experiments/cpu_race/): a build
# trained on the program that times it would be timing its own training.
#
# `make OPTIMISE=no` IS TODAY'S PLAIN -O2, for quick rebuilds while working: with the
# three on, any source change re-trains, and then every object compiles again, because
# the profile they were compiled with changed.
#
# EACH STEP TURNS ITSELF OFF WHERE ITS TOOL IS NOT BESIDE THE COMPILER, and the link
# line says which: g++ (the PGO here is clang's), no llvm-profdata (AlmaLinux: the llvm
# package), no ld.lld (the lld package), no llvm-bolt (AlmaLinux ships none). A fresh
# clone with only g++ builds exactly as it did before this file.
#
# BESIDE THE COMPILER, NEVER FROM PATH: a profile written by one LLVM is not read by
# another, and this machine has clang 24 in ~/opt and clang 21 in /usr/bin.
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

ifeq ($(OPTIMISE)$(CPU)$(PGO_STAGE),yes)
ifneq ($(findstring clang version,$(CXX_VERSION)),)
  OPTIMISE_TOOLS := $(dir $(realpath $(shell command -v $(CXX) 2>/dev/null)))
  PROFDATA       := $(if $(OPTIMISE_TOOLS),$(wildcard $(OPTIMISE_TOOLS)llvm-profdata))
  LLD            := $(if $(OPTIMISE_TOOLS),$(wildcard $(OPTIMISE_TOOLS)ld.lld))
  LLVM_BOLT      := $(if $(OPTIMISE_TOOLS),$(wildcard $(OPTIMISE_TOOLS)llvm-bolt))
  MERGE_FDATA    := $(if $(OPTIMISE_TOOLS),$(wildcard $(OPTIMISE_TOOLS)merge-fdata))
endif
PGO  = $(if $(PROFDATA),yes)
LTO  = $(if $(LLD),yes)
BOLT = $(if $(and $(LLVM_BOLT),$(MERGE_FDATA)),yes)
OPTIMISE_SKIPPED = $(strip $(if $(findstring clang version,$(CXX_VERSION)),,PGO, ThinLTO and BOLT are clang's and this is not clang) \
                           $(if $(findstring clang version,$(CXX_VERSION)),$(if $(PGO),,no llvm-profdata beside $(CXX): no PGO) \
                                $(if $(LTO),,no ld.lld beside $(CXX): no ThinLTO) \
                                $(if $(BOLT),,no llvm-bolt beside $(CXX): no BOLT)))
endif

# WHAT THIS BUILD IS, in the build fingerprint (020-version.mk) and on the link line. The
# training build is handed its parent's word, so --same finds the description it expects.
OPTIMISE_KIND ?= $(strip $(if $(PGO),PGO) $(if $(LTO),ThinLTO) $(if $(BOLT),BOLT))

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
# folder it runs in. Copies, because satl writes a .sate beside each program; its own
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
# THE TRAINING satl, BY A MAKE INSIDE THIS ONE. Asked every time, like the stamps: the
# inner make decides whether anything changed, and the profile below is made again only
# when the training satl was relinked or a training program changed. It loads the real
# build's libraries, which are built plain in both.
$(TRAIN_BUILD)/satl: FORCE | $(BUILD_STAMP) libraries
	@mkdir -p $(TRAIN_BUILD) && ln -sfn ../satellite-numbers $(TRAIN_BUILD)/satellite-numbers
	@$(MAKE) --no-print-directory PGO_STAGE=pgo-train OPTIMISE_KIND='$(OPTIMISE_KIND)' INSTALL_AFTER_BUILD=no $@

$(PGO_PROFILE): $(TRAIN_BUILD)/satl $(TRAINING)
	@echo "PGO: $(TRAIN_BUILD)/satl runs $(words $(TRAINING)) training programs (make_support/training/)"
	@rm -rf $(PGO_DIR)/raw && mkdir -p $(PGO_DIR)/raw
	@$(call run_training,$(TRAIN_BUILD)/satl,LLVM_PROFILE_FILE=$(CURDIR)/$(PGO_DIR)/raw/%p.profraw,$(PGO_DIR)/run)
	@$(PROFDATA) merge -o $@.tmp $(PGO_DIR)/raw/*.profraw && mv $@.tmp $@ && rm -rf $(PGO_DIR)/raw $(PGO_DIR)/discard
endif

ifneq ($(BOLT),)
# BOLT RUNS satl, SO THE LIBRARIES IT LOADS COME FIRST.
$(BUILD)/satl: | libraries

# $(1) is the satl just linked. On any failure it is deleted, so the next make links it
# again rather than installing a half-made one; the logs stay in build/bolt/.
define bolt_it
@echo "BOLT: $(1) is instrumented, runs the training, and is laid out by what ran"
@rm -rf $(BOLT_DIR) && mkdir -p $(BOLT_DIR) && ln -sfn ../satellite-numbers $(BOLT_DIR)/satellite-numbers
@$(LLVM_BOLT) $(1) -instrument -instrumentation-file=$(CURDIR)/$(BOLT_DIR)/prof -instrumentation-file-append-pid \
     -o $(BOLT_DIR)/satl > $(BOLT_DIR)/instrument.log 2>&1 || \
 { echo "BOLT could not instrument $(1) -- $(BOLT_DIR)/instrument.log" >&2; rm -f $(1); exit 1; }
@$(call run_training,$(BOLT_DIR)/satl,,$(BOLT_DIR)/run) || { rm -f $(1); exit 1; }
@$(MERGE_FDATA) $(BOLT_DIR)/prof.*.fdata > $(BOLT_DIR)/merged.fdata 2> $(BOLT_DIR)/merge.log || \
 { echo "BOLT could not merge its profiles -- $(BOLT_DIR)/merge.log" >&2; rm -f $(1); exit 1; }
@$(LLVM_BOLT) $(1) -o $(1).bolted -data=$(BOLT_DIR)/merged.fdata -reorder-blocks=ext-tsp \
     -reorder-functions=cdsort -split-functions -split-all-cold -update-debug-sections \
     > $(BOLT_DIR)/optimise.log 2>&1 && mv $(1).bolted $(1) || \
 { echo "BOLT could not rewrite $(1) -- $(BOLT_DIR)/optimise.log" >&2; rm -f $(1) $(1).bolted; exit 1; }
endef
endif
