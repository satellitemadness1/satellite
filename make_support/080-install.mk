# satellite -- installing what `make` just built, every time it builds.
#
# SINCE 2026-09-13, at the author's instruction: a bare `make`, or `make all`,
# builds and then runs satellite_enterprise/install.sh, which copies satl,
# satl-cpu-level and satl-term into $HOME/.satl and links ~/.local/bin/satl and
# satl-term to them. The installed copy used to go stale every milestone until
# somebody remembered to reinstall; now it is never older than the last build.
#
# THE LAST FRAGMENT, because it names $(ALL_TARGETS) in a prerequisite list,
# which make expands as it reads -- so 050-build.mk must have set it already.
#
# ONLY FOR `all`. `make satl`, `make test`, `make startup` and `make clean` do
# what they name and install nothing: a suite run under CXX=g++ must not put a
# g++ interpreter on PATH as a side effect of checking it. And NEVER AFTER A
# FAILED BUILD, because the install depends on every binary in `all` and make
# does not reach a target whose prerequisite failed.
#
# DEPENDS ON $(ALL_TARGETS) and is not merely listed beside them under `all`.
# Prerequisites of one target run side by side under 005-jobs.mk's -j24, so a
# sibling would start copying binaries that were still being linked.
#
# THE INSTALLER RUNS make ITSELF -- install_support/050-building.sh, before it
# copies anything -- so without the MAKELEVEL test this rule recurses forever.
# That make is started from inside this recipe and reads MAKELEVEL 1; it builds
# `all`, finds it up to date, and stops. MAKELEVEL rather than a variable of our
# own, because a command-line `INSTALL_AFTER_BUILD=yes` is handed to every
# sub-make in MAKEFLAGS and would switch such a guard straight back on.
#
# NOT AS ROOT. `sudo make` would run the HOME installer as root, which is not
# the install anybody meant by it; the system install is its own command, and it
# builds as the invoking user for the reasons 050-building.sh gives. Checked when
# the recipe runs rather than when this file is read, so that an ordinary make
# does not spend a fork asking.
#
# `make INSTALL_AFTER_BUILD=no` builds without installing.
INSTALL_AFTER_BUILD ?= yes

ifeq ($(INSTALL_AFTER_BUILD)$(MAKELEVEL),yes0)
all: install-after-build
endif

.PHONY: install-after-build
install-after-build: $(ALL_TARGETS)
	@if [ "$$(id -u)" = 0 ]; then \
	    echo "note: not installing as root. Run make as yourself, or for /usr/local:"; \
	    echo "      sudo sh satellite_enterprise/install.sh --system"; \
	 else \
	    sh satellite_enterprise/install.sh; \
	 fi
