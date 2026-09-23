# satellite 004 -- installing what `make` just built, every time it builds.
#
# THE AUTHOR, 2026-09-22 (D0.5.1): "yes let's install 004 to ~/.satl with every
# make". A bare `make`, or `make all`, builds and then runs
# satellite_enterprise/install.sh, which puts satl and satellite-numbers/ into
# $HOME/.satl -- where ~/.local/bin/satl points, so the word satl is always the
# last build. 003 did the same from 2026-09-13 (its 080-install.mk); this is that
# rule, ported.
#
# THE LAST FRAGMENT, because it names $(ALL_TARGETS) in a prerequisite list, which
# make expands as it reads -- so 050-build.mk must have set it already.
#
# ONLY FOR A BARE make OR make all. `check` and `test` depend on `all` here, so the
# rule is tied to the goals asked for and not to `all` being built: `make test`
# under CXX=g++ must not put a g++ interpreter on PATH as a side effect of
# checking it. NEVER AFTER A FAILED BUILD: the install depends on every target in
# `all`, and make does not reach a target whose prerequisite failed.
#
# NEVER FROM A LINKED WORKTREE. Builders work in .claude/worktrees/ on changes
# nobody has reviewed yet, and a make there must not become the author's satl.
# Checked in the recipe (git rev-parse), so an ordinary make reads no git state.
#
# NO RECURSION: the recipe tells the installer make has just built
# (SATELLITE_JUST_BUILT), and the installer's own make, when it is run by hand,
# carries INSTALL_AFTER_BUILD=no. MAKELEVEL covers a make inside a recipe.
#
# NOT AS ROOT: `sudo make` would install into root's home, which is not the
# install anybody meant.
#
# `make INSTALL_AFTER_BUILD=no` builds without installing.
INSTALL_AFTER_BUILD ?= yes

# AND NEVER ONE PROCESSOR'S BUILD (010-compiler.mk's CPU): which of those this machine
# gets is satl-cpu-level's to say (055-cpus.mk), not whichever was built last.
ifeq ($(INSTALL_AFTER_BUILD)$(MAKELEVEL)$(CPU),yes0)
ifeq ($(filter-out all,$(MAKECMDGOALS)),)
all: install-after-build
endif
endif

.PHONY: install-after-build
install-after-build: $(ALL_TARGETS)
	@if [ "$$(id -u)" = 0 ]; then \
	    echo "note: not installing as root. Run make as yourself."; \
	 elif [ "$$(git rev-parse --git-dir 2>/dev/null)" != "$$(git rev-parse --git-common-dir 2>/dev/null)" ]; then \
	    echo "note: a linked worktree does not install; only the main checkout's make does."; \
	 else \
	    SATELLITE_JUST_BUILT=yes sh satellite_enterprise/install.sh; \
	 fi
