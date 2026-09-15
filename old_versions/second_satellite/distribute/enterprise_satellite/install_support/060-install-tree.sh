# satellite -- WHAT gets installed. This is the declaration; everything else in
# this installer is machinery around it.
#
# ONE FUNCTION EMITS THE WHOLE TREE and both directions read it. install walks
# it forwards and uninstall walks it backwards, so a path that is added here is
# added to both, and there is no second list to forget. The other two installers
# in this tree are built the same way and for the same reason.
#
# EACH LINE IS "source<TAB>destination<TAB>mode<TAB>how".
#
# TAB AND NOT SPACE, because a destination path may contain a space --
# /run/media/mom/My USB/ is an ordinary directory name -- and `read a b c` on a
# space-separated line would split it. A tab may also appear in a filename in
# principle, which is why this is a note about a tradeoff and not a claim to be
# airtight; nothing this script installs has one.
_TAB=$(printf '\t')

# "how" IS EITHER copy OR launcher, and there is exactly one launcher. Both are
# installed and both are removed; they differ only in that the launcher's
# contents are edited on the way in, by rewrite_launcher() below. It is in this
# list rather than off in 070-desktop.sh with the rest of the desktop work
# SPECIFICALLY so that --uninstall removes it: a path that is installed
# somewhere other than the declaration is a path the declaration cannot remove.
install_tree() {
    # THE INTERPRETER, under the name satl whichever build was chosen.
    # 050-payload.sh set $satl_source.
    printf '%s\t%s\t%s\t%s\n' "$satl_source" "$bindir/satl" 755 copy

    # THE DETECTOR. Installed, not just used and discarded, so that the choice
    # this script made can be checked afterwards by the person it was made for.
    printf '%s\t%s\t%s\t%s\n' \
        "$payload_programs/satl-cpu-level" "$bindir/satl-cpu-level" 755 copy

    # THE WINDOW, AND IT MUST LAND IN THE SAME DIRECTORY AS satl. That is a
    # requirement and not tidiness: ../../src/programs/terminal.cpp reads
    # /proc/self/exe and spawns the `satl` sitting NEXT TO ITSELF rather than
    # the one PATH finds, and ../../src/programs/window_handover.cpp does the
    # mirror image when satl finds it has no console. Split them across two
    # directories and each looks for the other where it is not.
    #
    # Gated on the library check in 040, and on the uninstall it is always
    # named: an uninstall does no checking, and remove_file() is silent about a
    # path that was never installed.
    if [ "$have_term" = yes ] || [ "$have_term" = unchecked ] ||
       [ "$action" = uninstall ]; then
        printf '%s\t%s\t%s\t%s\n' \
            "$payload_programs/satl-term" "$bindir/satl-term" 755 copy

        # THE LAUNCHER -- the file that puts satellite in the applications grid,
        # which is the reason this package exists. It goes with the window and
        # not with the interpreter: satl is a command-line program with no
        # window of its own, and a launcher for it would open something with
        # nowhere to appear.
        printf '%s\t%s\t%s\t%s\n' \
            "$payload_share/applications/$app_id.desktop" \
            "$datadir/applications/$app_id.desktop" 644 launcher
    fi

    # THE .satl FILE TYPE. Installed whether or not the window is, because the
    # type and its icon are worth having on their own -- a .satl file is
    # recognised, described and drawn correctly even on a machine with nothing
    # to open it in a window.
    printf '%s\t%s\t%s\t%s\n' \
        "$payload_share/mime/packages/$mime_name.xml" \
        "$datadir/mime/packages/$mime_name.xml" 644 copy

    # THE ARTWORK. Two icons at nine sizes: the application icon, named after
    # the GApplication id so that the launcher, the window and the icon all
    # agree; and the file-type icon, named after the mime type with the '/'
    # replaced by '-' because that is the name a file manager constructs on its
    # own. 010-defaults.sh carries the argument for both names.
    for _size in $icon_sizes; do
        printf '%s\t%s\t%s\t%s\n' \
            "$payload_share/icons/hicolor/$_size/apps/$app_id.png" \
            "$datadir/icons/hicolor/$_size/apps/$app_id.png" 644 copy
        printf '%s\t%s\t%s\t%s\n' \
            "$payload_share/icons/hicolor/$_size/mimetypes/$mime_name.png" \
            "$datadir/icons/hicolor/$_size/mimetypes/$mime_name.png" 644 copy
    done

    # THE EXAMPLE PROGRAMS, which are here because of who this package is for.
    # Someone learning the language needs something to open, and a file type
    # with no file of that type on the machine is a registration nobody ever
    # sees. share/satellite/example is where a prefix keeps a program's own
    # data, so these are removed by --uninstall along with everything else.
    #
    # GLOBBED AND NOT LISTED, unlike everything above. The artwork has a fixed
    # shape that a missing file would break; the examples are a directory whose
    # contents are expected to change every milestone, and a list here would be
    # a second place to update and forget. The `[ -e ]` guard is what makes an
    # unmatched glob emit nothing instead of emitting the pattern.
    if [ -d "$payload_example" ]; then
        for _f in "$payload_example"/*; do
            [ -e "$_f" ] || continue
            printf '%s\t%s\t%s\t%s\n' \
                "$_f" "$datadir/satellite/example/$(basename -- "$_f")" 644 copy
        done
    fi
    :
}

# THE DIRECTORIES THIS INSTALL MAY HAVE CREATED, deepest first, for the
# uninstall to try to remove.
#
# TRY, not remove: every one of these goes through remove_dir_if_empty(), which
# is rmdir, which refuses a directory that still holds something. So
# ~/.local/bin full of other programs survives and ~/.local/share/satellite,
# which nothing else uses, goes. There is no ownership test to get wrong because
# rmdir is the test.
#
# DEEPEST FIRST because rmdir removes one level at a time: share/satellite must
# go before share, and each icon size before icons/hicolor.
tree_dirs() {
    printf '%s\n' "$datadir/satellite/example"
    printf '%s\n' "$datadir/satellite"
    printf '%s\n' "$datadir/applications"
    printf '%s\n' "$datadir/mime/packages"
    printf '%s\n' "$datadir/mime"
    for _size in $icon_sizes; do
        printf '%s\n' "$datadir/icons/hicolor/$_size/apps"
        printf '%s\n' "$datadir/icons/hicolor/$_size/mimetypes"
        printf '%s\n' "$datadir/icons/hicolor/$_size"
    done
    printf '%s\n' "$datadir/icons/hicolor"
    printf '%s\n' "$datadir/icons"
    printf '%s\n' "$bindir"
    printf '%s\n' "$datadir"
}

# rewrite_launcher() -- copy the .desktop file with Exec and TryExec pointing at
# the absolute path this install actually used.
#
# THE SHIPPED FILE SAYS `Exec=satl-term %f`, a bare command name, and its own
# comments explain why: the prefix is not known when that file is WRITTEN. It is
# known here, which is what an installer is for, so this is not a departure from
# that file's reasoning but the other end of it.
#
# WHY IT MATTERS ON THIS OPERATING SYSTEM SPECIFICALLY. A bare command name in a
# launcher resolves through the PATH of the DESKTOP SESSION, which is not the
# PATH of a login shell. Measured on AlmaLinux 10.2, 2026-09-08: `systemd-path
# search-binaries-default` is /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin,
# and nothing in /etc/profile or /etc/profile.d adds ~/.local/bin -- the stock
# ~/.bashrc does, and a desktop shell launching a .desktop entry never reads
# .bashrc. So `Exec=satl-term` into a home-directory prefix is a bet on how the
# session happened to be started, and TryExec means losing the bet HIDES the
# entry: the apps grid shows nothing, with no error anywhere.
#
# A LOOP AND NOT sed, because the replacement is a path chosen at run time and
# sed would need every one of / & \ and the delimiter escaped out of it. A path
# with a '&' in it is unusual and a path under a home directory with an unusual
# character in it is exactly the case that should not silently produce a
# corrupt launcher.
rewrite_launcher() {
    _src=$1
    _dst=$2
    _exec=$bindir/satl-term

    # THE DESKTOP ENTRY SPEC RESERVES A LIST OF CHARACTERS in Exec and requires
    # a quoted argument when one appears. TryExec is a plain path and takes no
    # quoting, which is why only one of the two lines below is guarded.
    case $_exec in
        *[\ \"\'\\\>\<\~\|\&\;\$\*\?\#\(\)\`]*)
            _quoted_exec=$(printf '%s' "$_exec" | sed 's/[\\"$`]/\\&/g')
            _quoted_exec="\"$_quoted_exec\""
            ;;
        *)
            _quoted_exec=$_exec
            ;;
    esac

    # THE PARENT FIRST, and this is here rather than inherited because this
    # function does NOT go through install_file() -- it writes the destination
    # itself, so it owns every step install_file() would have done. Leaving it
    # out made the redirection below fail with "No such file or directory" on a
    # prefix that did not already have an applications/ directory, which is
    # every first install.
    _dir=$(dirname -- "$_dst")
    [ -d "$_dir" ] || run mkdir -p "$_dir"

    if [ "$dry_run" = yes ]; then
        printf '   %s\n' "$(quoted cp "$_src" "$_dst") ${_TAB}# with Exec=$_exec"
        return 0
    fi

    # `|| [ -n "$_line" ]` so that a final line with no newline is not dropped.
    # THE FIELD CODE IS AN ARGUMENT TO printf AND NOT PART OF THE FORMAT: %f in
    # a format string is a floating-point conversion, and passing it as data is
    # what makes it come out as the two characters the spec means.
    while IFS= read -r _line || [ -n "$_line" ]; do
        case $_line in
            'Exec='*)    printf '%s %s\n' "Exec=$_quoted_exec" '%f' ;;
            'TryExec='*) printf 'TryExec=%s\n' "$_exec" ;;
            *)           printf '%s\n' "$_line" ;;
        esac
    done < "$_src" > "$_dst"
    chmod 644 -- "$_dst"
}

# ---------------------------------------------------------------------------
# THE WALK. Forwards to install, backwards to remove.

installed_count=0
removed_count=0

if [ "$action" = install ]; then
    install_tree | while IFS=$_TAB read -r _src _dst _mode _how; do
        [ -n "$_src" ] || continue
        case $_how in
            launcher) rewrite_launcher "$_src" "$_dst" ;;
            *)        install_file "$_src" "$_dst" "$_mode" ;;
        esac
    done

    # COUNTED IN A SECOND PASS, and this looks wasteful until you notice that
    # the loop above runs in a SUBSHELL -- it is the right-hand side of a pipe --
    # so a counter incremented inside it is discarded when that subshell exits.
    # This is the classic POSIX sh trap and the choice is between a temporary
    # file, a here-document, and counting the declaration again. Counting it
    # again is free: install_tree() is pure, it reads nothing it did not compute,
    # and it produces the same lines twice.
    installed_count=$(install_tree | grep -c . || :)
else
    install_tree | while IFS=$_TAB read -r _src _dst _mode _how; do
        [ -n "$_dst" ] || continue
        remove_file "$_dst"
    done
    removed_count=$(install_tree | grep -c . || :)

    # THE DIRECTORIES, after the files, deepest first. 070-desktop.sh runs
    # AFTER this and rebuilds the indexes over whatever is left, which is why
    # rebuild_data_indexes() there checks that each directory still exists
    # before running a tool against it.
    tree_dirs | while IFS= read -r _d; do
        remove_dir_if_empty "$_d"
    done
fi
