# satellite -- make `satl` and `satl-term` work in any terminal window.
#
# THIS FRAGMENT EDITS ~/.bashrc, WHICH THE OTHER TWO INSTALLERS IN THIS TREE
# REFUSE TO DO. Their argument was sound and is worth restating rather than
# deleted: a child process cannot change its parent's environment, so an
# installer that "exports PATH" exports it into a shell that exits one line
# later; and editing a login file is a permanent change made by a program
# somebody ran once, which an uninstaller cannot reliably undo.
#
# IT IS OVERRIDDEN HERE AT THE AUTHOR'S INSTRUCTION, and the second half of that
# objection is answered rather than ignored. What this writes is a BLOCK BETWEEN
# TWO MARKERS, and --uninstall removes exactly the lines between them. That is
# what makes the change reversible, and it is the only reason this is allowed to
# be an edit rather than a suggestion:
#
#     # >>> satellite >>>
#     ...
#     # <<< satellite <<<
#
# IDEMPOTENT, WHICH MATTERS MORE THAN IT LOOKS. Running an installer twice is
# normal -- a reinstall after an upgrade, a second run after installing the
# libraries the first run asked for -- and the failure mode of the naive version
# is a .bashrc that grows a copy of this block every time and a PATH with the
# same directory in it nine times. An existing block is REMOVED and rewritten,
# so the file has exactly one however many times this runs.
#
# THE BLOCK ITSELF GUARDS AGAIN AT RUN TIME. Even one copy of it in .bashrc runs
# on every interactive shell, including a shell started from a shell, so it
# checks whether the directory is already on PATH before prepending it.
# Otherwise nesting three terminals deep would put it there three times.

# ~/.bashrc AND NOT ~/.bash_profile, because the question is "any console
# window". A terminal emulator starts an INTERACTIVE NON-LOGIN shell, which
# reads .bashrc and does not read .bash_profile. AlmaLinux's stock
# .bash_profile sources .bashrc anyway, so writing to .bashrc covers the login
# case too and writing to .bash_profile would not have covered this one.
shell_rc=$HOME/.bashrc
shell_path_state=skipped

# THE MARKERS. Long enough not to collide with anything else, and containing the
# word satellite so that somebody reading their own .bashrc a year from now can
# tell what put it there and what to search for.
_begin='# >>> satellite >>>'
_end='# <<< satellite <<<'

# strip_block() -- write $shell_rc back without the marked block, and without
# the blank lines left at the end where it was.
#
# awk AND NOT sed, because sed's range addressing would silently do the wrong
# thing on a file that somehow has a begin marker and no end marker: `/a/,/b/d`
# with no /b/ deletes to end of file. This flag-based version deletes nothing
# when the end marker never arrives, which is the safe direction to be wrong in
# on a file somebody else owns.
#
# THE TRAILING BLANK LINES ARE TRIMMED, and that is not tidiness -- it is what
# stops the file growing. The block is appended after a blank line, so the blank
# line is OUTSIDE the markers and survives the strip; install, uninstall,
# install would then leave one more empty line at the end of .bashrc every time,
# forever. Measured on 2026-09-08 after an install, a reinstall and an
# uninstall: three newlines where the original file had one.
#
# Trailing blank lines at the end of a shell startup file mean nothing to any
# shell, so removing them changes no behaviour. They are held in a counter and
# emitted only when a non-blank line follows, which is what makes blank lines in
# the MIDDLE of somebody's .bashrc survive untouched -- those are theirs.
strip_block() {
    awk -v b="$_begin" -v e="$_end" '
        $0 == b { inblock = 1 }
        !inblock {
            if ($0 ~ /^[[:space:]]*$/) { held++; next }
            while (held > 0) { print ""; held-- }
            print
        }
        $0 == e { inblock = 0 }
    ' "$1"
}

