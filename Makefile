PKGS     = gtk4 vte-2.91-gtk4

# The clang 24 trunk build this project is developed against, but ONLY if it is
# actually there. It lives under $HOME, and $HOME is not a constant: `sudo make`
# runs with HOME=/root, so a hard-coded $(HOME)/.local/... resolved to
# /root/.local/llvm/bin/clang++ and the build died with "No such file or
# directory" on a machine where the compiler was sitting in plain sight. The
# same absence is the normal case on any build machine, which is why
# debian/rules already overrides CXX by hand.
#
# origin, rather than ?=, because CXX is one of make's built-in variables and is
# therefore already set: ?= would never fire. `default` means nobody has chosen,
# so `make CXX=g++` and CXX from the environment both still win.
LLVM_BIN = $(HOME)/.local/llvm/bin
ifeq ($(origin CXX),default)
  CXX := $(if $(wildcard $(LLVM_BIN)/clang++),$(LLVM_BIN)/clang++,c++)
endif

# The GUI flags are DELIBERATELY not in CXXFLAGS. Linking the interpreter
# against gtk4 and vte pulls 119 shared objects that the dynamic linker loads
# before main() on every run, and `satl --run` touches none of them:
# 25.9 ms with the link, 2.5 ms without, against a 2.2 ms bare-process floor.
# Only window.o gets them, and only satl-term links them.
CXXFLAGS = -std=c++20 -Wall -Wextra -O2
GTKFLAGS = $(shell pkg-config --cflags $(PKGS))
LDLIBS   = $(shell pkg-config --libs $(PKGS))

# LDFLAGS is set nowhere in this file on purpose, so that a distribution's
# link-time hardening -- -Wl,-z,relro,-z,now from dpkg-buildflags, and whatever
# the next one adds -- arrives from the environment or the command line and
# reaches both link rules below. A Makefile that never mentions the variable is
# a Makefile those flags cannot reach.
LDFLAGS ?=

# ThreadSanitizer is the one part of the suite that is not portable: it is a
# 64-bit-only runtime, and Debian builds libtsan2 for amd64, arm64, mips64el,
# ppc64el, riscv64 and s390x and for nothing else, so on i386 or armhf
# -fsanitize=thread does not compile at all. TSAN=0 drops that one binary and
# still runs the other ten, which is what a package build on such an
# architecture needs: both binary packages are Architecture: any, and that is a
# promise the package builds everywhere (Debian Policy 5.6.8).
TSAN      ?= 1
TSAN_TEST  = $(if $(filter-out 0,$(TSAN)),library_test_tsan)

# `prefix` is baked into the binary; DESTDIR is staging and is baked into
# NOTHING. The distinction is the whole contract with a packaging system: a
# .deb is built with prefix=/usr into a DESTDIR chroot, and a binary that had
# learned the chroot's name would look for its library there on the user's
# machine, where that directory does not exist.
prefix  ?= /usr/local
DESTDIR ?=
bindir   = $(prefix)/bin
datadir  = $(prefix)/share
mandir   = $(datadir)/man
docdir   = $(datadir)/doc/satellite

OBJS      = main.o library.o satellite_string.o system.o bignum.o lexer.o \
            ast.o parser.o value.o env.o eval.o interp.o
HDRS      = library.hpp value.hpp satellite_string.hpp system.hpp bignum.hpp \
            lexer.hpp ast.hpp parser.hpp env.hpp eval.hpp interp.hpp
TESTSRCS  = library.cpp satellite_string.cpp system.cpp bignum.cpp lexer.cpp \
            ast.cpp parser.cpp value.cpp env.cpp eval.cpp interp.cpp
TESTFLAGS = -std=c++20 -Wall -Wextra -pthread

all: satl satl-term

satl: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ -pthread

satl-term: window.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

window.o: window.cpp
	$(CXX) $(CXXFLAGS) $(GTKFLAGS) -c -o $@ window.cpp

# The prefix is written to a file so that make can see it. Make invalidates a
# target when a PREREQUISITE changes, and the value below is not a prerequisite
# of anything -- so without this stamp, `make && make install prefix=/usr`
# reuses the system.o compiled for /usr/local and installs a binary whose
# last-resort library directory names a prefix nothing was ever installed to,
# while `satl --where` reports that directory as fact. The stamp is
# rewritten only when the value actually changes, so a rebuild at the same
# prefix still recompiles nothing.
.libdir-stamp: FORCE
	@printf '%s' '$(datadir)/satellite/lib' | cmp -s - $@ || \
	    printf '%s' '$(datadir)/satellite/lib' > $@

FORCE:

# system.o is the only object that learns the install prefix, so retargeting a
# build invalidates one object and not twelve. DESTDIR is deliberately absent
# from this define -- see the prefix/DESTDIR note above.
system.o: system.cpp .libdir-stamp
	$(CXX) $(CXXFLAGS) -DSATELLITE_LIB_DIR='"$(datadir)/satellite/lib"' \
	    -c -o $@ system.cpp

$(OBJS): $(HDRS)

library_test: library_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O2 -o $@ library_test.cpp $(TESTSRCS)

library_test_tsan: library_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O1 -g -fsanitize=thread -o $@ library_test.cpp $(TESTSRCS)

satellite_string_test: satellite_string_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O2 -o $@ satellite_string_test.cpp $(TESTSRCS)

lexer_test: lexer_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O2 -o $@ lexer_test.cpp $(TESTSRCS)

ast_test: ast_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O2 -o $@ ast_test.cpp $(TESTSRCS)

parser_test: parser_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O2 -o $@ parser_test.cpp $(TESTSRCS)

eval_test: eval_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O2 -o $@ eval_test.cpp $(TESTSRCS)

interp_test: interp_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O2 -o $@ interp_test.cpp $(TESTSRCS)

