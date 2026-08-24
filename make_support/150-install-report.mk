# satellite -- what the install says when it is done.
#
# A separate target and not the tail of `install`, so that `make install` on a
# packaging machine says nothing about a PATH nobody there will type into.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# What the auto-install says when it is done, and the reason it is a target of
# its own rather than three more lines in `all`: a recipe line that mentions
# $(MAKE) is executed even under `make -n` -- that is how a dry run recurses at
# all -- so a report printed inline would announce an install that a dry run did
# not perform. Reached as a GOAL of that sub-make it inherits -n along with
# everything else and is printed rather than run, which is what a dry run is for.
#
# It is deliberately NOT part of `install`. A packager staging a tree into
# debian/tmp is not the audience for advice about this machine's PATH.
install-report:
	@printf 'installed  %s\n' '$(bindir)/satl'
	@if [ -f satl-term ]; then printf 'installed  %s\n' '$(bindir)/satl-term'; fi
	@printf 'icons      %s\n' '$(datadir)/icons/hicolor'
# Membership in PATH is not the question; PRECEDENCE is. `satl` runs whichever
# copy the shell finds first, so an install into a directory that is on PATH but
# behind another directory that also has a satl changes nothing the user can
# see -- and printing "installed" and stopping there would be the kind of true
# sentence that misleads. install.sh:332 asks the membership question and says
# "so `satl` finds it", which on this machine was already false.
#
# -ef as well as a string compare, because a PATH entry can reach the same
# directory through a symlink: the strings differ, the file does not, and a
# string compare alone reports a shadow that is not there.
#
# `hash -r` is named only in the branch that needs it. Overwriting a binary in
# place needs nothing, because the shell caches the resolved path and not the
# inode; CREATING one earlier in PATH than the copy the shell has already hashed
# is the case that needs the reminder. Advice printed every time is advice
# nobody reads.
	@found=`command -v satl 2>/dev/null || :`; \
	mine='$(bindir)/satl'; \
	if [ -z "$$found" ]; then \
	    printf 'note: %s is not on your PATH, so typing `satl` will not find it.\n' \
	        '$(bindir)'; \
	    printf '      Nothing was changed for you -- no startup file was edited.\n'; \
	    printf '      To add it yourself:  export PATH="%s:$$PATH"\n' '$(bindir)'; \
	elif [ "$$found" = "$$mine" ] || [ "$$found" -ef "$$mine" ]; then \
	    printf 'satl       on your PATH is this one\n'; \
	else \
	    printf 'note: `satl` still runs %s, not the copy just installed.\n' "$$found"; \
	    printf '      An earlier PATH entry shadows %s.\n' '$(bindir)'; \
	    printf '      If this shell has run satl already, run: hash -r\n'; \
	fi
# GTK resolves an icon name to the LAST base directory in its search path that
# holds it, and not the first -- measured on gtk4 4.16.7, in both directions:
# with the search path [A,B] the icon in B answers, with [B,A] the one in A
# does. That is the reverse of what the icon theme spec says, and it is the
# difference between an install that changes the picture on the screen and one
# that does not. $HOME/.local/share/icons comes FIRST in that path, so a copy of
# this artwork left behind in /usr/local/share/icons or /usr/share/icons -- both
# later -- goes on being drawn however many times this install refreshes ours.
#
# Reported, never repaired: those directories belong to root, and the note above
# `all` is that a build does not escalate. cmp rather than mtimes, because
# "different bytes" is the question and a timestamp answers a different one.
	@if [ -z "$(DESTDIR)" ]; then \
	    for base in /usr/local/share/icons /usr/share/icons; do \
	        [ -d "$$base" ] || continue; \
	        [ "$$base" = "$(datadir)/icons" ] && continue; \
	        stale=0; \
	        for f in `find dist/icons -type f -name '*.png' -printf '%P\n'`; do \
	            if [ -f "$$base/$$f" ] && \
	               ! cmp -s "dist/icons/$$f" "$$base/$$f"; then \
	                stale=`expr $$stale + 1`; \
	            fi; \
	        done; \
	        if [ "$$stale" -gt 0 ]; then \
	            other=`dirname "$$base"`; other=`dirname "$$other"`; \
	            printf 'note: %s holds %s satellite icons that are not these,\n' \
	                "$$base" "$$stale"; \
	            printf '      and GTK reads that directory AFTER %s.\n' \
	                '$(datadir)/icons'; \
	            printf '      The last one with an icon wins, so the desktop keeps drawing\n'; \
	            printf '      the old artwork. It belongs to root, so this build leaves it:\n'; \
	            printf '          sudo ./install.sh --prefix %s              refreshes it\n' \
	                "$$other"; \
	            printf '          sudo ./install.sh --uninstall --prefix %s  removes it\n' \
	                "$$other"; \
	        fi; \
	    done; \
	fi
