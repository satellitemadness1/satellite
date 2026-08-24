#!/bin/sh
# install.sh — install satellite from a source tarball or from the prebuilt
# download folder.
#
# For people who have a tarball rather than a .deb. It is deliberately thin: the
# install tree is declared exactly once, in the Makefile's `install` target, and
# this script only decides WHERE that tree goes and then calls it. Two lists of
# files to install is how an install tree rots — the day someone adds a data
# file to one of them, the other is silently wrong, and the failure surfaces as
# a missing file on a user's machine rather than as a build error here.
#
# The prebuilt download folder has no Makefile to call, and that used to be the
# end of the story: the script exited with "no Makefile beside ./install.sh"
# and installed nothing. It carries install_tree/ instead — the output of that
# same one declaration, staged prefix-relative by `make bundle` — and this
# script copies it. Still one list, still in the Makefile; the bundle ships
# what running it produced rather than a second copy of it. See the mode
# detection further down.
#
# POSIX sh, not bash. On a minimal Ubuntu image /bin/sh is dash, and an
# installer is the one program that has to run before anything is installed.
#
# Nothing here writes to .profile, .bashrc, .zshrc or any other file the user
# owns, and nothing tells the user to set an environment variable. Three
# reasons, of which the third is the one that matters:
#
#   - A child process cannot change its parent's environment. An installer that
#     "exports PATH" exports it into a shell that exits one line later, so the
#     gesture is theatre even when the user wanted it.
#   - Editing a login file is a permanent change to something the user owns,
#     made by a program they ran once. `--uninstall` cannot reliably undo it,
#     which is how a stale PATH entry outlives three uninstalls.
#   - An install that only works once SATELLITE_PATH is set is not an install.
#     The interpreter resolves its own library directory in three tiers —
#     $SATELLITE_PATH, then ../share/satellite/lib relative to /proc/self/exe,
#     then the compiled-in SATELLITE_LIB_DIR baked from `prefix` — and that
#     ordering exists precisely so tiers 2 and 3 cover every installed case.
#     If satellite needs SATELLITE_PATH to find its library, tier 2 or tier 3 is
#     broken, and exporting the variable hides the bug instead of fixing it.
#     See DESIGN.md §9 and satl(1) under ENVIRONMENT.
#
# So the most this script does about PATH is SAY that $prefix/bin is not on it,
# and print the exact line the user may add themselves if they want it.

set -eu

# The default prefix follows who is running the script, because there is only
# ever one right answer and it is decided by that.
#
# /usr/local is the correct place for an install from source, and it needs root.
# Defaulting to it unconditionally meant that plain `./install.sh` -- the
# command anyone tries first -- could not succeed for a normal user: it printed
# a permission error and the two flags to choose between, and installed
# nothing. Nobody who ran it wanted that outcome. Someone with root wants the
# system install; someone without wants the one that needs no privileges, and
# ~/.local is on PATH and in the XDG data search path on any current desktop, so
# it works with no further setup.
#
# --prefix still wins over both, which is what keeps this a default rather than
# a policy, and the chosen value is printed before anything is written so it is
# never a surprise. This does NOT run sudo: escalating on the user's behalf is
# the thing the note further down refuses to do, and choosing a writable
# directory instead is the opposite of that, not a version of it.
if [ "$(id -u)" = 0 ]; then
    prefix=/usr/local
else
    prefix=${HOME:?HOME is not set, so there is no home prefix to default to}/.local
fi
# DESTDIR is read from the environment rather than given a flag because that is
# the interface every packaging system already drives: `DESTDIR=$PWD/stage
# ./install.sh`. It stages a tree and is never baked into a path or a binary,
# which is why it is not part of the writability advice below — staging into a
# scratch directory has nothing to do with whether /usr/local is yours.
destdir="${DESTDIR:-}"
action=install
dry_run=no
: "${MAKE:=make}"

self=$0

die() {
    printf 'install.sh: %s\n' "$1" >&2
    exit 1
}