# WHETHER THE EDIT IS WANTED AT ALL. A --system install puts the programs in
# /usr/local/bin, which is on the default PATH of every shell on the machine
# before anything is read -- `systemd-path search-binaries-default` on AlmaLinux
# 10.2 is /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin. Writing a PATH line
# for a directory that is already on PATH would be noise in somebody's login
# file, which is the thing the rule this fragment overrides was protecting.
needs_path_edit=yes
case ":/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:" in
    *":$bindir:"*) needs_path_edit=no ;;
esac
[ "$edit_shell_rc" = yes ] || needs_path_edit=no

if [ "$action" = install ] && [ "$needs_path_edit" = yes ]; then
    if [ "$dry_run" = yes ]; then
        printf '   %s\n' "# add $bindir to PATH in $shell_rc"
        shell_path_state=dry-run
    else
        # A .bashrc THAT DOES NOT EXIST IS CREATED, which is the case on an
        # account whose shell is not bash and on a very bare home directory.
        # An empty file with our block in it is correct and harmless.
        [ -f "$shell_rc" ] || : > "$shell_rc"

        # THE BACKUP IS TAKEN ONCE AND NEVER OVERWRITTEN. A second install must
        # not replace the copy of the file as it was BEFORE satellite ever
        # touched it with a copy that already has our block in it -- that would
        # turn the safety net into a copy of the thing it is protecting against.
        if [ ! -f "$shell_rc.satellite-backup" ]; then
            run cp -- "$shell_rc" "$shell_rc.satellite-backup"
        fi

        # REMOVE THEN APPEND, via a temporary file beside the target so that the
        # rename at the end is on one filesystem and therefore atomic. A .bashrc
        # truncated by a full disk halfway through a rewrite is a login shell
        # that no longer works, which is a bad way to find out about a full
        # disk.
        _tmp=$shell_rc.satellite-new.$$
        strip_block "$shell_rc" > "$_tmp"

        # THE BLOCK. Quoted heredoc, so the $HOME and $PATH inside it are
        # written as text and expanded later by the shell that reads .bashrc,
        # rather than expanded now and frozen. $bindir is the one value that
        # must be baked in, so it is substituted by the printf below instead.
        {
            printf '\n%s\n' "$_begin"
            printf '# Added by satellite'\''s install-satellite.sh. Everything between these\n'
            printf '# two markers was written by that script and is removed by:\n'
            printf '#     install-satellite.sh --uninstall\n'
            printf '# It puts satl and satl-term on PATH so they run in any terminal.\n'
            printf '# The guard means opening a terminal inside a terminal does not add\n'
            printf '# the directory a second time.\n'
            printf 'case ":$PATH:" in\n'
            printf '    *":%s:"*) ;;\n' "$bindir"
            printf '    *) PATH="%s:$PATH" ;;\n' "$bindir"
            printf 'esac\n'
            printf 'export PATH\n'
            printf '%s\n' "$_end"
        } >> "$_tmp"

        mv -- "$_tmp" "$shell_rc"
        shell_path_state=written
    fi

elif [ "$action" = uninstall ]; then
    # REMOVED WHETHER OR NOT THIS RUN WOULD HAVE WRITTEN IT. An uninstall does
    # not know which flags the install was given, and a block left behind would
    # put a directory that no longer exists on PATH forever.
    if [ -f "$shell_rc" ] && grep -qF "$_begin" "$shell_rc" 2>/dev/null; then
        if [ "$dry_run" = yes ]; then
            printf '   %s\n' "# remove the satellite block from $shell_rc"
        else
            _tmp=$shell_rc.satellite-new.$$
            strip_block "$shell_rc" > "$_tmp"
            mv -- "$_tmp" "$shell_rc"
        fi
        shell_path_state=removed

        # THE BACKUP IS LEFT IN PLACE, deliberately. It is the file as it was
        # before satellite ever touched it, it is the only copy of that, and
        # removing it would be this script deleting a file it did not create
        # the contents of. 080-report.sh names it so it is not a mystery.
    fi
fi
