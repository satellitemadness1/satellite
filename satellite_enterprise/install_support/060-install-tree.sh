# satellite -- WHAT gets installed. This is the one declaration of it.
#
# ONE LIST, read by both the install and the uninstall below, because two lists
# is how an install tree rots: the day somebody adds a file to one of them the
# other is silently wrong, and it surfaces as an orphan in a user's home
# directory rather than as an error here.
#
# The first satellite kept this list in its Makefile's `install` target and had
# install.sh call it, which is the better arrangement when a Makefile has such a
# target. The second satellite has no `install` target yet. WHEN IT GETS ONE,
# this function is what should be deleted -- not duplicated -- and this script
# should call make the way its predecessor did.

# The nine pixel sizes in satellite_enterprise/icons/hicolor. A list rather than
# a find, so that a size which failed to copy across is a named failure instead
# of an icon that is quietly absent at one resolution.
icon_sizes='16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256 512x512'

# Lines of "mode source destination-relative-to-root".
#
# THE SVG IS NOT IN THIS LIST AND MUST NOT BE. icons/org.satellite.terminal.svg
# is a complete icon and it travels in the tree, but the icon theme spec lets
# either a scalable or a pixel icon satisfy a lookup, so installing both makes
# which one a shell draws unpredictable. The first satellite found this and
# wrote it down; the artwork is a photograph, which has no scalable form, so the
# PNGs are the ones that ship. See make_support/140-install.mk in
# old_versions/first_satellite.
#
# THE .desktop ENTRY IS NOT IN THIS LIST EITHER, yet. It launches satl-term,
# which lands at M11 and does not exist in this tree; installing a launcher for
# a missing binary puts an entry in the user's menu that does nothing. It comes
# back as one line here in the milestone that builds the binary it names.
install_tree() {
    printf '755 %s satl\n' "${satl_source:--}"
    printf '644 %s share/mime/packages/application-x-satellite.xml\n' \
        "$here/icons/application-x-satellite.xml"

    for _size in $icon_sizes; do
        printf '644 %s share/icons/hicolor/%s/apps/org.satellite.terminal.png\n' \
            "$here/icons/hicolor/$_size/apps/org.satellite.terminal.png" "$_size"
        printf '644 %s share/icons/hicolor/%s/mimetypes/application-x-satellite.png\n' \
            "$here/icons/hicolor/$_size/mimetypes/application-x-satellite.png" "$_size"
    done
}

# Every directory a destination needs, and every directory above it, deepest
# first. Used to make them on the way in and to take them away on the way out.
tree_dirs() {
    install_tree | while read -r _mode _src _dst; do
        _d=${_dst%/*}
        [ "$_d" = "$_dst" ] && continue
        while [ -n "$_d" ] && [ "$_d" != "." ] && [ "$_d" != "/" ]; do
            printf '%s\n' "$_d"
            _parent=${_d%/*}
            [ "$_parent" = "$_d" ] && break
            _d=$_parent
        done
    done | sort -u | awk -F/ '{print NF, $0}' | sort -rn -k1,1 | cut -d' ' -f2-
}

if [ "$action" = install ]; then
    # EVERY SOURCE CHECKED BEFORE ANY DESTINATION IS WRITTEN, so that a missing
    # file leaves nothing half-installed. In a dry run the satl binary has not
    # necessarily been built, and its absence is not a failure to report -- the
    # transcript is describing what a real run would do after building, which is
    # what the bracketed placeholder from 050-building.sh stands for.
    while read -r _mode _src _dst; do
        [ "$_src" = "-" ] && continue
        case $_src in *'['*) continue ;; esac
        if [ ! -f "$_src" ]; then
            [ "$dry_run" = yes ] && continue
            die "$_src is missing, so nothing was installed.
       It is named in install_support/060-install-tree.sh, which is the one
       place this install tree is declared."
        fi
    done <<EOF
$(install_tree)
EOF

    step "installing into $root"

    run mkdir -p "$root"

    # The same tree_dirs the uninstall will walk, so the set of directories made
    # and the set removed are one set by construction rather than by agreement.
    while read -r _d; do
        [ -n "$_d" ] || continue
        run mkdir -p "$root/$_d"
    done <<EOF
$(tree_dirs | sort)
EOF

    while read -r _mode _src _dst; do
        run install -m "$_mode" "$_src" "$root/$_dst"
    done <<EOF
$(install_tree)
EOF
fi

if [ "$action" = uninstall ]; then
    # No heading here: 080-report.sh announces the removal, and announcing it
    # twice reads as two things having happened.
    while read -r _mode _src _dst; do
        run rm -f "$root/$_dst"
    done <<EOF
$(install_tree)
EOF

    # rmdir AND NEVER rm -rf. This is the first satellite's rule about `clean`,
    # and it matters more here: $HOME/.satl is a directory a user may reasonably
    # have put something of their own into, and a recursive delete aimed at a
    # variable is one bad --root away from taking a home directory with it.
    #
    # rmdir removes a directory only when it is empty, so anything this script
    # did not install survives, and 080-report.sh says what survived instead of
    # leaving it to be discovered. Deepest first, because a parent cannot be
    # empty until its children are gone.
    while read -r _d; do
        [ -n "$_d" ] || continue
        run rmdir "$root/$_d" 2>/dev/null || :
    done <<EOF
$(tree_dirs)
EOF
    run rmdir "$root" 2>/dev/null || :
fi
