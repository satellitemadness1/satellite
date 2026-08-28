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
# Whether this run should be linking the window and its launcher. Yes when the
# build made one; yes on an uninstall, which builds nothing and therefore never
# learns, and where ours() checks every path before it is touched so naming one
# that was never linked removes nothing. 060-install-tree.sh gates its own list
# the same way and for the same reason.
term_linkable() {
    [ "${have_term:-unknown}" = yes ] || [ "$action" = uninstall ]
}

desktop_tree() {
    if [ "$link_bin" = yes ]; then
        printf '%s %s\n' "$root/satl" "$user_bin/satl"

        # satl-term TOO, AND IT IS NOT A CONVENIENCE. The launcher installed
        # below has `Exec=satl-term` and `TryExec=satl-term` -- bare command
        # names, because the entry is written once and cannot know a prefix --
        # so a desktop shell finds the window only if that name is on PATH.
        # Without this link the entry installs, hides itself, and looks like a
        # broken install rather than an incomplete one.
        #
        # The link is safe for the reason terminal.cpp is written the way it
        # is: /proc/self/exe resolves the symlink, so the window still finds
        # the `satl` sitting beside the real binary in $root rather than
        # whatever else is in ~/.local/bin.
        if term_linkable; then
            printf '%s %s\n' "$root/satl-term" "$user_bin/satl-term"
        fi
    fi

    # satl-cpu-level IS DELIBERATELY NOT LINKED. It is installed -- it is how
    # the choice this script made can be checked afterwards -- but it is a
    # question about the machine, not a command anybody types by habit, and
    # ~/.local/bin is a directory of things a person means to run.

    if [ "$desktop" = yes ]; then
        printf '%s %s\n' \
            "$root/share/mime/packages/application-x-satellite.xml" \
            "$xdg_data/mime/packages/application-x-satellite.xml"

        # THE LAUNCHER, which is the whole reason --desktop is worth asking for
        # once there is a window: the mime packet gives a .satl file its icon
        # and its type, and this is what gives a file manager something to open
        # it WITH. The file name is the GApplication id window.cpp registers,
        # and it has to stay that -- the entry, the icon and StartupWMClass all
        # agree on org.satellite.terminal so that a shell finding any one of
        # them finds the others.
        if term_linkable; then
            printf '%s %s\n' \
                "$root/share/applications/org.satellite.terminal.desktop" \
                "$xdg_data/applications/org.satellite.terminal.desktop"
        fi

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
#
# THE DANGLING CASE IS THE NORMAL CASE ON AN UNINSTALL, and missing it made
# --uninstall leave every symlink it had created. Found 2026-08-28 by running
# --link --desktop and then --uninstall against a scratch HOME, which is a path
# this machine never took: here the first satellite owns those names, so every
# one of them is refused on the way in and there was never a link of ours to
# remove on the way out.
#
# install.sh sources 060 before 070, so by the time this runs the install tree
# has already been removed -- and `readlink -f` cannot canonicalise a path whose
# parent directories are gone, so it fails and every link looked like somebody
# else's. Falling back to the LITERAL target is exactly right and is not a
# loosening: the literal is what this script wrote into the link, and a target
# that cannot be resolved cannot be a live file belonging to anything else.
ours() {
    [ -L "$1" ] || return 1
    _t=$(readlink -f -- "$1" 2>/dev/null) || _t=
    if [ -z "$_t" ]; then
        _t=$(readlink -- "$1" 2>/dev/null) || return 1
    fi
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
        # THE THIRD INDEX, new with the launcher. A .desktop file dropped into
        # applications/ is found by a menu on its own, but the MimeType= line is
        # only consulted through mimeinfo.cache, which this builds -- so without
        # it the entry appears in the menu and a .satl file still has nothing
        # offered to open it, which is the half of the install that would look
        # like it worked.
        if command -v update-desktop-database >/dev/null 2>&1; then
            run update-desktop-database "$xdg_data/applications"
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
    for _d in "$xdg_data/mime/packages" "$xdg_data/icons/hicolor" \
              "$xdg_data/applications"; do
        [ -d "$_d" ] && run rmdir "$_d" 2>/dev/null || :
    done
    :
fi
