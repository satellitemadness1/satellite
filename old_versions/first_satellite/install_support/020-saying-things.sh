# satellite -- die, usage, and the two that print a command before running it.
#
# EVERY COMMAND THIS SCRIPT RUNS GOES THROUGH run(), which is what makes
# --dry-run a complete and honest transcript rather than an approximation.
# quoted() is why that transcript can be pasted back into a shell.
#
# Moved out of the 634-line install.sh on 2026-08-24, byte for byte.

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
