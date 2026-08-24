# satellite -- taking it all back out again.
#
# LAST of the sixteen, because .PHONY at the bottom reads $(TESTALIASES) from
# 120-tests.mk and a prerequisite list is expanded where it is read.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# Symmetric with install, and the asymmetry in the commands is deliberate:
# share/satellite and share/doc/satellite are directories this install created
# and owns outright, so removing the tree is exact. Everything else lives in a
# directory shared with the rest of the system, where only the named files may
# go and the directory itself must stay.
uninstall:
	rm -f "$(DESTDIR)$(bindir)/satl" "$(DESTDIR)$(bindir)/satl-term"
# The icons are removed by the same walk that installed them, so the two lists
# cannot drift: hicolor is a directory shared with every other application on
# the system, so only the files this install named may go, and the size
# directories themselves must stay even when ours was the only icon in one.
	find dist/icons -type f -name '*.png' -printf '%P\n' | while read -r f; do \
	    rm -f "$(DESTDIR)$(datadir)/icons/$$f"; \
	done
	rm -f "$(DESTDIR)$(datadir)/mime/packages/application-x-satellite.xml"
	rm -f "$(DESTDIR)$(mandir)/man1/satl.1.gz" \
	      "$(DESTDIR)$(mandir)/man1/satl-term.1.gz"
	rm -f "$(DESTDIR)$(datadir)/applications/org.satellite.terminal.desktop"
	rm -f "$(DESTDIR)$(datadir)/bash-completion/completions/satl"
	rm -rf "$(DESTDIR)$(datadir)/satellite"
	rm -rf "$(DESTDIR)$(docdir)"
# Both indexes are rebuilt on the way out as well as on the way in. Without
# this the launcher stays in the shell's menu and .satl files keep an icon
# whose file is gone, which reads to a user as an uninstall that did not work.
	@if [ -z "$(DESTDIR)" ]; then \
	    command -v update-desktop-database >/dev/null 2>&1 && \
	        update-desktop-database "$(datadir)/applications" 2>/dev/null || true; \
	    command -v gtk-update-icon-cache >/dev/null 2>&1 && \
	        gtk-update-icon-cache -qtf "$(datadir)/icons/hicolor" 2>/dev/null || true; \
	    command -v update-mime-database >/dev/null 2>&1 && \
	        update-mime-database "$(datadir)/mime" 2>/dev/null || true; \
	fi

clean:
	$(MAKE) -C example/cxx_compare clean
	$(MAKE) -C example/py_compare clean
# example/website has had a clean target of its own all along and this file
# never called it, so `tour` and the two .out files it diffs survived every
# clean in the tree. Called now, for the reason the two above are.
	$(MAKE) -C example/website clean
# library_test_tsan is named outright rather than through $(TSAN_TEST), so that
# `make clean TSAN=0` still removes the 27MB binary an earlier build left.
	rm -f satl satl-term $(TESTBINS) $(TESTS)/library_test/library_test_tsan \
	      $(SRC)/*/*.o $(SRC)/*/*.o.tmp .libdir-stamp .cxxflags-stamp \
	      dist/satl.1.gz dist/satl-term.1.gz
# Three compiled things no rule in this file builds and no clean target had
# ever claimed. example/website/gtk_window is the one that matters: debian/
# rules deletes it during a package build with a note saying nothing else
# does, and an ELF binary under share/satellite/examples breaks Debian Policy
# 9.1.1's architecture-independence rule. The other two sit beside the sources
# they were compiled from. A build product that survives `make clean` is a
# build product that ends up in somebody's tarball.
	rm -f example/website/gtk_window pcg_test/pcg_test speed_test/cxx_speed
	rm -rf '$(DOWNLOAD_DIR)' '$(DOWNLOAD_TAR)'

.PHONY: all test compare python install install-report uninstall clean deps \
        bundle gui-skipped FORCE $(TESTALIASES)