usage() {
    cat <<EOF
usage: $self [--prefix DIR] [-n|--dry-run]
       $self --uninstall [--prefix DIR] [-n|--dry-run]
       $self --help

  --prefix DIR   install under DIR. Optional: with no --prefix this installs to
                 \$HOME/.local as a normal user and to /usr/local as root, so
                 running it plain and running it under sudo each do the right
                 thing on their own. For you, right now, that is:
                     $prefix
                 Must be absolute: the prefix is compiled into the interpreter
                 as its last-resort library location, and a relative path would
                 resolve against whatever directory the program happened to be
                 run from later.
  --uninstall    remove a previously installed tree from the same prefix.
  -n, --dry-run  print the commands that would run, and run none of them.
  --help         this text.

environment:
  DESTDIR        stage the tree under this directory instead of installing it
                 live. For package builds; never baked into any path.
  MAKE           the make to use (default: $MAKE). Source trees only -- the
                 prebuilt download folder is installed without make.

This installs either of two things, and tells them apart by what is beside it:

  a source tree      a Makefile is beside this script. The work is done by
                     "make install", so that the list of installed files lives
                     in one place, and this script builds first if the binaries
                     are missing.
  the download folder  install_tree/ is beside this script. That is the same
                     "make install" tree, already staged, so it is copied into
                     the prefix -- no Makefile, no sources and no compiler
                     needed on this machine.

Either way this script chooses the prefix and reports afterwards; it never
edits your shell startup files and never asks you to set an environment
variable.
EOF
}

# Every command this script prints -- in a --dry-run transcript, and in the
# advice it gives when a prefix is not writable -- is printed so that it can be
# pasted straight back into a shell. That matters here because a tarball is
# routinely unpacked somewhere with a space in the path, and advice that has to
# be edited before it works is advice that gets mistyped. Words that need no
# quoting are printed bare, because a transcript in which every word is quoted
# is one nobody reads.
quoted() {
    case $1 in
        '' | *[!A-Za-z0-9_/.,=:@%+-]*)
            printf "'%s'" "$(printf '%s' "$1" | sed "s/'/'\\\\''/g")"
            ;;
        *)
            printf '%s' "$1"
            ;;
    esac
}

show() {
    _sep='  '
    for _word in "$@"; do
        printf '%s%s' "$_sep" "$(quoted "$_word")"
        _sep=' '
    done
    printf '\n'
}

run() {
    if [ "$dry_run" = yes ]; then
        show "$@"
    else
        "$@"
    fi
}

while [ $# -gt 0 ]; do
    case $1 in
        --prefix)
            [ $# -ge 2 ] || die "--prefix needs a directory"
            prefix=$2
            shift 2
            ;;
        --prefix=*)
            prefix=${1#--prefix=}
            shift
            ;;
        --uninstall)
            action=uninstall
            shift
            ;;
        -n | --dry-run)
            dry_run=yes
            shift
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        -*)
            printf 'install.sh: unknown option %s\n\n' "$1" >&2
            usage >&2
            exit 2
            ;;
        *)
            printf 'install.sh: unexpected argument %s\n\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

[ $# -eq 0 ] || die "unexpected argument $1"
[ -n "$prefix" ] || die "--prefix needs a directory"

case $prefix in
    /*) ;;
    *)
        die "--prefix must be an absolute path, not \"$prefix\".
       The prefix is compiled into the interpreter, so a relative one would
       name a different directory every time the program is run. To build a
       package against a staging directory, keep an absolute prefix and set
       DESTDIR instead: DESTDIR=\$PWD/stage $(quoted "$self") --prefix /usr"
        ;;
esac

# A trailing slash installs identically and reads worse in every message below.
while :; do
    case $prefix in
        /) break ;;
        */) prefix=${prefix%/} ;;
        *) break ;;
    esac
done

# The tarball this script was unpacked with is the one it installs, so the
# repository is found relative to the script and not to the caller's directory:
# `sh /mnt/satellite/install.sh` from $HOME must still build /mnt/satellite.
repo=$(dirname -- "$self")
repo=$(cd -- "$repo" && pwd)

# TWO KINDS OF TARBALL REACH THIS SCRIPT, and which one this is decides
# everything below.
#
#   source   the repository, or a tarball of it. There is a Makefile beside
#            this script, so the install tree is declared next door and this
#            script's whole job is to choose a prefix and call `make install`.
#
#   bundle   the download folder from the website: two prebuilt binaries, this
#            script, and install_tree/ -- which is that same `make install`
#            tree, already staged prefix-relative by `make bundle`. No
#            Makefile, no sources, nothing to compile. That is the point of a
#            prebuilt download: the machine receiving it needs no toolchain,
#            and requiring one here would have made the whole bundle pointless.
#
# Until bundle mode existed this script died on the spot in the second case --
# "no Makefile beside ./install.sh" -- so a user who downloaded the folder,
# unpacked it and ran the installer inside it installed nothing at all.
#
# Source mode is tested FIRST, because one directory can be both: `make bundle`
# leaves enterprise_download/ inside the repository, and a developer who runs
# the repository's own install.sh means the Makefile, which is the newer and
# more complete answer of the two.
tree=$repo/install_tree
if [ -f "$repo/Makefile" ]; then
    mode=source
