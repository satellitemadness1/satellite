# satellite -- what happened, and whether `satl` will actually be found.
#
# $did and $removed come from 030-arguments.sh.

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

    printf 'install.sh: the build in %s was not touched.\n' "$build"
    printf '            %s -C %s clean removes it.\n' "$MAKE" "$here"
    printf 'install.sh: nothing was ever added to your shell startup files,\n'
    printf '            so there is nothing left behind to take out of them.\n'
    exit 0
fi

printf 'install.sh: %s\n' "$did"
printf '    %s\n' "$installed_satl"
if [ -x "$build/satl-cpu-level" ] || [ -x "$installed_cpu" ]; then
    printf '    %s\n' "$installed_cpu"
fi
case ${have_term:-unknown} in
    yes)       printf '    %s\n' "$installed_term"
               printf '    %s\n' \
                   "$root/share/applications/org.satellite.terminal.desktop" ;;
    undecided) printf '    %s   (if vte-2.91-gtk4 is present)\n' "$installed_term" ;;
esac
printf '    %s\n' "$root/share/mime/packages/application-x-satellite.xml"
printf '    %s   (nine sizes, apps and mimetypes)\n' "$root/share/icons/hicolor"

# SAID WHENEVER IT IS TRUE, and not only when somebody asks. A machine with no
# vte gets a complete install of the interpreter and no window, and the
# difference between that and a broken install is one line of output.
if [ "$have_term" = no ]; then
    cat <<EOF
install.sh: note -- satl-term was not built, so the window and its launcher
            were not installed. The interpreter is unaffected. To get one:

                sudo apt install libvte-2.91-gtk4-dev

            then re-run this script. On Ubuntu 22.04 there is no such package:
            vte gained its GTK4 build at 0.70 and jammy ships 0.68.
EOF
fi

# THE THREE INDEX TOOLS, named only when --desktop actually needed them.
# 070-desktop.sh collects the absent ones rather than warning three times.
if [ -n "$missing_desktop_tools" ]; then
    printf 'install.sh: note -- --desktop linked the artwork, but these were not\n'
    printf '            here to rebuild the indexes that make a desktop see it:\n'
    for _t in $missing_desktop_tools; do printf '                %s\n' "$_t"; done
    printf '            The files are in place; nothing has been told about them.\n'
    printf '                sudo apt install%s\n' \
        "$(printf '%s' "$missing_desktop_tools" | sed 's/[^ ]*/ &/g;s/^ *//;s/^/ /')"
    printf '            then log out and back in, or re-run this script.\n'
fi

# satl.haswell IS NOT MISSING. Said here rather than left to be noticed, because
# a person who has just watched `make` produce four files and an installer
# announce three has a reasonable question, and the answer is a design decision
# rather than an omission.
if [ "$variant" = haswell ] || [ "$variant" = baseline ]; then
    if [ "$have_term" = yes ]; then
        printf 'install.sh: the build made four files and this installed three\n'
        printf '            programs: satl.haswell and satl are one program\n'
        printf '            compiled twice, and the %s one went in as satl.\n' "$variant"
    else
        printf 'install.sh: satl.haswell is not missing -- it and satl are one\n'
        printf '            program compiled twice, and the %s one went in\n' "$variant"
        printf '            as satl. Only one of the pair is ever installed.\n'
    fi
fi

if [ -n "$linked" ]; then
    printf 'install.sh: %s these symlinks, which point back into %s:\n' "$did" "$root"
    printf '%s' "$linked" | while read -r _p; do
        [ -n "$_p" ] && printf '    %s\n' "$_p"
    done
fi

# REFUSED, AND NAMED. This script will not overwrite what it did not create.
if [ -n "$occupied" ]; then
    cat <<EOF
install.sh: note -- these paths already hold something this script did not
            create, so they were left exactly as they were:
EOF
    printf '%s' "$occupied" | while read -r _p; do
        [ -n "$_p" ] && printf '    %s\n' "$_p"
    done
    cat <<EOF
            Removing them is that install's business, not this one's. Nothing
            above was changed.
EOF
fi

# THE BINARY IS ITS OWN RECORD. This install writes no manifest saying which of
# the two builds it chose, and needs none: the flags line below was baked into
# that object at compile time. Running the INSTALLED file rather than the one in
# build/ is the point -- it is the only check that the copy arrived, is
# executable, and does not fault on this CPU, which is the one way a wrong
# variant choice would show up.
if [ "$dry_run" = no ]; then
    printf 'install.sh: checking the installed binaries by running them\n'
    if "$installed_satl" --version; then
        :
    else
        die "$installed_satl was installed but would not run.
       If this CPU was told it could run the haswell build and could not, that
       is the one bug this check exists to catch: re-run and report what
       $build/satl-cpu-level --explain says."
    fi

    # THE WINDOW IS CHECKED THE SAME WAY AND WITHOUT OPENING ONE.
    # ../src/programs/window.cpp answers --version before it touches GTK, on
    # purpose, so this check runs on a headless box and in a container. It is a
    # real check and not a formality: satl-term is the one binary here that
    # links anything outside libc and libstdc++, so it is the one whose copy can
    # arrive at a machine that cannot load its libraries.
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
else
    printf 'install.sh: a real run would now execute %s --version, which is\n' \
        "$installed_satl"
    printf '            what proves the chosen build actually runs on this CPU.\n'
