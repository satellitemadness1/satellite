# satellite -- the install tree, declared exactly once.
#
# THIS RULE IS THE FILE LIST. install.sh says so in its header, in as many words,, and the
# bundle in 100-build.mk is produced by RUNNING this target into a DESTDIR
# rather than by a second list. A data file added here reaches the download
# folder, the .deb and `make install` at once; a data file added anywhere else
# reaches none of them.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# The tree that library_path()'s tiers 2 and 3 both describe. Its whole
# obligation is that `satl --where` answers correctly afterwards with no
# environment variable set -- an install that needs one is not an install.
#
# Every path is quoted, because prefix and DESTDIR are given by the caller and
# a tarball is routinely unpacked, or staged, somewhere with a space in the
# path. Unquoted, `prefix="/opt/my satellite"` turns one path into two words
# and the recipes below silently install to the wrong places -- and the
# uninstall recipe, being rm -rf, deletes them.
#
# $(GUI_TARGET) rather than satl-term, and the two satl-term lines below ask
# whether the file is there: this target is now reached by `all`, and `all` is
# what must keep working on a machine with no gtk. Naming satl-term outright
# made the missing-package guard on window.o fire (Makefile's note on
# MISSING_PKGS at the top), which turned the graceful skip into the hard build
# failure that guard exists to prevent. The interpreter still installs there,
# alone, which is the same argument one target up: there is no reason for it to
# go down with the window. A package build is unaffected -- debian/control
# Build-Depends on both libraries, so the file is always there -- and if it ever
# were not, debian/satellite-term.install names usr/bin/satl-term and dh_install
# fails loudly, which is where that failure belongs.
install: satl $(GUI_TARGET) dist/satl.1.gz dist/satl-term.1.gz
	install -Dm755 satl "$(DESTDIR)$(bindir)/satl"
	if [ -f satl-term ]; then \
	    install -Dm755 satl-term "$(DESTDIR)$(bindir)/satl-term"; \
	fi
# Every directory this install creates gets its mode stated, for the same
# reason every file does. install -d without -m takes the umask, so under the
# 002 that is Ubuntu's default for the primary user -- and common in CI images
# -- share/ and share/satellite/ came out group-writable, which is a package
# shipping a group-writable /usr/share. Each level is named, because -m applies
# to the directories named and not to the ancestors created along the way.
#
# lib/ is created empty on purpose. There are no .satl files to ship yet, but
# the search accepts a candidate only if the DIRECTORY is there, so this empty
# directory is exactly what makes a relocated tarball resolve to itself instead
# of falling through to the prefix it was built for.
	install -d -m755 "$(DESTDIR)$(datadir)" \
	                 "$(DESTDIR)$(datadir)/satellite" \
	                 "$(DESTDIR)$(datadir)/satellite/lib" \
	                 "$(DESTDIR)$(mandir)" "$(DESTDIR)$(mandir)/man1"
# example/ is a tree, not a flat list, so it is walked rather than globbed.
# Executables are skipped: `make compare` and `make python` leave their
# compiled C++ drivers in these directories, and one machine's binaries are
# not an example of anything.
	find example -type f ! -perm -u+x -printf '%P\n' | while read -r f; do \
	    install -Dm644 "example/$$f" \
	        "$(DESTDIR)$(datadir)/satellite/examples/$$f" || exit 1; \
	done
	install -Dm644 DESIGN.md "$(DESTDIR)$(docdir)/DESIGN.md"