elif [ -d "$tree" ]; then
    mode=bundle
else
    die "no Makefile and no install_tree beside $self.
       This installs one of two things: an unpacked satellite source tarball,
       which has a Makefile beside it, or the prebuilt download folder, which
       has an install_tree/ beside it. This directory is neither, so there is
       nothing here to install."
fi

# The nearest existing ancestor is what actually decides the answer:
# --prefix "$HOME/.local" is perfectly installable when .local does not exist
# yet, as long as $HOME does, and testing -w on a directory that is about to be
# created would refuse every first install.
existing_ancestor() {
    _dir=$1
    while [ ! -d "$_dir" ]; do
        _parent=$(dirname -- "$_dir")
        if [ "$_parent" = "$_dir" ]; then
            break
        fi
        _dir=$_parent
    done
    printf '%s\n' "$_dir"
}

target=$destdir$prefix
ancestor=$(existing_ancestor "$target")

if [ ! -w "$ancestor" ]; then
    user=$(id -un 2>/dev/null || printf '%s' "${USER:-you}")
    message="$ancestor is not writable by $user.

  Either install it as root:
      sudo sh $(quoted "$self") --prefix $(quoted "$prefix")
  or install it into your home directory, which needs no privileges:
      sh $(quoted "$self") --prefix \"\$HOME/.local\"

  This script will not run sudo for you. A program that silently escalates is a
  program you cannot audit by reading the command you typed."

    # A dry run is asking what WOULD happen, so it answers that question and
    # says the permissions would stop it, rather than refusing to answer.
    if [ "$dry_run" = yes ]; then
        printf 'install.sh: note — %s\n\n' "$message"
    else
        die "$message"
    fi
fi

# Source mode only: bundle mode has the binaries already, which is what it is
# for. Everything inside this block is about compiling, and in bundle mode
# there is no compiler, no sources and no Makefile to drive.
if [ "$action" = install ] && [ "$mode" = source ]; then
    # `make install` would build these itself, but doing it as a separate step
    # means a compile failure is reported as a compile failure, before anything
    # has been copied anywhere.
    #
    # The prefix goes to the BUILD, not only to the install: it is compiled in
    # as SATELLITE_LIB_DIR, so building without it and installing with it would
    # bake /usr/local into a binary going to $HOME/.local. Passing the same
    # prefix to both is also what stops make from compiling everything twice.
    #
    # SATELLITE_AUTOINSTALL=0 goes to both build invocations below, because
    # `make` at the top of the tree now installs what it built. That is right
    # for someone typing make; it is wrong here twice over. The build below runs
    # `make -C "$repo"`, which the Makefile cannot tell apart from a hand-typed
    # build in that directory, so without this it would install once during the
    # build and again at the `make install` further down -- and in the sudo
    # branch the first one is fatal, not merely redundant: that build runs as
    # the human, at a prefix only root can write, so it would refuse the install
    # and (worse, on any make that returned non-zero for it) take `set -e` and
    # the whole installer down before the privileged copy ever happened. This
    # script chooses the prefix and this script performs the install; the build
    # step it drives does neither.
    #
    # A command-line assignment rather than an exported variable, for the same
    # reason the prefix is one: sudo's env_reset strips the environment across
    # the -u boundary below, and the make command line is the only level that
    # outranks an assignment inside the makefile.
    if [ "$(id -u)" = 0 ] && [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != root ]; then
        # Compile as the human, copy as root. Under sudo the compile would run
        # as root with HOME=/root, which fails to find a toolchain installed in
        # the user's home and -- worse when it succeeds -- leaves root-owned .o
        # files in a source tree the developer then cannot rebuild without
        # sudo forever after. Copying files into /usr/local is the only part of
        # this that actually needs privilege, so it is the only part that gets
        # it.
        printf 'install.sh: building as %s; root is used only for the copy\n' \
            "$SUDO_USER"
        run sudo -H -u "$SUDO_USER" "$MAKE" -C "$repo" "prefix=$prefix" \
            SATELLITE_AUTOINSTALL=0
    elif [ -x "$repo/satl" ] && [ -x "$repo/satl-term" ]; then
        # Binaries left over from a plain `make` carry that make's prefix, which
        # is now the one it installed itself to -- $HOME/.local for an ordinary
        # user, not /usr/local. The `make install` below is what corrects them:
        # the Makefile records the prefix in a stamp file that system.o depends
        # on, so a changed prefix recompiles the one object that was told the
        # old one and relinks. Nothing here has to force it.
        printf 'install.sh: using the binaries already built in %s\n' "$repo"
    else
        printf 'install.sh: building\n'
        run "$MAKE" -C "$repo" "prefix=$prefix" SATELLITE_AUTOINSTALL=0
    fi
