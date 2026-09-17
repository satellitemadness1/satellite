# satellite 004 -- die, refuse, and the usage.
#
# EVERY MESSAGE GOES THROUGH shown(), because a root's name and a word on the
# command line are the user's bytes: ESC ] 2 ; BEL in a folder name retitles the
# terminal the moment it is printed (DESIGN §9). The escapes are satl's own
# (satellite/machine/shown.hpp): \x1b for a control byte or a byte that is not
# UTF-8, \u{009b} for a C1 control or a direction control. gawk in the C locale,
# so it counts bytes; RS='^$' reads the whole word, newlines included.

shown() {
    printf '%s' "$1" | LC_ALL=C awk -v RS='^$' '
    BEGIN { for (i = 1; i < 256; i++) byte[sprintf("%c", i)] = i }
    {
        n = length($0); out = ""; i = 1
        while (i <= n) {
            c = substr($0, i, 1); b = byte[c]
            if (b < 32 || b == 127) { out = out sprintf("\\x%02x", b); i++; continue }
            if (b < 128) { out = out c; i++; continue }
            len = (b >= 194 && b <= 223) ? 2 : (b >= 224 && b <= 239) ? 3 : (b >= 240 && b <= 244) ? 4 : 0
            ok = len > 0 && i + len - 1 <= n
            cp = (len == 2) ? b % 32 : (len == 3) ? b % 16 : b % 8
            for (k = 1; ok && k < len; k++) {
                next_byte = byte[substr($0, i + k, 1)]
                if (next_byte < 128 || next_byte > 191) ok = 0; else cp = cp * 64 + next_byte % 64
            }
            if (ok && ((len == 3 && cp < 2048) || (len == 4 && cp < 65536) || (cp >= 55296 && cp <= 57343) || cp > 1114111)) ok = 0
            if (!ok) { out = out sprintf("\\x%02x", b); i++; continue }
            if ((cp >= 128 && cp <= 159) || cp == 1564 || cp == 8206 || cp == 8207 || (cp >= 8234 && cp <= 8238) || (cp >= 8294 && cp <= 8297))
                out = out sprintf("\\u{%04x}", cp)
            else
                out = out substr($0, i, len)
            i += len
        }
        printf "%s", out
    }'
}

die() {
    printf 'install.sh: %s\n' "$(shown "$1")" >&2
    exit 1
}

# A command line this installer does not take: the reason, where to read more, 23.
refuse_command_line() {
    printf 'install.sh: %s\n' "$(shown "$1")" >&2
    printf 'install.sh: sh %s --help says what it takes\n' "$(shown "$self")" >&2
    exit "$command_line_not_understood"
}

step() {
    printf 'install.sh: %s\n' "$(shown "$1")"
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
