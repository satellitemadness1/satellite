# satellite -- what happened, and whether `satl` will actually be found.
#
# A dry run reports too, in the conditional: telling someone their files were
# installed when the whole point of -n was that they were not is the kind of
# small lie that costs somebody an hour later.

if [ "$dry_run" = yes ]; then
    did='would install'
    removed='would remove'
else
    did=installed
    removed=removed
fi

# Anything under the root that this script did not put there. 060-install-tree.sh
# removes files by name and directories with rmdir, so whatever is left is
# whatever was not ours -- and the point of not using rm -rf is that the user
# gets told about it rather than losing it.
leftovers() {
    [ -d "$root" ] || return 0
    find "$root" -mindepth 1 2>/dev/null | head -20
}

if [ "$action" = uninstall ]; then
    printf 'install.sh: %s the satellite install tree from %s\n' "$removed" "$root"

    if [ "$desktop_removed" = yes ] && [ "$left_alone" -gt 0 ]; then
        printf 'install.sh: %s other path(s) under %s were left alone,\n' \
            "$left_alone" "$HOME/.local"
        printf '            because this script did not create them.\n'
    fi

    _left=$(leftovers)
    if [ -n "$_left" ] && [ "$dry_run" = no ]; then
        printf 'install.sh: %s still exists, because it is not empty:\n' "$root"
        printf '%s\n' "$_left" | while read -r _p; do
            [ -n "$_p" ] && printf '    %s\n' "$_p"
        done
        printf '            Nothing here was installed by this script, so it was\n'
        printf '            not removed by it either. Delete it yourself if you\n'
        printf '            want the directory gone.\n'
    fi

    printf 'install.sh: nothing was ever added to your shell startup files,\n'
    printf '            so there is nothing left behind to take out of them.\n'
    exit 0
fi

printf 'install.sh: %s\n' "$did"
printf '    %s\n' "$root/satl"
printf '    %s\n' "$root/share/mime/packages/application-x-satellite.xml"
printf '    %s   (nine sizes, apps and mimetypes)\n' "$root/share/icons/hicolor"

if [ -n "$linked" ]; then
    printf 'install.sh: %s these symlinks, which point back into %s:\n' "$did" "$root"
    printf '%s' "$linked" | while read -r _p; do
        [ -n "$_p" ] && printf '    %s\n' "$_p"
    done
fi

# REFUSED, AND NAMED. This is the list the machine this was written on produces
# on every --link --desktop run, because the first satellite's installer owns
# all of those paths and this script will not overwrite what it did not create.
if [ -n "$occupied" ]; then
    cat <<EOF
install.sh: note -- these paths already hold something this script did not
            create, so they were left exactly as they were:
EOF
    printf '%s' "$occupied" | while read -r _p; do
        [ -n "$_p" ] && printf '    %s\n' "$_p"
    done
    cat <<EOF
            On a machine that has the first satellite installed, that is what
            these are: its interpreter and its artwork, installed by its own
            installer. Removing them is that install's business, not this
            one's. Nothing above was changed.
EOF
fi

# THE BINARY IS ITS OWN RECORD. This install writes no manifest saying which of
# the two builds it chose, and needs none: the flags line below was baked into
# that object at compile time by make_support/060-compile.mk. Running the
# INSTALLED file rather than the one in the build tree is the point -- it is the
# only check that the copy arrived, is executable, and does not fault on this
# CPU, which is the one way a wrong variant choice would show up.
if [ "$dry_run" = no ]; then
    printf 'install.sh: checking the installed binary by running it\n'
    if "$root/satl" --version; then
        :
    else
        die "$root/satl was installed but would not run.
       If this CPU was told it could run the haswell build and could not, that
       is the one bug this check exists to catch: re-run with
       $(quoted "$self") and report what $repo/satl-cpu-level --explain says."
    fi
else
    printf 'install.sh: a real run would now execute %s --version, which is\n' \
        "$root/satl"
    printf '            what proves the chosen build actually runs on this CPU.\n'
fi

# `satl` runs whichever copy the shell finds FIRST, and $root is not on PATH by
# design -- it is a directory of satellite's own, not a bin directory. So the
# question worth answering is not "is it on PATH" but "what does the word satl
# get you right now", which command -v answers exactly. On the machine this was
# written on the answer is the FIRST satellite, and saying so is more useful
# than any advice about PATH.
found=$(command -v satl 2>/dev/null || :)
printf '\n'
if [ -z "$found" ]; then
    cat <<EOF
install.sh: \`satl\` is not on your PATH. Run it in full:

                $(quoted "$root/satl")

            or re-run this script with --link, which puts a symlink in
            ~/.local/bin (already on your PATH) and which --uninstall removes
            again. Nothing here edits .profile, .bashrc or any other file you
            own; if you would rather do it yourself, this is the line:

                export PATH="$root:\$PATH"
EOF
elif [ "$found" = "$root/satl" ] || [ "$found" -ef "$root/satl" ]; then
    printf 'install.sh: `satl` runs the copy just installed.\n'
else
    cat <<EOF
install.sh: note -- \`satl\` already runs $found,
            which is not the copy just installed. That is expected on a machine
            with the first satellite on it, and nothing was done about it: this
            install put its interpreter in $root and touched
            nothing else. To run the one from this tree, spell it out:

                $(quoted "$root/satl")

            If this shell has run satl already, run \`hash -r\` first.
EOF
fi

# Said last, because it is the thing a reader of the banner will want next and
# because at M1 it is the honest headline: there is a binary, and it does not
# interpret anything yet.
printf '\n'
printf 'install.sh: this build is milestone 1 -- it says what it is and how a\n'
printf '            file will be run. Running one lands at M8.\n'
