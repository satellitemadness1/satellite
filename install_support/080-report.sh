# satellite -- what happened, and whether `satl` will actually be found.
#
# A dry run reports too, in the conditional: telling someone their files were
# installed when the whole point of -n was that they were not is the kind of
# small lie that costs somebody an hour later.
#
# Moved out of the 634-line install.sh on 2026-08-24, byte for byte.

# A dry run still reports, because a preview of where the tree lands and whether
# that is on PATH is most of what the flag is for. It reports in the
# conditional, though: telling someone their files were installed when the whole
# point of -n was that they were not is the kind of small lie that costs an hour
# of someone else's afternoon later.
if [ "$dry_run" = yes ]; then
    did='would install'
    removed='would remove'
    staged='would stage'
else
    did=installed
    removed=removed
    staged=staged
fi

if [ "$action" = uninstall ]; then
    printf 'install.sh: %s the satellite install tree from %s\n' \
        "$removed" "$prefix"
    printf 'install.sh: nothing was ever added to your shell startup files,\n'
    printf '            so there is nothing left behind to take out of them.\n'
    exit 0
fi

# Staging is not an installation, so none of the advice below applies to it: the
# binaries are not where these paths say they are, and will not be until the
# package that contains them is installed somewhere.
if [ -n "$destdir" ]; then
    printf 'install.sh: %s the tree under %s\n' "$staged" "$destdir$prefix"
    exit 0
fi

bindir=$prefix/bin
printf 'install.sh: %s\n' "$did"
printf '    %s\n' "$bindir/satl" "$bindir/satl-term"
printf '    %s\n' "$prefix/share/satellite/lib"

# Membership in PATH was the question this used to ask, and it is the wrong
# one: `satl` runs whichever copy the shell finds FIRST, so a bindir that is on
# PATH behind another directory that also holds a satl satisfies the case
# statement and changes nothing the user can see. On the machine this was
# written on that was not hypothetical -- /usr/local/bin was on PATH, the
# sentence below said so, and the satl that answered came from $HOME/.local/bin.
# `command -v` answers the question that was meant. -ef as well as a string
# compare, because a PATH entry can reach the same directory through a symlink,
# where the two spellings differ and the file does not.
found=$(command -v satl 2>/dev/null || :)
case ":${PATH:-}:" in
    *:"$bindir":*)
        if [ -n "$found" ] &&
           { [ "$found" = "$bindir/satl" ] || [ "$found" -ef "$bindir/satl" ]; }
        then
            printf 'install.sh: %s is on your PATH, so `satl` finds it.\n' \
                "$bindir"
        elif [ -z "$found" ]; then
            printf 'install.sh: note — %s is on your PATH but no `satl` is\n' \
                "$bindir"
            printf '            reachable through it. Check %s exists.\n' \
                "$bindir/satl"
        else
            cat <<EOF
install.sh: note — $bindir is on your PATH, but \`satl\` still runs
            $found, which comes from an earlier entry, so the copy
            just installed is shadowed. Remove the other one, or put $bindir
            ahead of it, or spell this one out in full. If this shell has run
            satl already, run \`hash -r\` as well.
EOF
        fi
        ;;
    *)
        cat <<EOF
install.sh: note — $bindir is not on your PATH, so typing \`satl\` will not
            find it yet. Nothing was changed for you: this installer does not
            edit .profile, .bashrc or any other file it did not install. If you
            want it on your PATH, add this line to your shell's startup file
            yourself:

                export PATH="$bindir:\$PATH"

            satellite does not need it. The interpreter finds its own library
            directory relative to its own binary, so \`$bindir/satl\` works
            correctly right now, spelled out in full, with an empty environment.
EOF
        ;;
esac
