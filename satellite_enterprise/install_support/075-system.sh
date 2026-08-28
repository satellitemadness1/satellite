# satellite -- the part that only happens when installing INTO an operating
# system, and the part that takes the first satellite's place there.
#
# New on 2026-08-28. Everything here is gated on layout=prefix and does nothing
# at all in the default home-directory install, which is why it is a fragment of
# its own rather than three more branches inside 070-desktop.sh: that file
# answers "how does this user's desktop find it", this one answers "how does the
# machine find it", and they are not the same question even though they end in
# the same three index-rebuilding commands.
#
# 070-desktop.sh is sourced first and is where rebuild_data_indexes() comes
# from. It was split out of refresh_indexes() for this file.

if [ "$layout" = prefix ]; then

# ---------------------------------------------------------------------------
# THE THEME INDEX, WHICH IS WHY .satl FILES HAVE THE WRONG ICON.
#
# This is the whole bug the author reported, and it is not about satellite at
# all. An icon directory is only a THEME if it contains an index.theme, and
# without one GTK does not look inside it: has_icon() answers false for every
# icon in it, at every size, and the desktop draws the generic fallback. So a
# prefix can hold all eighteen correct PNGs and a .satl file still shows a blank
# page, with nothing anywhere reporting an error.
#
# gtk-update-icon-cache does not reveal it either, because the -t this script
# passes is --ignore-theme-index: the cache builds happily over a directory that
# is not yet a theme. The only symptom is artwork that never appears.
#
# The file belongs to the hicolor-icon-theme package, which installs it under
# /usr and NOWHERE ELSE. So every prefix except /usr starts without one --
# /usr/local included, which is this script's own default. The first satellite
# found this and wrote it down in make_support/140-install.mk; that install is
# the reason the icons work on this machine today, and an installer that
# replaces it without doing the same thing breaks them on its way in.
#
# COPIED AND NOT GENERATED, because the file describes hicolor itself -- its
# full directory list, their sizes and their contexts -- and not our nine sizes
# of it. Only when absent: a prefix that already has one has it for somebody
# else's icons too.
theme_index=$root/share/icons/hicolor/index.theme

if [ "$action" = install ]; then
    if [ -f "$theme_index" ]; then
        theme_index_state=present
    elif [ -f /usr/share/icons/hicolor/index.theme ]; then
        run mkdir -p "$root/share/icons/hicolor"
        run cp /usr/share/icons/hicolor/index.theme "$theme_index"
        theme_index_state=installed
    else
        # Not fatal. The interpreter is installed and correct; what is missing
        # is the file that makes a desktop read the icons, and the package that
        # owns it is named rather than guessed at.
        theme_index_state=missing
    fi
fi

# ---------------------------------------------------------------------------
# WHAT AN EARLIER SATELLITE OWNED HERE THAT THIS ONE DOES NOT INSTALL.
#
# THE OVERLAP NEEDS NO CODE AT ALL, and that is worth saying first because it is
# most of the problem. Of the twenty-six satellite-owned paths measured under
# /usr/local on 2026-08-28, TWENTY-TWO are paths this install writes itself --
# the launcher, the mime packet, all eighteen icons, plus the compiled
# share/mime/application/x-satellite.xml that update-mime-database regenerates.
# 060-install-tree.sh writes straight over them and the OS is left holding this
# build. Nothing here is needed for those.
#
# THE REMAINING FOUR CANNOT BE OVERWRITTEN, because overwriting is something you
# do to a path you write, and this install writes none of them. A man page, a
# completion file and two directories of examples and docs are simply not part
# of satellite yet. Left alone they are not neutral: `man satl` would answer
# with 2026-08-23's flags for a binary that now has different ones, and the
# shell would complete options this satl does not have. That is the exact rot
# 060's header warns about, arriving from the outside.
#
# DECLARED, NOT SCATTERED, and declared WITH ITS REASON, so that the report at
# the bottom of this file explains each deletion instead of listing paths. This
# is the same argument 060-install-tree.sh makes about the install tree: one
# list, in one place, or the day somebody adds an entry the other copy is
# silently wrong.
superseded_tree() {
    printf 'share/man/man1/satl.1.gz\ta man page for a satl with different flags\n'
    printf 'share/man/man1/satl-term.1.gz\ta man page for the old window\n'
    printf 'share/bash-completion/completions/satl\tcompletions for options this satl does not have\n'
    printf 'share/satellite\tthe old example programs and the lib/ search directory\n'
    printf 'share/doc/satellite\tthe old DESIGN.md, design/ and licence\n'
}