# DESIGN.md is now the index and design/ is the document -- nineteen numbered
# sections, one per file -- so installing one without the other ships a page
# of links to nothing. Globbed rather than walked because design/ is flat and
# every file in it is a part; -t rather than a per-file -D because a
# multi-source install needs the destination named as a directory. The leading
# directories still come out 755 under a 002 umask, which is what the note on
# `install -d` above is about -- **verified** rather than assumed.
	install -Dm644 -t "$(DESTDIR)$(docdir)/design" design/*.md
# The licence ships under the name every tool looks for, so that a tarball
# install states its terms as fully as the .deb does: dh_installdocs writes
# debian/copyright to this same path, and debian/copyright is a transcription
# of LICENSE precisely so the two cannot say different things.
	install -Dm644 LICENSE "$(DESTDIR)$(docdir)/copyright"
# README.md is not written yet. The guard is so that `make install` works today
# and picks it up the day it lands, rather than failing now and needing a
# second edit here later. It is an `if` rather than a `|| true` so that a real
# failure to copy an existing README still stops the install.
	if [ -f README.md ]; then \
	    install -Dm644 README.md "$(DESTDIR)$(docdir)/README.md"; \
	fi
	install -Dm644 dist/satl.1.gz \
	    "$(DESTDIR)$(mandir)/man1/satl.1.gz"
	install -Dm644 dist/satl-term.1.gz \
	    "$(DESTDIR)$(mandir)/man1/satl-term.1.gz"
# Named after the GApplication id that window.cpp registers, because that is
# the string GTK puts on the toplevel and the string a desktop shell looks a
# .desktop file up by. Installed as satl-term.desktop the window arrives in the
# shell associated with nothing: no icon, and nothing to pin.
# Under the same guard as the binary: a launcher whose Exec= names a program
# that was never built is an entry in the shell's menu that fails when clicked.
	if [ -f satl-term ]; then \
	    install -Dm644 dist/org.satellite.terminal.desktop \
	        "$(DESTDIR)$(datadir)/applications/org.satellite.terminal.desktop"; \
	fi
# The icon tree under dist/icons mirrors its install destination exactly, so
# this is a copy and not a translation: every path under dist/icons/hicolor is
# already <size>/<context>/<name>, and getting the layout wrong is a mistake
# that shows up as a missing icon rather than as a build failure. hicolor is
# the theme every other theme falls back to, so the artwork is found whichever
# theme the user has chosen, and the basenames are the two names that are
# looked up: org.satellite.terminal for Icon= in the .desktop entry, and
# application-x-satellite for the mime type declared in
# dist/application-x-satellite.xml.
#
# These are pixel sizes rather than the single scalable/ SVG that used to live
# here, because the artwork is now a photograph and a photograph has no
# scalable form. dist/org.satellite.terminal.svg is still in the tree and is
# still a complete icon: to go back to it, restore the one-line install of
# scalable/apps/ and drop the apps/ half of this walk. Installing BOTH is the
# one thing that does not work -- the theme spec lets either satisfy a lookup,
# so which one a shell picks stops being predictable.
	find dist/icons -type f -name '*.png' -printf '%P\n' | while read -r f; do \
	    install -Dm644 "dist/icons/$$f" \
	        "$(DESTDIR)$(datadir)/icons/$$f" || exit 1; \
	done
# The mime packet, which is what makes a .satl file a satellite file rather
# than an unlabelled text file. It has to be installed before
# update-mime-database runs below: that tool compiles every packet in
# packages/ into the binary index a file manager actually reads, and a packet
# added afterwards is inert until something triggers the compile again.
	install -Dm644 dist/application-x-satellite.xml \
	    "$(DESTDIR)$(datadir)/mime/packages/application-x-satellite.xml"
# A shell finds a launcher through two indexes, and a newly installed .desktop
# and icon are invisible until both are rebuilt -- which is why a fresh install
# shows the generic icon until the next login. Skipped entirely when DESTDIR is
# set: that tree is staging for a package, and dpkg fires its own triggers on
# the installing machine. Failure is ignored because neither tool is required
# for the install to be correct, only for it to be noticed promptly.
#
# update-mime-database joins them for the same reason and under the same guard:
# the packet installed above is XML that nothing reads directly, and until it
# is compiled into share/mime/mime.cache a .satl file keeps whatever type
# content sniffing alone gives it.
#
# The index.theme line is not decoration. An icon directory is only a THEME if
# it contains one, and without it GTK does not look inside at all: has_icon()
# answers false for every icon just installed, at every size, and the desktop
# shows the generic fallback. The file belongs to hicolor-icon-theme, which
# installs it under /usr and nowhere else -- so EVERY prefix except /usr starts
# without one, and that includes this Makefile's own default of /usr/local. An
# install that leaves it missing has copied eighteen PNGs nothing will ever read.
#
# gtk-update-icon-cache does not reveal the problem, because -t is
# --ignore-theme-index: the cache builds happily over a directory that is not
# yet a theme, so the only symptom is artwork that never appears.
#
# Copied rather than generated, because the file describes hicolor itself -- its
# directory list and their sizes and contexts -- and not our subset of it. Only
# when absent, so any prefix that already has one is left alone.
#
# It is deliberately NOT removed by uninstall below. Every other application
# that installs an icon into this prefix depends on it, so deleting ours on the
# way out would break theirs; an orphaned theme index is the correct outcome.
	@if [ -z "$(DESTDIR)" ]; then \
	    if [ ! -f "$(datadir)/icons/hicolor/index.theme" ] && \
	       [ -f /usr/share/icons/hicolor/index.theme ]; then \
	        cp /usr/share/icons/hicolor/index.theme \
	           "$(datadir)/icons/hicolor/index.theme" 2>/dev/null || true; \
	    fi; \
	    command -v update-desktop-database >/dev/null 2>&1 && \
	        update-desktop-database "$(datadir)/applications" 2>/dev/null || true; \
	    command -v gtk-update-icon-cache >/dev/null 2>&1 && \
	        gtk-update-icon-cache -qtf "$(datadir)/icons/hicolor" 2>/dev/null || true; \
	    command -v update-mime-database >/dev/null 2>&1 && \
	        update-mime-database "$(datadir)/mime" 2>/dev/null || true; \
	fi
# bash-completion loads the file named after the command, so the extension
# that distinguishes it in dist/ is dropped on the way in.
	install -Dm644 dist/satl.bash-completion \
	    "$(DESTDIR)$(datadir)/bash-completion/completions/satl"
