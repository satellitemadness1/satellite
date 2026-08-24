# satellite -- the four things a staged tree cannot carry, and the call site.
#
# hicolor's index.theme and the three indexes a desktop reads once at session
# start. Defines bundle_reindex, then runs the bundle-mode block that calls it
# and the two functions in 060-bundle-tree.sh.
#
# Moved out of the 634-line install.sh on 2026-08-24, byte for byte.

# The four things `make install` does at the end of its own recipe and this
# staged tree could not carry: hicolor's index.theme, and the three indexes a
# desktop reads once at session start.
#
# The Makefile does all four only when DESTDIR is empty, and `make bundle`
# staged this tree with DESTDIR set -- correctly, because staging must not
# touch the build machine's live share/. So in bundle mode they fall to this
# script, and this is the ONE place where bundle mode restates something the
# Makefile also says. The rot warned about at the top of this file does not
# apply to it: this is four tool invocations, not a list of files, and nothing
# here goes stale when a data file is added to the install tree.
#
# Each of the four is argued in full in the Makefile's `install` recipe. The
# short version: an icon directory is not a THEME until it holds index.theme
# and GTK does not look inside one that does not, so without it every icon just
# installed is invisible at every size; and the other three are indexes, so a
# launcher and a file type stay unknown to the shell until they are rebuilt.
#
# index.theme is copied only when absent and only from the system's own copy,
# never invented -- it describes hicolor, not satellite -- and it is
# deliberately NOT removed on the way out, because every other application that
# installed an icon into this prefix depends on it.
#
# Failure is ignored throughout: none of these tools is required for the
# install to be CORRECT, only for it to be noticed before the next login.
bundle_reindex() {
    # A staged tree is not an installation. Rebuilding this machine's indexes
    # over a DESTDIR that will be packaged and installed somewhere else is at
    # best noise and at worst a package build editing the build machine.
    [ -n "$destdir" ] && return 0

    _hicolor=$prefix/share/icons/hicolor
    if [ -d "$_hicolor" ] && [ ! -f "$_hicolor/index.theme" ] &&
       [ -f /usr/share/icons/hicolor/index.theme ]; then
        run cp /usr/share/icons/hicolor/index.theme \
               "$_hicolor/index.theme" 2>/dev/null || :
    fi

    if command -v update-desktop-database >/dev/null 2>&1; then
        run update-desktop-database "$prefix/share/applications" \
            2>/dev/null || :
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        run gtk-update-icon-cache -qtf "$_hicolor" 2>/dev/null || :
    fi
    if command -v update-mime-database >/dev/null 2>&1; then
        run update-mime-database "$prefix/share/mime" 2>/dev/null || :
    fi
}

if [ "$mode" = source ]; then
    if [ -n "$destdir" ]; then
        run "$MAKE" -C "$repo" "$action" "prefix=$prefix" "DESTDIR=$destdir"
    else
        run "$MAKE" -C "$repo" "$action" "prefix=$prefix"
    fi
elif [ "$action" = install ]; then
    bundle_install
    bundle_reindex
else
    bundle_uninstall
    bundle_reindex
fi