# THE LIST ABOVE IS FILTERED AGAINST THE INSTALL TREE, AND THAT FILTER IS THE
# WHOLE REASON THIS SURVIVES CONTACT WITH A FUTURE SATELLITE.
#
# A hardcoded list of paths to delete is correct exactly once. The day satellite
# ships its own man page, `make install` writes share/man/man1/satl.1.gz and
# this fragment deletes it again -- and the failure is an install that reports
# success and is missing a file, which is the worst shape a bug can have. The
# filter makes the list self-correcting instead: a path leaves the superseded
# set on the day it enters the install tree, automatically, with no second edit
# and no way to forget. Nothing this run installs can be removed by this run.
#
# The directory arm matters as much as the exact one. share/satellite is removed
# with rm -rf, so if a later satellite installs share/satellite/lib/prelude.satl
# the directory stops being somebody else's the moment that line is added, and
# the recursive delete must stop with it.
_installed_paths=$(install_tree | while IFS=$_TAB read -r _m _s _d; do
    printf '%s\n' "$_d"
done)

this_install_owns() {
    case "
$_installed_paths
" in
        *"
$1
"*)  return 0 ;;
        *"
$1/"*) return 0 ;;
    esac
    return 1
}

superseded=

if [ "$action" = install ]; then
    while IFS=$_TAB read -r _old _why; do
        [ -n "$_old" ] || continue
        [ -e "$root/$_old" ] || continue

        # The filter, applied before anything is removed rather than after.
        if this_install_owns "$_old"; then
            continue
        fi

        if [ -d "$root/$_old" ]; then
            # rm -rf, and only here. $root is absolute and non-/ by
            # 030-arguments.sh, the path is a literal from the list above and
            # not a variable, and this_install_owns() has just been asked about
            # it. Those three together are what make a recursive delete
            # something other than a hazard.
            run rm -rf "$root/$_old"
        else
            run rm -f "$root/$_old"
        fi
        superseded="$superseded$root/$_old$_TAB$_why
"
    done <<EOF
$(superseded_tree)
EOF
fi

