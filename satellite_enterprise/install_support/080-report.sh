# satellite -- what happened, and whether `satl` will actually be found.
#
# $did and $removed come from 030-arguments.sh, which derives them from
# --dry-run along with the installed paths this file verifies.

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
printf '    %s\n' "$installed_satl"
if [ -x "$repo/satl-cpu-level" ] || [ -x "$installed_cpu" ]; then
    printf '    %s\n' "$installed_cpu"
fi
case ${have_term:-unknown} in
    yes)       printf '    %s\n' "$installed_term"
               printf '    %s\n' \
                   "$root/share/applications/org.satellite.terminal.desktop" ;;
    undecided) printf '    %s   (if gtk4 and vte are present)\n' "$installed_term" ;;
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

# REFUSED, AND NAMED, WITHOUT GUESSING WHOSE THEY ARE. This used to say they
# must be the first satellite's, and on 2026-09-06 they were not: ~/.local/bin
# held plain copies of THIS tree's binaries, put there by hand. What matters is
# that this script did not create them, which is the one thing it knows.
if [ -n "$occupied" ]; then
    cat <<EOF
install.sh: note -- these paths already hold something this script did not
            create, so they were left exactly as they were:
EOF
    printf '%s' "$occupied" | while read -r _p; do
        [ -n "$_p" ] && printf '    %s\n' "$_p"
    done
    cat <<EOF
            They may be an older install, or copies of a satl put there by
            hand. Delete the ones you do not want and re-run this script, and
            it will link those names to $root.
            Nothing above was changed.
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
    if "$installed_satl" --version; then
        :
    else
        die "$installed_satl was installed but would not run.
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
        if "$installed_term" --version; then
            :
        else
            die "$installed_term was installed but would not run.
       It answers --version without opening a window, so a failure here is the
       loader, not the display: run ldd on it and look for gtk4 or
       vte-2.91-gtk4. The interpreter at $installed_satl is unaffected and was
       checked first."
        fi
    fi

    # satl-cpu-level IS NOT RE-RUN. 050-building.sh already ran it, from the
    # build tree, and its answer is what chose the file that was just verified
    # above -- running the installed copy again would be asking the same
    # question a second time and would prove nothing the line above has not.
else
    printf 'install.sh: a real run would now execute %s --version, which is\n' \
        "$installed_satl"
    printf '            what proves the chosen build actually runs on this CPU,\n'
    printf '            and %s --version, which answers\n' "$installed_term"
    printf '            without opening a window and so proves its libraries load.\n'
fi

# `satl` runs whichever copy the shell finds FIRST, and $root is not on PATH by
# design -- it is a directory of satellite's own, not a bin directory. So the
# question worth answering is not "is it on PATH" but "what does the word satl
# get you right now", which command -v answers for PATH. It cannot answer for an
# alias or a shell function, which live in the interactive shell that started
# this one and never reach a script -- so the closing lines below say so rather
# than let "runs the copy just installed" stand as the whole truth.
found=$(command -v satl 2>/dev/null || :)
printf '\n'
if [ -z "$found" ]; then
    cat <<EOF
