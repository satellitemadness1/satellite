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
# THE .desktop ENTRY IS IN THIS LIST AS OF 2026-08-28, and it was held out until
# now for a reason that has expired. It launches satl-term, and installing a
# launcher for a missing binary puts an entry in the user's menu that does
# nothing -- so it waited for the milestone that builds the binary it names.
# M11.A built satl-term on 2026-08-27, PLAN.md sec 5.3 says the remaining step
# was naming it here, and this is that line. It is still conditional: the entry
# and the binary arrive together or neither arrives, because the reason for
# holding it back was never the date, it was the binary.
#
# THREE PROGRAMS INSTALL, OR FOUR ARE BUILT AND THREE INSTALL, and the
# difference is worth stating in the one place the tree is declared:
#
#     satl            the interpreter, whichever of the two builds this CPU can
#                     run. See 050-building.sh for the choice.
#     satl-cpu-level  the program that made that choice, kept because it is
#                     also how a person checks it afterwards, and because a
#                     machine that is upgraded is a machine whose answer moved.
#     satl-term       the GTK4/VTE window, when the libraries to build it were
#                     there.
#
# satl.haswell IS NOT A FOURTH PROGRAM AND IS NOT INSTALLED. It is the same
# program as satl compiled against a second instruction set, and exactly one of
# the pair is installed, under the name satl, by design: PLAN.md sec 4.2's
# whole argument is that the installed binary is its own record -- `satl
# --version` prints the flags its objects were compiled with -- which stops
# being true the moment two interpreters sit in the root and something has to
# say which one runs. The build makes four files; the install is three
# programs.
#
# satl-term FINDS ITS INTERPRETER BY SITTING NEXT TO IT. src/programs/terminal.cpp
# reads /proc/self/exe and spawns the `satl` beside it -- "not this binary
# again, and not whatever PATH happens to resolve" -- so these two must land in
# ONE directory, which the fixed root gives for free and a prefix layout with a
# bin/ would also give. It is written down here because it is a constraint on
# this list rather than a property of the tree: move satl-term out of the root
# and the window spawns nothing.
install_tree() {
    printf '755 %s satl\n' "${satl_source:--}"

    # UNCONDITIONAL ON x86-64 AND ABSENT EVERYWHERE ELSE, which is why this asks
    # the file rather than the architecture: 045-microarchitecture.mk builds one
    # satl and no detector on a machine that has no variants to choose between,
    # and 050-building.sh has already run whatever is there.
    if [ -x "$repo/satl-cpu-level" ] || [ "$action" = uninstall ]; then
        printf '755 %s satl-cpu-level\n' "$repo/satl-cpu-level"
    fi

    # THE WINDOW AND ITS LAUNCHER, TOGETHER OR NOT AT ALL. On an uninstall both
    # are named whatever this run knows, because an uninstall builds nothing and
    # so never learns whether they were installed; rm -f on a file that is not
    # there costs nothing, and leaving a binary behind because this run could
    # not prove it was installed is how a tree rots. 010-defaults.sh says the
    # same thing where have_term is declared.
    case ${have_term:-unknown} in
        yes)
            printf '755 %s satl-term\n' "$repo/satl-term"
            printf '644 %s share/applications/org.satellite.terminal.desktop\n' \
                "$here/icons/org.satellite.terminal.desktop"
            ;;
        undecided)
            # A dry run on a tree that has not been built. The bracket is the
            # same device 050-building.sh uses for satl: it names a file the
            # transcript cannot yet be sure of, and the check below skips any
            # source with a bracket in it rather than reporting it missing.
            printf '755 %s satl-term\n' "$repo/satl-term[ if gtk4 is present ]"
            printf '644 %s share/applications/org.satellite.terminal.desktop\n' \
                "$here/icons/org.satellite.terminal.desktop"
            ;;
        *)
            # no, or unknown-on-an-uninstall. Nothing is printed on an install
            # with no window; an uninstall prints both so it can remove them.
            # A `return` here instead of an `if` would drop the mime packet and
            # the icons from the rest of this function, which is the one way a
            # gate inside a list of files can go badly wrong.
            if [ "$action" = uninstall ]; then
                printf '755 %s satl-term\n' "$repo/satl-term"
                printf '644 %s share/applications/org.satellite.terminal.desktop\n' \
                    "$here/icons/org.satellite.terminal.desktop"
            fi
            ;;
    esac

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
