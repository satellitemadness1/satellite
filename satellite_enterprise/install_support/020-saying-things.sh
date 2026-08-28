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
usage: $self [--no-link] [--desktop] [-n|--dry-run]
       $self --system [-n|--dry-run]              (needs to be run as root)
       $self --uninstall [--system] [-n|--dry-run]
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

  --no-link      do NOT symlink ~/.local/bin/satl and ~/.local/bin/satl-term
                 at the installed programs. The links are made by default, and
                 they are what makes typing \`satl\` work: ~/.local/bin is on
                 PATH by convention, and $root deliberately is not. Nothing is
                 ever overwritten -- a path holding something this script did
                 not create is refused and listed at the end -- so the reason
                 to decline is that you want the name left alone, not safety.
                 It never edits a shell startup file either way, and never
                 asks you to set a variable.
  --desktop      also symlink the icons, the .satl file type and the satl-term
                 launcher into ~/.local/share, which is where a desktop
                 actually looks. Without it they are installed under the root
                 above, which no desktop reads, so .satl files keep whatever
                 icon they already had. The interpreter is unaffected either
                 way. OFF by default, unlike --link, because it also rebuilds
                 three indexes covering every application on the machine, and
                 that is a fair thing to ask for and an unfair thing to
                 assume.
  --system       install into the operating system -- /usr/local, in the bin/
                 and share/ shape, over whatever satl is there now -- instead
                 of into your home directory. This is the one that makes the
                 word \`satl\` mean this build for every user of the machine,
                 and the one that fixes the icon a .satl file is drawn with,
                 because a desktop reads /usr/local/share and never reads
                 the private root above. It needs a writable prefix, and says
                 so rather than calling sudo itself.
  --static       link the C++ runtime into the programs instead of loading it
                 at run time, so they do not need a libstdc++ on the machine
                 they run on. ON BY DEFAULT wherever this machine can do it,
                 which costs about a megabyte a binary and buys one that runs
                 with an empty environment and no compiler present. It matters
                 most under --system:
                 a build made in an environment with LD_RUN_PATH set otherwise
                 loads its libstdc++ out of the builder's home directory, and
                 in /usr/local/bin that is a program every account can find and
                 one account can start. Off by default for a home install,
                 which is run by that account anyway. --no-static turns it off.
                 satl-term is never fully static -- gtk and vte load their own
                 modules -- but its C++ runtime is linked in like the rest.
  --uninstall    remove everything this script installed, by name. Give it
                 with --system to remove a system install.
  -n, --dry-run  print the commands that would run, and run none of them.
  --help         this text.

options for rehearsing and packaging:
  --root DIR     install under DIR rather than the default above. This exists
                 so that the script can be rehearsed without writing into the
                 home directory it is meant for; it is not a prefix, and
                 satellite does not look for itself anywhere but where it was
                 put.
  --prefix DIR   --system, but somewhere other than /usr/local. bin/ and
                 share/ are written underneath DIR. Give one of --root and
                 --prefix: they are two layouts, not two spellings.

environment:
  MAKE           the make to use (default: $MAKE).

Without --system nothing here needs root, because nothing is written outside
your home directory. With it, everything is written under the prefix and
nothing else changes: this script never calls sudo, never edits .profile,
.bashrc or any other file you own, and never asks you to export a variable.
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
