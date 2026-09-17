# satellite 004 -- die, refuse, and the usage.

die() {
    printf 'install.sh: %s\n' "$1" >&2
    exit 1
}

# A command line this installer does not take: the reason, where to read more, 23.
refuse_command_line() {
    printf 'install.sh: %s\n' "$1" >&2
    printf 'install.sh: sh %s --help says what it takes\n' "$self" >&2
    exit "$command_line_not_understood"
}

step() {
    printf 'install.sh: %s\n' "$1"
}

usage() {
    cat <<EOF
usage: sh $self --root <folder>
       sh $self --help

Builds satellite 004 with make, then installs satl, its libraries
(satellite-numbers/) and satl-term into <folder>, and proves the install by
running examples/hello_world.satl with the installed satl.

WHERE 004 INSTALLS IS NOT DECIDED YET (PLAN M0.5, D0.5.1), so <folder> is
required, and these are refused:

  - \$HOME/.satl, and anything inside it, and /usr/local: satellite 003's;
  - a folder on PATH, where a file named satl would change what \`satl\` runs;
  - the repository's top folder;
  - a folder holding a satl, satl-term or satellite-numbers/ this installer
    did not put there, which it knows from its record, .satellite-004-install;
  - --link, --desktop and --system, which are 003's installer's.

Nothing outside <folder> is written, 003's install is not touched, and no
shell startup file is edited. Run the installed satl by its path.

It exits 0 when the install is proven, 23 for a command line it does not take,
and 1 for anything else, with the reason.
EOF
}
