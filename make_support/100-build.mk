# satellite -- the default goal, the two binaries, and the download bundle.
#
# THE FIRST FRAGMENT THAT DECLARES A TARGET, which is what makes `all` the
# first target make sees. `.DEFAULT_GOAL := all` below says it outright anyway,
# and after the 2026-08-24 split that line carries more weight than it used to:
# the order of the rules is now the order of the includes.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# `all`, whatever order the rules below end up in.
.DEFAULT_GOAL := all

all: satl $(GUI_TARGET) $(if $(AUTOINSTALL),bundle)
ifeq ($(AUTOINSTALL),yes)
	@dir='$(prefix)'; \
	while [ ! -d "$$dir" ] && [ "$$dir" != / ]; do dir=`dirname "$$dir"`; done; \
	if [ ! -w "$$dir" ]; then \
	    printf 'satl built, and NOT installed: %s is not writable by %s.\n' \
	        "$$dir" "`id -un 2>/dev/null || echo you`"; \
	    printf 'A build does not run sudo. To install it as root:\n'; \
	    printf '    sudo ./install.sh --prefix %s\n' '$(prefix)'; \
	elif ! $(MAKE) --no-print-directory $(INSTALL_QUIET) \
	          -o satl -o satl-term GUI_TARGET= \
	          install install-report SATELLITE_AUTOINSTALL=0; then \
	    printf 'satl built, but the install did not finish.\n'; \
	    printf 'Run `make install` to see what stopped it.\n'; \
	fi
endif

# BELOW `all`, and that position is load-bearing: the first target in a
# makefile is the default goal, so defining this one above `all` quietly made
# `make` build the bundle and nothing else -- binaries fresh, install skipped,
# exit 0, no error anywhere. .DEFAULT_GOAL is set at the top as well, so the
# next rule that lands in the wrong place cannot repeat it.
bundle: satl $(GUI_TARGET)
	@rm -rf '$(DOWNLOAD_DIR)'
	@install -d -m755 '$(DOWNLOAD_DIR)'
	@install -m755 satl '$(DOWNLOAD_DIR)/satl'
	@if [ -f satl-term ]; then \
	    install -m755 satl-term '$(DOWNLOAD_DIR)/satl-term'; \
	fi
	@install -m755 install.sh '$(DOWNLOAD_DIR)/install.sh'
# install.sh IS AN INDEX, and the installer is the nine files beside it. It has
# never been a file that works alone -- it needs a Makefile or an install_tree/
# next to it, and says so -- so the folder travelling with it costs the bundle
# nothing it was not already paying. A glob rather than a list, so a fragment
# added to install_support/ reaches the download folder without an edit here;
# 644 rather than 755, because they are sourced and never executed.
	@install -d -m755 '$(DOWNLOAD_DIR)/install_support'
	@install -m644 install_support/*.sh '$(DOWNLOAD_DIR)/install_support/'
# THE DATA HALF OF THE BUNDLE, and the reason install.sh works from inside it.
#
# The three files above are a program; they are not an install. The icons, the
# mime packet, the .desktop entry, the man pages, the examples, the design and
# the licence are the rest of it, and until this rule existed the bundle had
# none of them -- so install.sh, which drives `make install` and needs a
# Makefile beside it, exited with "no Makefile beside ./install.sh" and a user
# who downloaded, unpacked and ran it installed nothing at all.
#
# STAGED BY `make install` ITSELF rather than by a second list of files here.
# That is the whole point: install.sh's header says the install tree is declared
# exactly once, in the `install` target, because two lists is how an install
# tree rots. This bundle does not carry a copy of the list -- it carries the
# RESULT of the one list, produced by running it. Add a data file to `install`
# above and it is in the next bundle with no edit here.
#
# prefix= EMPTY, which is what makes the staged tree prefix-relative:
# $(bindir) becomes /bin and $(datadir) becomes /share, so the tree is
# install_tree/bin/satl and install_tree/share/..., and installing it anywhere
# is a copy with no path translation on either side. install.sh does not have
# to know what prefix this machine built at.
#
# -o satl -o satl-term GUI_TARGET=, for the reason `all` passes the same three
# words one target up: prefix is compiled into system.o through .libdir-stamp,
# so a sub-make at a DIFFERENT prefix -- and empty is a different prefix --
# would rewrite the stamp, recompile system.o and relink satl, at a prefix that
# is not a directory. --old-file on the two binaries says they are current, and
# the sub-make then reaches `install` with nothing to compile. **Verified**
# with `make -n`: zero compile lines, and .libdir-stamp is not touched.
#
# The binary in the tree is therefore the one built for THIS machine's prefix,
# with that prefix baked in as SATELLITE_LIB_DIR -- and it does not matter,
# because tier 2 of library_path() resolves ../share/satellite/lib from
# /proc/self/exe (DESIGN §9), and `install` creates that directory empty for
# exactly this reason. The bundle installs correctly at /usr/local, at
# $HOME/.local, or anywhere else, with no rebuild and no environment variable.
#
# DESTDIR is set, so `install` skips its own index-rebuild block -- correct
# here, this is staging. install.sh runs those three tools after it copies.
	@$(MAKE) --no-print-directory --silent -o satl -o satl-term GUI_TARGET= \
	    install prefix= DESTDIR='$(CURDIR)/$(BUNDLE_TREE)' \
	    SATELLITE_AUTOINSTALL=0
# The tarball sits OUTSIDE the folder it archives, in the project root, so that
# unpacking it recreates the folder rather than scattering three files into
# whatever directory the download landed in.
#
# --sort=name, and the mtime/owner clamps, for the reason `gzip -9n` is used on
# the man pages: two builds of the same binaries should produce the same
# archive, byte for byte, so a published checksum means something.
	@tar --create --xz --file '$(DOWNLOAD_TAR)' \
	     --sort=name --owner=root:0 --group=root:0 \
	     --mtime='@0' --format=gnu '$(DOWNLOAD_DIR)'
	@printf 'bundled    %s/  and  %s\n' '$(DOWNLOAD_DIR)' '$(DOWNLOAD_TAR)'

gui-skipped:
	@printf 'satl built.\n'
	@printf 'satl-term SKIPPED: pkg-config cannot find %s\n' '$(MISSING_PKGS)'
	@printf 'To build the terminal window too, %s\n' '$(DEPS_ADVICE)'

# Explicit, and never run by `make` on its own: installing system packages is
# the user's decision, and a build that quietly took it would be a build that
# runs sudo without being asked. No -y -- the package manager lists what it is
# about to do and asks, which is the confirmation this deliberately keeps.
deps:
ifeq ($(DEPS_CMD),)
	@printf 'No dnf, apt-get or pacman here -- %s\n' '$(DEPS_ADVICE)'
	@exit 1
else
	$(DEPS_CMD)
endif

satl: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ -pthread

satl-term: $(PROGRAMS)/window.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)