env_test: env_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O2 -o $@ env_test.cpp $(TESTSRCS)

spacesuit_test: spacesuit_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O2 -o $@ spacesuit_test.cpp $(TESTSRCS)

bignum_test: bignum_test.cpp $(TESTSRCS) $(HDRS)
	$(CXX) $(TESTFLAGS) -O2 -o $@ bignum_test.cpp $(TESTSRCS)

test: library_test $(TSAN_TEST) satellite_string_test bignum_test lexer_test ast_test parser_test env_test eval_test interp_test spacesuit_test
	./library_test
	$(if $(TSAN_TEST),./$(TSAN_TEST))
	./satellite_string_test
	./bignum_test
	./lexer_test
	./ast_test
	./parser_test
	./env_test
	./eval_test
	./interp_test
	./spacesuit_test

# satellite against compiled C++ with the compiler's own time counted, which is
# the comparison that changes the answer. Depends on `satl` because the
# driver refuses to guess at an interpreter that is not built; the build of the
# interpreter is deliberately not part of what it measures.
compare: satl
	$(MAKE) -C example/cxx_compare run

# satellite against CPython. Two interpreters, so there is no build step on
# either side and the number is simply the whole-process wall clock.
python: satl
	$(MAKE) -C example/py_compare run

# gzip -9n, not -9c on a named file: -n keeps the source file name and the
# current time out of the gzip header, which is what makes two builds of the
# same page byte-identical and is why dh_compress uses it. dh_compress cannot
# repair ours, because it skips what is already .gz. Compressing here rather
# than in the install recipe is what puts the modes under install(1) instead of
# under the caller's umask; the temporary file is so that a failed gzip cannot
# leave a truncated page behind under a name make would then believe in.
dist/%.1.gz: dist/%.1
	gzip -9nc $< > $@.tmp && mv $@.tmp $@

# The tree that library_path()'s tiers 2 and 3 both describe. Its whole
# obligation is that `satl --where` answers correctly afterwards with no
# environment variable set -- an install that needs one is not an install.
#
# Every path is quoted, because prefix and DESTDIR are given by the caller and
# a tarball is routinely unpacked, or staged, somewhere with a space in the
# path. Unquoted, `prefix="/opt/my satellite"` turns one path into two words
# and the recipes below silently install to the wrong places -- and the
# uninstall recipe, being rm -rf, deletes them.
install: satl satl-term dist/satl.1.gz dist/satl-term.1.gz
	install -Dm755 satl "$(DESTDIR)$(bindir)/satl"
	install -Dm755 satl-term "$(DESTDIR)$(bindir)/satl-term"
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
	install -Dm644 dist/org.satellite.terminal.desktop \
	    "$(DESTDIR)$(datadir)/applications/org.satellite.terminal.desktop"
# The icon is scalable/, not a pixel size: one SVG answers every size a shell
# asks for, and hicolor is the theme every other theme falls back to, so the
# icon is found whichever theme the user has chosen. The basename is the
# application id, which is what Icon= in the .desktop entry names.
	install -Dm644 dist/org.satellite.terminal.svg \
	    "$(DESTDIR)$(datadir)/icons/hicolor/scalable/apps/org.satellite.terminal.svg"
# A shell finds a launcher through two indexes, and a newly installed .desktop
# and icon are invisible until both are rebuilt -- which is why a fresh install
# shows the generic icon until the next login. Skipped entirely when DESTDIR is
# set: that tree is staging for a package, and dpkg fires its own triggers on
# the installing machine. Failure is ignored because neither tool is required
# for the install to be correct, only for it to be noticed promptly.
	@if [ -z "$(DESTDIR)" ]; then \
	    command -v update-desktop-database >/dev/null 2>&1 && \
	        update-desktop-database "$(datadir)/applications" 2>/dev/null || true; \
	    command -v gtk-update-icon-cache >/dev/null 2>&1 && \
	        gtk-update-icon-cache -qtf "$(datadir)/icons/hicolor" 2>/dev/null || true; \
	fi
# bash-completion loads the file named after the command, so the extension
# that distinguishes it in dist/ is dropped on the way in.
	install -Dm644 dist/satl.bash-completion \
	    "$(DESTDIR)$(datadir)/bash-completion/completions/satl"

# Symmetric with install, and the asymmetry in the commands is deliberate:
# share/satellite and share/doc/satellite are directories this install created
# and owns outright, so removing the tree is exact. Everything else lives in a
# directory shared with the rest of the system, where only the named files may
# go and the directory itself must stay.
uninstall:
	rm -f "$(DESTDIR)$(bindir)/satl" "$(DESTDIR)$(bindir)/satl-term"
	rm -f "$(DESTDIR)$(datadir)/icons/hicolor/scalable/apps/org.satellite.terminal.svg"
	rm -f "$(DESTDIR)$(mandir)/man1/satl.1.gz" \
	      "$(DESTDIR)$(mandir)/man1/satl-term.1.gz"
	rm -f "$(DESTDIR)$(datadir)/applications/org.satellite.terminal.desktop"
	rm -f "$(DESTDIR)$(datadir)/bash-completion/completions/satl"
	rm -rf "$(DESTDIR)$(datadir)/satellite"
	rm -rf "$(DESTDIR)$(docdir)"

clean:
	$(MAKE) -C example/cxx_compare clean
	$(MAKE) -C example/py_compare clean
	rm -f satl satl-term library_test library_test_tsan satellite_string_test \
	      lexer_test ast_test parser_test env_test eval_test interp_test \
	      spacesuit_test bignum_test *.o *.o.tmp .libdir-stamp \
	      dist/satl.1.gz dist/satl-term.1.gz

.PHONY: all test compare python install uninstall clean FORCE
