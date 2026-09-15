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
_TAB=$(printf '\t')

icon_sizes='16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256 512x512'

# Records of "mode<TAB>source<TAB>destination-relative-to-root".
#
# TAB-SEPARATED, AND THAT IS A CORRECTNESS REQUIREMENT RATHER THAN A STYLE.
# These records carry two paths chosen by the caller -- $repo and $root -- and
# `read` splits on IFS, which by default includes the space. So a single space
# anywhere in either path silently reassigns the fields: the destination becomes
# the tail of the source, and the install writes somewhere nobody named.
#
# MEASURED 2026-08-28 with HOME set to ".../we ird%home". The install reported
# success and created a directory tree called `ird%home/` in the CURRENT
# WORKING DIRECTORY -- inside the source tree -- containing nested directories
# named `satl`, `satl /tmp`, `satl /tmp/claude-1000` and so on, one per fragment
# of the split path. Exit status 0 throughout. That is the worst shape a bug can
# have: it does not fail, it writes to a path it invented.
#
# A tab cannot appear in these records by accident the way a space can, and the
# readers set IFS to exactly it, so the fields are the fields. The first
# satellite's Makefile quoted every path in its install rules for this same
# reason and said so; this is that care applied to the format that carries them.

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
# M1.5 built satl-term on 2026-08-27, PLAN.md sec 5.3 says the remaining step
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
# bin_rel IS EMPTY OR IT IS "bin/", and it is prepended to the three program
# names below and to nothing else. 010-defaults.sh sets it; --system sets it to
# bin/. share/ needs no equivalent because it was already written in its
# XDG-relative form -- share/icons/hicolor/... , share/mime/packages/... -- so
# the same lines describe a prefix and a private root without a translation
# table, which is what 010's note about share/ under the root was for.
#
# satl-term FINDS ITS INTERPRETER BY SITTING NEXT TO IT. src/programs/terminal.cpp
# reads /proc/self/exe and spawns the `satl` beside it -- "not this binary
# again, and not whatever PATH happens to resolve" -- so these two must land in
# ONE directory, which the fixed root gives for free and a prefix layout with a
# bin/ would also give. It is written down here because it is a constraint on
# this list rather than a property of the tree: move satl-term out of the root
# and the window spawns nothing.
install_tree() {
    printf '755\t%s\t%ssatl\n' "${satl_source:--}" "$bin_rel"

    # UNCONDITIONAL ON x86-64 AND ABSENT EVERYWHERE ELSE, which is why this asks
    # the file rather than the architecture: 045-microarchitecture.mk builds one
    # satl and no detector on a machine that has no variants to choose between,
    # and 050-building.sh has already run whatever is there.
    if [ -x "$repo/satl-cpu-level" ] || [ "$action" = uninstall ]; then
        printf '755\t%s\t%ssatl-cpu-level\n' "$repo/satl-cpu-level" "$bin_rel"
    fi

    # THE WINDOW AND ITS LAUNCHER, TOGETHER OR NOT AT ALL. On an uninstall both
    # are named whatever this run knows, because an uninstall builds nothing and
    # so never learns whether they were installed; rm -f on a file that is not
    # there costs nothing, and leaving a binary behind because this run could
    # not prove it was installed is how a tree rots. 010-defaults.sh says the
    # same thing where have_term is declared.
    case ${have_term:-unknown} in
        yes)
            printf '755\t%s\t%ssatl-term\n' "$repo/satl-term" "$bin_rel"
            printf '644\t%s\tshare/applications/org.satellite.terminal.desktop\n' \
                "$here/icons/org.satellite.terminal.desktop"
            ;;
        undecided)
            # A dry run on a tree that has not been built. The bracket is the
            # same device 050-building.sh uses for satl: it names a file the
            # transcript cannot yet be sure of, and the check below skips any
            # source with a bracket in it rather than reporting it missing.
            printf '755\t%s\t%ssatl-term\n' \
                "$repo/satl-term[ if gtk4 is present ]" "$bin_rel"
            printf '644\t%s\tshare/applications/org.satellite.terminal.desktop\n' \
                "$here/icons/org.satellite.terminal.desktop"
            ;;
        *)
            # no, or unknown-on-an-uninstall. Nothing is printed on an install
            # with no window; an uninstall prints both so it can remove them.
            # A `return` here instead of an `if` would drop the mime packet and
            # the icons from the rest of this function, which is the one way a
            # gate inside a list of files can go badly wrong.
            if [ "$action" = uninstall ]; then
                printf '755\t%s\t%ssatl-term\n' "$repo/satl-term" "$bin_rel"
                printf '644\t%s\tshare/applications/org.satellite.terminal.desktop\n' \
                    "$here/icons/org.satellite.terminal.desktop"
            fi
            ;;
    esac

    printf '644\t%s\tshare/mime/packages/application-x-satellite.xml\n' \
        "$here/icons/application-x-satellite.xml"

    for _size in $icon_sizes; do
        printf '644\t%s\tshare/icons/hicolor/%s/apps/org.satellite.terminal.png\n' \
            "$here/icons/hicolor/$_size/apps/org.satellite.terminal.png" "$_size"
        printf '644\t%s\tshare/icons/hicolor/%s/mimetypes/application-x-satellite.png\n' \
            "$here/icons/hicolor/$_size/mimetypes/application-x-satellite.png" "$_size"
    done
}

