# satellite -- die, usage, and the two that print a command before running it.
#
# EVERY COMMAND THIS SCRIPT RUNS GOES THROUGH run(), which is what makes
# --dry-run a complete and honest transcript rather than an approximation.
# quoted() is why that transcript can be pasted back into a shell.
#
# Both are taken from the first satellite's installer unchanged, because they
# were right there and the argument for them has not moved.

die() {
    printf 'install.sh: %s\n' "$1" >&2
    exit 1
}

usage() {
    cat <<EOF
usage: $self [--link] [--desktop] [-n|--dry-run]
       $self --uninstall [-n|--dry-run]
       $self --help

Installs every satellite program, the .satl file type and the artwork into
    $root
The programs are the interpreter satl, the detector satl-cpu-level, and the
GTK4/VTE window satl-term when the libraries to build it are present.

On x86-64 it builds satl twice, once for the baseline and once for x86-64-v3
(the instruction set Haswell introduced in 2013), runs satl-cpu-level to ask
this CPU which of them it can execute, and installs that one under the name
satl. So the build makes four files and the install is three programs:
satl.haswell is not a fourth program, it is satl compiled a second time.

  --link         also symlink ~/.local/bin/satl and ~/.local/bin/satl-term at
                 the installed programs, so that typing either name finds it.
                 Off by default because those paths may already hold another
                 satl -- this machine's does -- and this script refuses to
                 overwrite anything it does not own. It never edits a shell
                 startup file either way.
  --desktop      also symlink the icons, the .satl file type and the satl-term
                 launcher into ~/.local/share, which is where a desktop
                 actually looks. Without it they are installed under the root
                 above, which no desktop reads, so .satl files keep whatever
                 icon they already had. The interpreter is unaffected either
                 way. Worth giving WITH --link: the launcher runs \`satl-term\`
                 by bare name, so it hides itself until that name is on PATH.
  --uninstall    remove everything this script installed, by name.
  -n, --dry-run  print the commands that would run, and run none of them.
  --help         this text.

options for rehearsing and packaging:
  --root DIR     install under DIR rather than the default above. This exists
                 so that the script can be rehearsed without writing into the
                 home directory it is meant for; it is not a prefix, and
                 satellite does not look for itself anywhere but where it was
                 put.

environment:
  MAKE           the make to use (default: $MAKE).

Nothing here needs root, because nothing here is written outside your home
directory. Nothing here edits .profile, .bashrc or any other file you own.
EOF
}

# Every command this script prints -- in a --dry-run transcript, and in the
# advice at the end -- is printed so that it can be pasted straight back into a
# shell. That matters because a home directory can have a space in it, and
# advice that has to be edited before it works is advice that gets mistyped.
# Words that need no quoting are printed bare, because a transcript in which
# every word is quoted is one nobody reads.
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

# A step heading, so that a transcript of thirty copies has somewhere to be read
# from. Deliberately not a progress bar: this install is a few dozen files and
# finishes faster than anything could usefully animate.
step() {
    printf 'install.sh: %s\n' "$1"
}
