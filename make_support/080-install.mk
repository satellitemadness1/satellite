# satellite 004 -- `make install`: what make built, into ~/.satl.
#
# THE AUTHOR, 2026-09-22 (D0.5.1): "yes let's install 004 to ~/.satl with every
# make" -- and on 2026-10-07: "plain make should build the program" and "I want to
# run make and make install". A bare make that found nothing to compile had re-run
# this installer and shown the same number, and read as make doing something else.
# SO SINCE 2026-10-07 A BARE `make` BUILDS -- a new number every time, 050-build.mk --
# AND THEN ASKS WHETHER TO INSTALL (the same evening: "we need plain make to run make
# install by default, and plain make to ask if you want to install satellite or just
# build it"): Enter or silence installs, n does not. `make install` is the install by
# name, asking nothing: it builds what changed, then runs satellite_enterprise/install.sh,
# which puts satl (every word inside it) into $HOME/.satl -- where ~/.local/bin/satl
# points, so the word satl is the last install. 003 did the same from 2026-09-13 (its
# 080-install.mk); this is that rule, ported. Both run run_the_installer below.
#
# THE LAST FRAGMENT, because it names $(ALL_TARGETS) in a prerequisite list, which
# make expands as it reads -- so 050-build.mk must have set it already.
#
# `make INSTALL_AFTER_BUILD=yes` IS THE BARE make AS IT WAS until 2026-10-07: build
# what changed, then install, in one. ONLY FOR A BARE make OR make all. `check` and
# `test` depend on `all` here, so the rule is tied to the goals asked for and not to
# `all` being built: `make test` under CXX=g++ must not put a g++ interpreter on
# PATH as a side effect of checking it. NEVER AFTER A FAILED BUILD: the install
# depends on every target in `all`, and make does not reach a target whose
# prerequisite failed.
#
# NEVER FROM A LINKED WORKTREE. Builders work in .claude/worktrees/ on changes
# nobody has reviewed yet, and a make there must not become the author's satl.
# Checked in the recipe (git rev-parse), so an ordinary make reads no git state.
#
# NO RECURSION: the installer's own make carries INSTALL_AFTER_BUILD=no, and
# MAKELEVEL covers a make inside a recipe -- so the installer's make, run from
# this recipe, finds everything built and installs. (The SATELLITE_JUST_BUILT
# shortcut that skipped that make went on 2026-10-07, on the author's word.)
#
# NOT AS ROOT: `sudo make` would install into root's home, which is not the
# install anybody meant.
#
# `make INSTALL_AFTER_BUILD=no` is what the installer's own make says (050-building.sh), and
# what a bare make does anyway since 2026-10-07.
INSTALL_AFTER_BUILD ?= no

# AND NEVER ONE PROCESSOR'S BUILD (010-compiler.mk's CPU): which of those this machine
# gets is satl-cpu-level's to say (055-cpus.mk), not whichever was built last.
ifeq ($(INSTALL_AFTER_BUILD)$(MAKELEVEL)$(CPU),yes0)
ifeq ($(filter-out all,$(MAKECMDGOALS)),)
all: install-after-build
endif
endif

# `make install` IS THE SAME INSTALL, ASKED FOR BY NAME (the author, 2026-10-07: "caught a
# really bad error, "make install" doesn't do anything at all"). It was never a target -- only a
# bare make installed -- so make answered "No rule to make target 'install'". It builds first,
# as the bare make does, and installs whatever INSTALL_AFTER_BUILD says.
.PHONY: install
install: install-after-build

.PHONY: install-after-build
install-after-build: $(ALL_TARGETS)
	@$(call run_the_installer)

# THE INSTALL, AS ONE RECIPE BODY: `make install` runs it above, and so does the bare make's
# question (050-build.mk) when the answer is yes. A `define` is expanded where it is called, when
# the recipe runs, so 050 may name it although this fragment is read last.
define run_the_installer
if [ "$$(id -u)" = 0 ]; then \
    echo "note: not installing as root. Run make as yourself."; \
elif [ "$$(git rev-parse --git-dir 2>/dev/null)" != "$$(git rev-parse --git-common-dir 2>/dev/null)" ]; then \
    echo "note: a linked worktree does not install; only the main checkout's make does."; \
else \
    sh satellite_enterprise/install.sh; \
fi
endef
