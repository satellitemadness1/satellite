#!/bin/sh
# install.sh — install satellite from a source tarball.
#
# For people who have a tarball rather than a .deb. It is deliberately thin: the
# install tree is declared exactly once, in the Makefile's `install` target, and
# this script only decides WHERE that tree goes and then calls it. Two lists of
# files to install is how an install tree rots — the day someone adds a data
# file to one of them, the other is silently wrong, and the failure surfaces as
# a missing file on a user's machine rather than as a build error here.
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

prefix=/usr/local
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

  --prefix DIR   install under DIR (default: $prefix). Must be absolute: the
                 prefix is compiled into the interpreter as its last-resort
                 library location, and a relative path would resolve against
                 whatever directory the program happened to be run from later.
  --uninstall    remove a previously installed tree from the same prefix.
  -n, --dry-run  print the commands that would run, and run none of them.
  --help         this text.

environment:
  DESTDIR        stage the tree under this directory instead of installing it
                 live. For package builds; never baked into any path.
  MAKE           the make to use (default: $MAKE).

The work is done by "make install" so that the list of installed files lives in
one place. This script chooses the prefix, builds if the binaries are missing,
and reports afterwards; it never edits your shell startup files and never asks
you to set an environment variable.
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
[ -f "$repo/Makefile" ] ||
    die "no Makefile beside $self — run this from an unpacked satellite tarball"

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

if [ "$action" = install ]; then
    # `make install` would build these itself, but doing it as a separate step
    # means a compile failure is reported as a compile failure, before anything
    # has been copied anywhere.
    #
    # The prefix goes to the BUILD, not only to the install: it is compiled in
    # as SATELLITE_LIB_DIR, so building without it and installing with it would
    # bake /usr/local into a binary going to $HOME/.local. Passing the same
    # prefix to both is also what stops make from compiling everything twice.
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
        run sudo -H -u "$SUDO_USER" "$MAKE" -C "$repo" "prefix=$prefix"
    elif [ -x "$repo/satl" ] && [ -x "$repo/satl-term" ]; then
        # Binaries left over from a plain `make` may carry a different prefix,
        # and the `make install` below is what corrects them: the Makefile
        # records the prefix in a stamp file that system.o depends on, so a
        # changed prefix recompiles the one object that was told the old one
        # and relinks. Nothing here has to force it.
        printf 'install.sh: using the binaries already built in %s\n' "$repo"
    else
        printf 'install.sh: building\n'
        run "$MAKE" -C "$repo" "prefix=$prefix"
    fi
fi

printf 'install.sh: %s prefix=%s%s\n' "$action" "$prefix" \
    "${destdir:+ (staged under $destdir)}"

if [ -n "$destdir" ]; then
    run "$MAKE" -C "$repo" "$action" "prefix=$prefix" "DESTDIR=$destdir"
else
    run "$MAKE" -C "$repo" "$action" "prefix=$prefix"
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

case ":${PATH:-}:" in
    *:"$bindir":*)
        printf 'install.sh: %s is on your PATH, so `satl` finds it.\n' \
            "$bindir"
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