install.sh: \`satl\` is not on your PATH. Run it in full:

                $(quoted "$installed_satl")

            or re-run this script with --link, which puts a symlink in
            ~/.local/bin (already on your PATH) and which --uninstall removes
            again, or with --system, which installs into /usr/local/bin, a
            directory every shell already searches.

            Nothing here edits .profile, .bashrc or any other file you own, and
            nothing here will suggest a line for you to add to one.
EOF
elif [ "$found" = "$installed_satl" ] || [ "$found" -ef "$installed_satl" ]; then
    printf 'install.sh: `satl` runs the copy just installed.\n'
elif [ -L "$user_bin/satl" ] && [ "$user_bin/satl" -ef "$installed_satl" ]; then
    # SHADOWED, WHICH IS NOT THE SAME FACT AS "NOT LINKED", and telling the two
    # apart is the whole reason this branch exists. Until 2026-08-28 both landed
    # in the else below, whose "nothing was done about it: this install put its
    # interpreter in $root and touched nothing else" is true when --link was
    # never asked for and a plain untruth here, where the link WAS made, points
    # at the right file, and is simply never reached. Reported by the author,
    # who had just taken the `satl` alias out of .bashrc and so met PATH order
    # for the first time: run --link, be told `satl` still runs something else,
    # read that the installer touched nothing, and the only available conclusion
    # is that the installer does not work. It does. PATH order does not care.
    #
    # The link is checked against the FILESYSTEM rather than against $linked,
    # which holds only what THIS run created: a correct link left by an earlier
    # run is the same fact for the reader and would otherwise report itself as
    # the missing-link case every second time the script was run.
    _shadow_dir=$(dirname -- "$found")

    printf 'install.sh: note -- `satl` runs %s, which is not the\n' "$found"
    printf '            copy just installed. The symlink is NOT the problem --\n\n'
    printf '                %s -> %s\n\n' "$user_bin/satl" "$installed_satl"
    printf '            is exactly right, and re-running this script cannot change\n'
    printf '            anything, because what decides is the order of your PATH:\n\n'
    case ":$PATH:" in
        *":$user_bin:"*)
            printf '                %s comes BEFORE %s\n\n' "$_shadow_dir" "$user_bin"
            printf '            so the shell stops at the first satl it finds and never\n'
            printf '            reaches ours.\n' ;;
        *)
            printf '                %s is not on your PATH at all\n\n' "$user_bin"
            printf '            so the link sitting in it is never consulted.\n' ;;
    esac

    # THE BUILD TREE ITSELF ON PATH IS NOT AN OLDER INSTALL, and until
    # 2026-09-13 this branch told the author to sudo-uninstall "the first
    # satellite" from /home/madness/code/cxx when the satl ahead of the link was
    # this tree's own build, the one the install had just been copied from.
    # Nothing there should be removed, so that case gets no remedy but PATH.
    if [ "$_shadow_dir" -ef "$repo" ]; then
        printf '\n            %s is the tree this install was built\n' "$_shadow_dir"
        printf '            from, so that satl is the same revision and needs no removing.\n'
        if [ "$variant" = haswell ]; then
            printf '            It is the baseline build, though, and the install is the\n'
            printf '            haswell one chosen for this CPU.\n'
        fi
        printf '            To reach the install, put %s ahead of\n' "$user_bin"
        printf '            %s in your PATH yourself.\n' "$_shadow_dir"
    else
        # THE REMEDY IS PRINTED AND NOT PERFORMED. Taking the older file out may
        # need root, and this script never calls sudo. The prefix is derived
        # from the shadowing file rather than assumed.
        printf '\n            This script will not touch %s: it is\n' "$found"
        printf '            not this install%ss file. Two ways out --\n' "'"
        printf '\n              1. remove the older install, using whatever put it there;\n'
        if [ -f "$repo/old_versions/first_satellite/install.sh" ]; then
            printf '                 if it is the first satellite, its own uninstaller:\n\n'
            _sudo=
            [ -w "$_shadow_dir" ] || _sudo='sudo '
            printf '                     %ssh %s \\\n' "$_sudo" \
                "$(quoted "$repo/old_versions/first_satellite/install.sh")"
            printf '                          --uninstall --prefix %s\n' \
                "$(quoted "$(dirname -- "$_shadow_dir")")"
        fi
        printf '\n              2. or put %s ahead of\n' "$user_bin"
        printf '                 %s in your PATH yourself. That is a\n' "$_shadow_dir"
        printf '                 change to a file you own, so it is yours to make and not\n'
        printf '                 this script%ss business.\n' "'"
    fi
    printf '\n            Afterwards run `hash -r`, or open a new shell:\n'
    printf '            this one has already remembered where satl was.\n'
else
    cat <<EOF
install.sh: note -- \`satl\` already runs $found,
            which is not the copy just installed, and nothing was done about
            it: this install put its interpreter in $root and
            linked nothing to it. To run the one just installed, spell it out:

                $(quoted "$installed_satl")

            If this shell has run satl already, run \`hash -r\` first.
EOF
fi

if [ -n "$found" ]; then
    printf '\ninstall.sh: an alias or shell function named satl beats PATH, and a\n'
    printf '            script cannot see yours: `type satl` in your shell says\n'
    printf '            what the word really runs.\n'
fi

# THE VERSION IS READ, NOT WRITTEN HERE. This paragraph used to name a milestone
# by hand and went stale twice: "milestone 2" through M3-M6, then "milestone 6
# ... it still runs no program" until 2026-09-13, long after M10 ran the first
# one. So the number now comes from the INSTALLED binary, which the check above
# already trusts as its own record, and from 020-version.mk in a dry run, where
# no binary has been copied yet -- the file that binary's number is compiled
# from. What follows it describes the command line, which milestones add to and
# have not yet taken anything away from.
if [ "$dry_run" = no ]; then
    release=$("$installed_satl" --version 2>/dev/null | sed -n '1s/^satl //p')
else
    _vf=$repo/make_support/020-version.mk
    _v=$(sed -n 's/^SATELLITE_VERSION[[:space:]]*?=[[:space:]]*//p' "$_vf" 2>/dev/null)
    _r=$(sed -n 's/^SATELLITE_REVISION[[:space:]]*?=[[:space:]]*//p' "$_vf" 2>/dev/null)
    release=${_v:+$_v revision $_r}
fi
printf '\n'
printf 'install.sh: this is satellite %s. `satl <file>` runs a program,\n' \
    "${release:-(no version could be read)}"
printf '            `satl --repl` opens the prompt, and `satl --help` lists the\n'
printf '            rest.\n'
if [ "$have_term" = yes ]; then
    printf '            `satl-term` is the same prompt in a window of its own, and\n'
    printf '            `satl-term <file>` runs a program in one.\n'
fi