# Every directory a destination needs, and every directory above it, deepest
# first. Used to make them on the way in and to take them away on the way out.
tree_dirs() {
    install_tree | while IFS=$_TAB read -r _mode _src _dst; do
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
    while IFS=$_TAB read -r _mode _src _dst; do
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

    while IFS=$_TAB read -r _mode _src _dst; do
        run install -m "$_mode" "$_src" "$root/$_dst"
    done <<EOF
$(install_tree)
EOF

    # THE LAUNCHER'S Exec= IS MADE ABSOLUTE, HERE AND ONLY HERE.
    #
    # icons/org.satellite.terminal.desktop ships with `Exec=satl-term %f` and
    # `TryExec=satl-term`, bare names, and the comment above that file is right
    # about why: a file sitting in the repo cannot know which prefix it will be
    # installed into. THIS script does know, at exactly this moment, so this is
    # where the two facts meet.
    #
    # WHAT IT FIXES. A desktop shell reads TryExec and hides the entry when the
    # name is not on the PATH the SESSION has -- which is not the PATH your
    # terminal has, and is not affected by anything in ~/.bashrc. Until now that
    # made --desktop silently depend on --link: ask for the launcher without the
    # symlink and you got a menu entry that hid itself, with no error anywhere,
    # which 080-report.sh had to carry a whole paragraph warning about. An
    # absolute path removes the dependency instead of documenting it, and the
    # two flags are now independent.
    #
    # SAFE FOR satl-term'S OWN LOOKUP. src/programs/terminal.cpp finds its
    # interpreter through /proc/self/exe rather than through argv[0] or PATH, so
    # naming the binary by an absolute path here changes nothing about which
    # satl it spawns -- it still takes the one sitting beside itself.
    #
    # Generated rather than copied, so the installed file is deliberately NOT
    # byte-identical to the source. --uninstall removes it by name and does not
    # care what is inside it.
    # QUOTED AND ESCAPED, BECAUSE Exec= IS NOT A PATH FIELD. The desktop entry
    # spec has it parsed into words the way a shell would, so an unquoted
    # /home/me/my satl/satl-term is two arguments and the entry silently stops
    # working -- the same disappearing-menu-entry this absolute path was written
    # to prevent, reintroduced for anyone whose home has a space in it. `%` is
    # worse than that: it is the field-code introducer, so a path containing one
    # loads without complaint and does nothing when clicked. Both are escaped
    # here. 030-arguments.sh already refuses to assume paths are tidy, and this
    # is the same care applied to the one file this script generates.
    #
    # TryExec IS DELIBERATELY LEFT BARE. It is a path field, not a command line:
    # a desktop shell stats it rather than word-splitting it, so quotes there
    # become part of the filename it looks for and the entry hides itself again.
    # The two fields take opposite treatment and that is not a slip.
    #
    # Written with a read loop rather than sed, because the replacement text is
    # a path chosen by the caller: an & in it is sed's "the whole match" and a |
    # is the delimiter, so a sed one-liner here would need escaping of its own
    # on top of the spec's, and would still be wrong for some path.
    absolutise_launcher() {
        _f=$1
        _q=$(printf '%s' "$installed_term" |
             sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/`/\\`/g' \
                 -e 's/\$/\\$/g' -e 's/%/%%/g')
        while IFS= read -r _line || [ -n "$_line" ]; do
            case $_line in
                Exec=satl-term*)
                    printf 'Exec="%s"%s\n' "$_q" "${_line#Exec=satl-term}" ;;
                TryExec=satl-term)
                    printf 'TryExec=%s\n' "$installed_term" ;;
                *)  printf '%s\n' "$_line" ;;
            esac
        done < "$_f" > "$_f.tmp" && mv -- "$_f.tmp" "$_f"
    }

    case ${have_term:-unknown} in
        yes | undecided)
            _launcher=$root/share/applications/org.satellite.terminal.desktop
            if [ "$dry_run" = yes ]; then
                show sed -i "s|^Exec=satl-term|Exec=\"$installed_term\"|" "$_launcher"
            else
                absolutise_launcher "$_launcher"
            fi
            ;;
    esac
fi

if [ "$action" = uninstall ]; then
    # No heading here: 080-report.sh announces the removal, and announcing it
    # twice reads as two things having happened.
    while IFS=$_TAB read -r _mode _src _dst; do
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