# ---------------------------------------------------------------------------
# DOES THIS BINARY RUN FOR ANYONE BUT THE PERSON WHO BUILT IT.
#
# A question only a system install has to ask. LD_RUN_PATH is exported in this
# author's environment, so the linker bakes it into the binary as an RPATH, and
# on this machine that RPATH names /home/madness/opt/gcc-17/lib64 -- which is
# where the libstdc++.so.6 these binaries load actually comes from. In
# $HOME/.satl that is nobody's problem: the only user who runs it is the user
# whose home it is. Copied to /usr/local it becomes a program every account on
# the machine can find on PATH and only one of them can start, and the failure
# arrives as a loader error naming a library rather than a home directory.
#
# A NOTE AND NOT A REFUSAL, and the same reasoning as 040-machine.sh's note
# about the distribution: this is a correct install for the machine it was built
# on, single-user boxes are most boxes, and refusing would be a policy. It is
# also not this script's to fix -- the RPATH is decided by the environment the
# BUILD ran in, so the repair is unsetting LD_RUN_PATH and rebuilding, which is
# a thing to be told rather than done behind the user's back.
#
# readelf is asked for rather than assumed. On a machine without binutils this
# check is skipped, which costs a warning and never costs an install.
if [ "$action" = install ] && [ "$dry_run" = no ] &&
   command -v readelf >/dev/null 2>&1 && [ -f "$installed_satl" ]; then
    _rpath=$(readelf -d "$installed_satl" 2>/dev/null |
             sed -n 's/.*R\(UN\)\?PATH.*\[\(.*\)\]/\2/p' | head -n 1)
    case $_rpath in
        *"$HOME"/* | /home/*)
            printf 'install.sh: note -- %s has a library path\n' "$installed_satl"
            printf '            baked into it that points inside a home directory:\n\n'
            printf '                %s\n\n' "$_rpath"
            printf '            It will run for you. It will fail for any other account on\n'
            printf '            this machine that cannot read that directory, and the error\n'
            printf '            will name a library rather than the home directory it is in.\n'
            printf '            That path came from LD_RUN_PATH in the environment the BUILD\n'
            printf '            ran in, not from anything this script did.\n\n'
            printf '            --static is the fix and is normally ON for --system, so\n'
            printf '            reaching this note means it could not be: this machine has\n'
            printf '            no static libstdc++ for %s to link. Install one, then run\n' "$MAKE"
            printf '            this again and the question goes away for good:\n\n'
            printf '                sudo dnf --enablerepo=crb install libstdc++-static glibc-static\n'
            printf '                sudo sh %s --prefix %s\n' \
                "$(quoted "$self")" "$(quoted "$root")"
            ;;
    esac
fi

# ---------------------------------------------------------------------------
# AND WHAT ALL OF THAT LOOKED LIKE. Reported here rather than in 080-report.sh
# because every line of it is system-only: a home install has no theme index to
# argue about, no other install to supersede and no other users to fail for. It
# is the same doing-and-saying split 060 and 080 keep -- this fragment is simply
# the whole of both halves for one layout.
# THE TWO THINGS --system DID THAT NOTHING ELSE DOES, both worth naming: one is
# the file that decides whether any icon in the prefix is ever drawn, and the
# other is a list of deletions, which a script should never make silently.
if [ "$layout" = prefix ]; then
    case $theme_index_state in
        installed)
            printf 'install.sh: %s %s\n' "$did" "$theme_index"
            printf '            That file is not artwork -- it is what makes the\n'
            printf '            directory a THEME. Without it GTK does not look inside\n'
            printf '            at any size, so the icons above install and are never\n'
            printf '            drawn, and a .satl file keeps the blank page. It ships\n'
            printf '            in /usr only, so every other prefix starts without one.\n'
            ;;
        missing)
            printf 'install.sh: note -- %s\n' "$theme_index"
            printf '            is missing and /usr/share/icons/hicolor/index.theme was\n'
            printf '            not there to copy. The icons installed, but nothing will\n'
            printf '            draw them until that file exists:\n'
            printf '              AlmaLinux/RHEL: dnf install hicolor-icon-theme\n'
            ;;
    esac

    if [ -n "$superseded" ]; then
        printf 'install.sh: %s these, because they described the satl this\n' "$removed"
        printf '            install just replaced at %s, and this satellite has\n' "$root"
        printf '            no counterpart to write over them with:\n'
        printf '%s' "$superseded" | while IFS=$_TAB read -r _p _why; do
            [ -n "$_p" ] || continue
            printf '    %s\n' "$_p"
            printf '        %s\n' "$_why"
        done
        printf '            Everything else an earlier install owned here -- the\n'
        printf '            launcher, the mime packet and all eighteen icons -- is a\n'
        printf '            path this one writes too, so it was simply overwritten and\n'
        printf '            is not listed. See superseded_tree() in 075-system.sh.\n'
    fi
fi


# index.theme IS DELIBERATELY NOT REMOVED ON AN UNINSTALL, which is inherited
# from v1 along with the argument: every other application that installs an icon
# into this prefix depends on that file, so deleting ours on the way out breaks
# theirs. An orphaned theme index is the correct outcome.

fi
