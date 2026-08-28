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
if [ -x "$repo/satl-cpu-level" ] || [ -x "$root/satl-cpu-level" ]; then
    printf '    %s\n' "$root/satl-cpu-level"
fi
case ${have_term:-unknown} in
    yes)       printf '    %s\n' "$root/satl-term"
               printf '    %s\n' \
                   "$root/share/applications/org.satellite.terminal.desktop" ;;
    undecided) printf '    %s   (if gtk4 and vte are present)\n' "$root/satl-term" ;;
esac
printf '    %s\n' "$root/share/mime/packages/application-x-satellite.xml"
printf '    %s   (nine sizes, apps and mimetypes)\n' "$root/share/icons/hicolor"

# SAID WHENEVER IT IS TRUE, and not only when somebody asks. A machine with no
# gtk4 gets a complete install of the interpreter and no window, and the
# difference between that and a broken install is one line of output.
if [ "$have_term" = no ]; then
    cat <<EOF
install.sh: note -- satl-term was not built, so the window and its launcher
            were not installed. The interpreter is unaffected. To get one:
              AlmaLinux/RHEL: dnf --enablerepo=crb install vte291-gtk4-devel
              Debian/Ubuntu:  apt install libvte-2.91-gtk4-dev
            then re-run this script.
EOF
fi

# satl.haswell IS NOT MISSING. Said here rather than left to be noticed, because
# a person who has just watched `make` produce four files and an installer
# announce three has a reasonable question, and the answer is a design decision
# rather than an omission -- PLAN.md sec 4.2.
if [ "$variant" = haswell ] || [ "$variant" = baseline ]; then
    if [ "$have_term" = yes ]; then
        printf 'install.sh: the build made four files and this installed three\n'
        printf '            programs: satl.haswell and satl are one program\n'
        printf '            compiled twice, and the %s one went in as satl.\n' \
            "$variant"
    else
        printf 'install.sh: satl.haswell is not missing -- it and satl are one\n'
        printf '            program compiled twice, and the %s one went in\n' \
            "$variant"
        printf '            as satl. Only one of the pair is ever installed.\n'
    fi
fi

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
    printf 'install.sh: checking the installed binaries by running them\n'
    if "$root/satl" --version; then
        :
    else
        die "$root/satl was installed but would not run.
       If this CPU was told it could run the haswell build and could not, that
       is the one bug this check exists to catch: re-run with
       $(quoted "$self") and report what $repo/satl-cpu-level --explain says."
    fi

    # THE WINDOW IS CHECKED THE SAME WAY AND WITHOUT OPENING ONE.
    # src/programs/window.cpp answers --version before it touches GTK, on
    # purpose -- "works over ssh and in a package build the same way `satl
    # --version` does" -- so this check runs on a headless box and in a
    # container, which is where an install is most likely to be rehearsed.
    #
    # It is a real check and not a formality: satl-term is the one binary here
    # that links anything outside libc and libstdc++, so it is the one whose
    # copy can arrive at a machine that cannot load its libraries.
    if [ "$have_term" = yes ]; then
        if "$root/satl-term" --version; then
            :
        else
            die "$root/satl-term was installed but would not run.
       It answers --version without opening a window, so a failure here is the
       loader, not the display: run ldd on it and look for gtk4 or
       vte-2.91-gtk4. The interpreter at $root/satl is unaffected and was
       checked first."
        fi
    fi

    # satl-cpu-level IS NOT RE-RUN. 050-building.sh already ran it, from the
    # build tree, and its answer is what chose the file that was just verified
    # above -- running the installed copy again would be asking the same
    # question a second time and would prove nothing the line above has not.
else
    printf 'install.sh: a real run would now execute %s --version, which is\n' \
        "$root/satl"
    printf '            what proves the chosen build actually runs on this CPU,\n'
    printf '            and %s --version, which answers\n' "$root/satl-term"
    printf '            without opening a window and so proves its libraries load.\n'
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

# THE LAUNCHER NEEDS THE LINK, and a desktop install without one is the single
# way this script can leave something that looks broken rather than absent: the
# entry is installed, TryExec cannot find satl-term on PATH, and the menu shows
# nothing at all with no error anywhere.
if [ "$desktop" = yes ] && [ "$link_bin" = no ] && term_linkable; then
    cat <<EOF

install.sh: note -- --desktop was given without --link, so the launcher was
            installed and will hide itself: its Exec and TryExec name
            \`satl-term\`, which is not on your PATH. Re-run with both to
            finish it:

                $(quoted "$self") --link --desktop
EOF
fi

# Said last, because it is the thing a reader of the banner will want next and
# because it is still the honest headline: there are binaries, and none of them
# interprets anything yet.
printf '\n'
printf 'install.sh: this build is milestone 2 -- the trie and the path interner\n'
printf '            landed 2026-08-28, so satl knows every word in the language\n'
printf '            and can dump the numbering with --words. It still runs no\n'
printf '            program: the lexer is M3 and the first program runs at M8.A.\n'
if [ "$have_term" = yes ]; then
    printf '            satl-term opens, spawns the satl beside it, and shows you\n'
    printf '            what that satl says -- which today is --repl declining.\n'
fi