fi

# `satl` runs whichever copy the shell finds FIRST, and $root is not on PATH by
# design -- it is a directory of satellite's own, not a bin directory. So the
# question worth answering is not "is it on PATH" but "what does the word satl
# get you right now", which command -v answers exactly.
found=$(command -v satl 2>/dev/null || :)
printf '\n'
if [ -z "$found" ]; then
    cat <<EOF
install.sh: \`satl\` is not on your PATH. Run it in full:

                $(quoted "$installed_satl")

            or re-run this script with --link, which puts a symlink in
            ~/.local/bin, or with --system, which installs into /usr/local/bin,
            a directory every shell already searches.

            Nothing here edits .profile, .bashrc or any other file you own, and
            nothing here will suggest a line for you to add to one.
EOF
    # THE DEBIAN-AND-UBUNTU CASE, WHICH IS NOT A BROKEN INSTALL. The stock
    # ~/.profile on this family adds ~/.local/bin to PATH only `if [ -d ]`, and
    # it tests that at LOGIN. So on a machine where this run CREATED that
    # directory, the link is correct and this shell will not see it until the
    # next login -- which looks exactly like a failed install and is not one.
    if [ -L "$user_bin/satl" ]; then
        case ":$PATH:" in
            *":$user_bin:"*) ;;
            *)
                printf '\n            The link IS there -- %s -- and\n' "$user_bin/satl"
                printf '            %s is not on this PATH. On Debian and Ubuntu\n' "$user_bin"
                printf '            ~/.profile adds that directory only if it EXISTS when you\n'
                printf '            log in, and this run may have just created it. Log out and\n'
                printf '            back in, or for this shell only:\n\n'
                printf '                PATH="%s:$PATH"\n' "$user_bin"
                ;;
        esac
    fi
elif [ "$found" = "$installed_satl" ] || [ "$found" -ef "$installed_satl" ]; then
    printf 'install.sh: `satl` runs the copy just installed.\n'
elif [ -L "$user_bin/satl" ] && [ "$user_bin/satl" -ef "$installed_satl" ]; then
    # SHADOWED, WHICH IS NOT THE SAME FACT AS "NOT LINKED", and telling the two
    # apart is the whole reason this branch exists: the link WAS made, points at
    # the right file, and is simply never reached. Reporting "the installer
    # touched nothing" here would leave the only available conclusion being that
    # the installer does not work. It does. PATH order does not care.
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

    # THE BUILD ITSELF ON PATH IS NOT AN OLDER INSTALL -- the tree, or build/
    # where this installer's make puts the binaries -- so it gets no advice to
    # remove anything. The enterprise installer told its author exactly that,
    # wrongly, until 2026-09-13.
    if [ "$_shadow_dir" -ef "$repo" ] || [ "$_shadow_dir" -ef "$build" ]; then
        printf '\n            %s holds the build this install was\n' "$_shadow_dir"
        printf '            copied from, so nothing there needs removing. To reach the\n'
        printf '            install, put %s ahead of it in your PATH\n' "$user_bin"
        printf '            yourself.\n'
    else
        # THE REMEDY IS PRINTED AND NOT PERFORMED. Taking the older file out
        # may need root, and this script does not escalate.
        printf '\n            This script will not touch %s: it is\n' "$found"
        printf '            not this install%ss file. Two ways out --\n' "'"
        printf '\n              1. remove the older install with its own uninstaller;\n'
        printf '\n              2. or put %s ahead of\n' "$user_bin"
        printf '                 %s in your PATH yourself. That is a\n' "$_shadow_dir"
        printf '                 change to a file you own, so it is yours to make.\n'
    fi
    printf '\n            Afterwards run `hash -r`, or open a new shell:\n'
    printf '            this one has already remembered where satl was.\n'
else
    cat <<EOF
install.sh: note -- \`satl\` already runs $found,
            which is not the copy just installed, and nothing was done about
            it: this install put its interpreter in $root and
            touched nothing else. To run the one from this tree, spell it out:

                $(quoted "$installed_satl")

            If this shell has run satl already, run \`hash -r\` first.
EOF
fi

if [ -n "$found" ]; then
    printf '\ninstall.sh: an alias or shell function named satl beats PATH, and a\n'
    printf '            script cannot see yours: `type satl` in your shell says\n'
    printf '            what the word really runs.\n'
fi

# THE VERSION IS READ, NOT WRITTEN HERE. This said "milestone 2 ... It still
# runs no program" until 2026-09-13, long after M10 ran the first one. The
# number now comes from the INSTALLED binary, and in a dry run from the root's
# make_support/020-version.mk, which this tree's build includes and compiles
# that number from. The enterprise installer's 080-report.sh does the same.
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
