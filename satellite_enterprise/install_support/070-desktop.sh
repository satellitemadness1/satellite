# satellite -- the two optional pieces that live outside the root.
#
# ~/.local/bin/satl, so that typing `satl` finds it, and the icons and .satl
# file type under ~/.local/share, which is the only place a desktop looks. Both
# are off unless asked for; 010-defaults.sh says why, with the measurement.
#
# SYMLINKS AND NOT COPIES, and on this machine that is a correctness decision
# rather than a tidy one. The first satellite's installer put REAL FILES at
# every one of these paths and they are still there. A symlink is therefore
# what makes ownership unambiguous: this script creates only symlinks, so
# --uninstall can remove exactly the things that resolve back into $root and
# leave every real file alone. A copy would be indistinguishable from the copy
# that was already there, and removing it would be a guess.

xdg_data=${XDG_DATA_HOME:-$HOME/.local/share}

# ~/.local/bin is not in the XDG basedir spec, which defines no directory for
# executables. It is the de-facto standard anyway -- systemd's user session puts
# it on PATH, and it is already on this machine's -- which is why it is spelled
# out here rather than read from a variable that does not exist.
user_bin=$HOME/.local/bin

# Lines of "link-target destination". The targets are inside $root, so these
# point at what was just installed rather than at the source tree, and an
# install that later moves is a re-run rather than a set of dangling links.
desktop_tree() {
    [ "$link_bin" = yes ] && printf '%s %s\n' "$root/satl" "$user_bin/satl"
    if [ "$desktop" = yes ]; then
        printf '%s %s\n' \
            "$root/share/mime/packages/application-x-satellite.xml" \
            "$xdg_data/mime/packages/application-x-satellite.xml"
        for _size in $icon_sizes; do
            printf '%s %s\n' \
                "$root/share/icons/hicolor/$_size/apps/org.satellite.terminal.png" \
                "$xdg_data/icons/hicolor/$_size/apps/org.satellite.terminal.png"
            printf '%s %s\n' \
                "$root/share/icons/hicolor/$_size/mimetypes/application-x-satellite.png" \
                "$xdg_data/icons/hicolor/$_size/mimetypes/application-x-satellite.png"
        done
    fi
    :
}

# Whether a path is a symlink this script would have made: a link, resolving
# somewhere inside $root. Anything else -- a real file, or a link pointing
# elsewhere -- belongs to something that is not us.
ours() {
    [ -L "$1" ] || return 1
    _t=$(readlink -f -- "$1" 2>/dev/null) || return 1
    case $_t in
        "$root"/*) return 0 ;;
        *) return 1 ;;
    esac
}

# Rebuild the two indexes a desktop reads. Both are best-effort: a machine with
# no desktop installed has neither tool, and that is not a failed install.
refresh_indexes() {
    if [ "$desktop" = yes ]; then
        if command -v update-mime-database >/dev/null 2>&1; then
            run update-mime-database "$xdg_data/mime"
        fi
        if command -v gtk-update-icon-cache >/dev/null 2>&1; then
            run gtk-update-icon-cache -qtf "$xdg_data/icons/hicolor"
        fi
    fi
}

# Set for 080-report.sh: paths that were refused because something else owns
# them, and paths that were left alone because they were already correct.
occupied=
linked=
desktop_removed=no

# Counted rather than listed on an uninstall. Every path in desktop_tree() that
# this script did not create is left alone -- that is the contract, not news --
# and on a machine with the first satellite installed there are twenty-one of
# them, which as a list is a screen of alarming output describing nothing
# happening. On an INSTALL the same fact is news, because the user asked for
# --link or --desktop and did not get it, so there it stays a list.
left_alone=0

if [ "$action" = install ] && { [ "$link_bin" = yes ] || [ "$desktop" = yes ]; }; then
    step "linking into $HOME/.local"

    while read -r _target _dest; do
        [ -n "$_dest" ] || continue

        # ALREADY OURS AND ALREADY RIGHT: nothing to do, and saying "installed"
        # about a link that was already there would be a small lie.
        if ours "$_dest" && [ "$(readlink -- "$_dest")" = "$_target" ]; then
            continue
        fi

        # SOMEBODY ELSE'S FILE. Refused rather than replaced, and collected for
        # the report so that the refusal is one visible list at the end instead
        # of a line per file scrolling past. On this machine the first
        # satellite's install owns every one of these paths.
        if [ -e "$_dest" ] && ! ours "$_dest"; then
            occupied="$occupied$_dest
"
            continue
        fi

        run mkdir -p "$(dirname -- "$_dest")"
        # -f to replace one of our own older links, -n so that a link pointing
        # at a DIRECTORY is replaced rather than followed into it, which is the
        # difference between updating a link and creating one inside the tree
        # it used to name.
        run ln -sfn "$_target" "$_dest"
        linked="$linked$_dest
"
    done <<EOF
$(desktop_tree)
EOF

    # Only when something actually changed. These two rebuild indexes covering
    # every application on the machine, and running them because a link was
    # REFUSED would be doing work on behalf of a change that did not happen.
    [ -n "$linked" ] && refresh_indexes
    :
fi

if [ "$action" = uninstall ]; then
    # An uninstall removes these whether or not this run was told to make them,
    # because a previous run may have. Which is safe for exactly one reason:
    # ours() checks before every removal, so a path holding anything but one of
    # our own symlinks is left where it is.
    link_bin=yes
    desktop=yes
    _removed_any=no

    while read -r _target _dest; do
        [ -n "$_dest" ] || continue
        if ours "$_dest"; then
            run rm -f "$_dest"
            _removed_any=yes
        elif [ -e "$_dest" ]; then
            left_alone=$((left_alone + 1))
        fi
    done <<EOF
$(desktop_tree)
EOF

    # Read by 080-report.sh. Whether anything of OURS was under ~/.local decides
    # whether the report mentions ~/.local at all: on a machine where this
    # script never linked anything, the twenty paths another install owns are
    # not news, they are just somebody else's files being correctly ignored.
    desktop_removed=$_removed_any

    [ "$_removed_any" = yes ] && refresh_indexes

    # Only the directories this script could have created, and only when empty.
    # ~/.local/bin and ~/.local/share belong to the user and are never touched.
    for _d in "$xdg_data/mime/packages" "$xdg_data/icons/hicolor"; do
        [ -d "$_d" ] && run rmdir "$_d" 2>/dev/null || :
    done
    :
fi