fi

printf 'install.sh: %s prefix=%s%s\n' "$action" "$prefix" \
    "${destdir:+ (staged under $destdir)}"

# ---------------------------------------------------------------------------
# Bundle mode: installing install_tree/ without a Makefile.
#
# The list of files is DERIVED BY WALKING WHAT WAS SHIPPED, never restated. That
# is what keeps the promise at the top of this file. install_tree/ is not a
# second copy of the install tree -- it is the RESULT of the one declaration in
# the Makefile's `install` target, produced by running it into a staging
# DESTDIR at an empty prefix. Add a data file to that target and it appears in
# the next bundle, and in these two functions, with no edit here.
#
# `install -D` per file rather than `cp -R`, for two reasons that both bite:
#
#   - cp -R applies the umask to what it creates. Under the 002 that is Ubuntu's
#     default for the primary user, share/ arrives group-writable -- the same
#     trap the Makefile's note on `install -d` describes at length.
#   - cp -Rp preserves the ownership of the unpacked tarball, which is whoever
#     unpacked it. `sudo ./install.sh` would then fill /usr/local with files
#     owned by that user. install(1) creates as the caller, which under sudo is
#     root, which is what /usr/local wants.
#
# The mode comes from the shipped file's own executable bit, so the binaries
# land 755 and the data lands 644, which is what the Makefile installed them as.
bundle_install() {
    # Directories first, and separately from the files, because
    # share/satellite/lib is shipped EMPTY and is load-bearing:
    # library_path()'s tier 2 accepts a candidate only if the directory exists,
    # and that empty directory is the entire reason a relocated tree resolves
    # to itself rather than falling through to the prefix it was built for
    # (DESIGN §9). A walk over files alone drops it without a word, and the
    # symptom is `satl --where` answering with this build machine's home
    # directory on somebody else's computer.
    #
    # -m755 stated on every one, for the umask reason above. $target itself is
    # not created here, for the same reason the Makefile does not create
    # $(prefix): install -D makes the leading directories it needs, and a
    # prefix the caller named is theirs to have made.
    (cd "$tree" && find . -type d -print) | while read -r d; do
        d=${d#.}
        d=${d#/}
        [ -n "$d" ] || continue
        run install -d -m755 "$target/$d" || exit 1
    done

    (cd "$tree" && find . -type f -print) | while read -r f; do
        f=${f#./}
        if [ -x "$tree/$f" ]; then _m=755; else _m=644; fi
        run install -D -m$_m "$tree/$f" "$target/$f" || exit 1
    done
}

# Symmetric with the Makefile's `uninstall`, and asymmetric with the install
# above in exactly the way that target is: share/satellite and
# share/doc/satellite are trees this install created and owns outright, so
# removing them wholesale is exact and also takes the empty lib/ with it.
# Everything else lives in a directory shared with the rest of the system --
# share/icons/hicolor, share/applications, share/mime/packages, share/man --
# where only the named files may go and the directory itself must stay, so
# those are removed one at a time by the same walk that installed them.
bundle_uninstall() {
    (cd "$tree" && find . -type f -print) | while read -r f; do
        f=${f#./}
        run rm -f "$target/$f" || exit 1
    done
    run rm -rf "$target/share/satellite" "$target/share/doc/satellite"
}

# The four things `make install` does at the end of its own recipe and this
# staged tree could not carry: hicolor's index.theme, and the three indexes a
# desktop reads once at session start.
#
# The Makefile does all four only when DESTDIR is empty, and `make bundle`
# staged this tree with DESTDIR set -- correctly, because staging must not
# touch the build machine's live share/. So in bundle mode they fall to this
# script, and this is the ONE place where bundle mode restates something the
# Makefile also says. The rot warned about at the top of this file does not
# apply to it: this is four tool invocations, not a list of files, and nothing
# here goes stale when a data file is added to the install tree.
#
# Each of the four is argued in full in the Makefile's `install` recipe. The
# short version: an icon directory is not a THEME until it holds index.theme
# and GTK does not look inside one that does not, so without it every icon just
# installed is invisible at every size; and the other three are indexes, so a
# launcher and a file type stay unknown to the shell until they are rebuilt.
#
# index.theme is copied only when absent and only from the system's own copy,
# never invented -- it describes hicolor, not satellite -- and it is
# deliberately NOT removed on the way out, because every other application that
# installed an icon into this prefix depends on it.
#
# Failure is ignored throughout: none of these tools is required for the
# install to be CORRECT, only for it to be noticed before the next login.
bundle_reindex() {
    # A staged tree is not an installation. Rebuilding this machine's indexes
    # over a DESTDIR that will be packaged and installed somewhere else is at
    # best noise and at worst a package build editing the build machine.
    [ -n "$destdir" ] && return 0

    _hicolor=$prefix/share/icons/hicolor
    if [ -d "$_hicolor" ] && [ ! -f "$_hicolor/index.theme" ] &&
       [ -f /usr/share/icons/hicolor/index.theme ]; then
        run cp /usr/share/icons/hicolor/index.theme \
               "$_hicolor/index.theme" 2>/dev/null || :
    fi

    if command -v update-desktop-database >/dev/null 2>&1; then
        run update-desktop-database "$prefix/share/applications" \
            2>/dev/null || :
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        run gtk-update-icon-cache -qtf "$_hicolor" 2>/dev/null || :
    fi
    if command -v update-mime-database >/dev/null 2>&1; then
        run update-mime-database "$prefix/share/mime" 2>/dev/null || :
    fi
}

if [ "$mode" = source ]; then
    if [ -n "$destdir" ]; then
        run "$MAKE" -C "$repo" "$action" "prefix=$prefix" "DESTDIR=$destdir"
    else
        run "$MAKE" -C "$repo" "$action" "prefix=$prefix"
    fi
elif [ "$action" = install ]; then
    bundle_install
    bundle_reindex
else
    bundle_uninstall
    bundle_reindex
fi

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

# The desktop half of the install -- the launcher, the app icon, and the icon on
# every .satl file -- has one prerequisite that a prefix outside /usr cannot
# satisfy on its own, and it fails silently.
#
# An icon directory is only a THEME if it contains index.theme. That file
# belongs to the hicolor-icon-theme package, which installs it into the system
# prefix and nowhere else, so $prefix/share/icons/hicolor is a directory full of
# correctly named PNGs that GTK will not look inside: has_icon() answers false
# for every one of them, at every size, and the shell falls back to a generic
# page. gtk-update-icon-cache does not reveal this -- the Makefile runs it with
# -t, which means --ignore-theme-index, so the cache builds successfully over a
# theme that is not yet a theme.
#
# This is reported rather than repaired, for the reason at the top of this file:
# index.theme describes hicolor, not satellite. Copying one in would put a file
# outside the install tree, owned by no package, that `--uninstall` must then
# either orphan or delete out from under whatever else has since installed an
# icon beside ours. Neither is a decision an installer gets to make quietly, and
# the fix is one line the user can read before running it.
# An icon directory is only a theme if it holds index.theme, and GTK ignores one
# that does not: every icon just installed is invisible, at every size. The
# install target copies the system's in when the prefix has none, which covers
# every ordinary case -- including /usr/local, which has no index.theme of its
# own and is this script's default, so the earlier version of this check
# skipping anything under /usr was skipping the commonest install of all.
#
# Reaching this branch therefore means the copy could not happen: no
# hicolor-icon-theme on the machine, or a prefix the install could not write
# that far into. Reported rather than repaired, because at that point there is
# no correct file to put there and inventing one would be declaring someone
# else's theme.
hicolor=$prefix/share/icons/hicolor
if [ -d "$hicolor" ] && [ ! -f "$hicolor/index.theme" ]; then
    cat <<EOF

install.sh: note — $hicolor has no index.theme,
            so the desktop will not find the icons that were just installed
            there. That file ships with hicolor-icon-theme; this machine
            appears not to have it. Installing that package, or copying an
            index.theme in by hand, is what makes the icons visible:

                gtk-update-icon-cache -qtf $(quoted "$hicolor")

            The interpreter is unaffected either way; this is only about the
            launcher and the icon shown on .satl files.
EOF
fi

# GNOME reads the desktop, icon and mime indexes once at session start. The
# Makefile has already rebuilt all three, so a new session picks everything up
# with no further action -- but the CURRENT one will not, and an icon that is
# correct on disk and absent on screen reads as an install that failed.
printf 'install.sh: the launcher and the .satl file icon appear at your next\n'
printf '            login; the indexes they come from are already rebuilt.\n'
