# satellite -- the command line.
#
# Reads the defaults from 010-defaults.sh and die()/usage() from
# 020-saying-things.sh, which is why both are sourced before this one.

while [ $# -gt 0 ]; do
    case $1 in
        --root)
            root_given=yes
            [ $# -ge 2 ] || die "--root needs a directory"
            [ "$layout" = prefix ] && die "--root and --prefix are two layouts.
       --root DIR puts everything at the top of one directory; --prefix DIR
       writes bin/ and share/ underneath it, which is what an operating system
       expects. Give one."
            root=$2
            shift 2
            ;;
        --root=*)
            root_given=yes
            [ "$layout" = prefix ] && die "--root and --prefix are two layouts."
            root=${1#--root=}
            shift
            ;;
        # INSTALL INTO THE OPERATING SYSTEM. Both spellings set the same
        # layout; --system is the one to type, and --prefix is for a machine
        # where /usr/local is not the answer. --system after --prefix does not
        # clobber the directory that was given, because a command line is read
        # left to right by a person who meant both words.
        --system)
            [ "$layout" = prefix ] || root=$system_prefix
            layout=prefix
            bin_rel=bin/
            shift
            ;;
        --prefix)
            [ $# -ge 2 ] || die "--prefix needs a directory"
            layout=prefix
            bin_rel=bin/
            root=$2
            shift 2
            ;;
        --prefix=*)
            layout=prefix
            bin_rel=bin/
            root=${1#--prefix=}
            shift
            ;;
        --static)
            static=yes
            shift
            ;;
        --no-static)
            static=no
            shift
            ;;
        --link)
            link_bin=yes
            link_explicit=yes
            shift
            ;;
        --desktop)
            desktop=yes
            shift
            ;;
        # The off spellings are accepted as well as the on ones, so that a
        # command line written from memory works whichever way the reader
        # remembers the default. They are the default, so they do nothing.
        --no-link)
            link_bin=no
            link_explicit=yes
            shift
            ;;
        --no-desktop)
            desktop=no
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
[ -n "$root" ] || die "--root needs a directory"

# ABSOLUTE, for the same reason the first satellite's installer demanded it of a
# prefix: this path is written into a symlink in ~/.local/bin, and a relative
# one there would resolve against whatever directory the shell happened to be in
# when the link was followed, which is to say against a different directory
# every time.
case $root in
    /*) ;;
    *)  die "--root must be an absolute path, not \"$root\"" ;;
esac

# WHERE THE PROGRAMS END UP, derived once here rather than spelled out in the
# four fragments that need it. Both layouts are covered by one expression, which
# is the point of bin_rel being a string and not a boolean: the root layout puts
# the binaries at the top of $root and the prefix layout puts them in $root/bin,
# and everything downstream asks this variable instead of asking which layout it
# is in. 080-report.sh in particular VERIFIES the install by running these, so a
# path that was right for only one layout would have reported a successful
# install of a file it never checked.
#
# Set before the trailing-slash trim below would matter, and recomputed after it
# -- see the reassignment under that loop -- because "$root/" and "$root" differ
# only in the messages, and only one of them is worth printing.
_derive_paths() {
    installed_bindir=$root${bin_rel:+/${bin_rel%/}}
    installed_satl=$installed_bindir/satl
    installed_term=$installed_bindir/satl-term
    installed_cpu=$installed_bindir/satl-cpu-level
}

# A trailing slash installs identically and reads worse in every message below.
while :; do
    case $root in
        /) break ;;
        */) root=${root%/} ;;
        *) break ;;
    esac
done

# --root IS A REHEARSAL AND MUST NOT TOUCH THE HOME IT IS REHEARSING FOR.
# 010-defaults.sh turned --link on by default on 2026-08-28; user_bin in
# 070-desktop.sh is $HOME/.local/bin and is NOT derived from $root, so that flip
# would otherwise make `--root /tmp/scratch` write two symlinks into the real
# ~/.local/bin -- pointing into a scratch directory that a later `rm -rf` makes
# dangling, and which an --uninstall without the same --root no longer
# recognises as its own. --root's whole stated purpose is to be rehearsable
# "without writing into the home directory it is meant for", so the default is
# withdrawn when it is given. An explicit --link still wins, because someone who
# named both flags meant both.
if [ "$root_given" = yes ] && [ "$link_explicit" = no ]; then
    link_bin=no
fi

_derive_paths

# HOW TO SAY IT AFTERWARDS, in the past tense or the conditional. Derived here
# with the other option-derived values rather than at the top of 080-report.sh,
# where it used to live, because 075-system.sh reports too and is sourced first.
# Telling someone their files were installed when the whole point of -n was that
# they were not is the kind of small lie that costs somebody an hour later.
if [ "$dry_run" = yes ]; then
    did='would install'
    removed='would remove'
else
    did=installed
    removed=removed
fi

# REFUSED, rather than accepted and regretted. --root exists to rehearse this
# script somewhere harmless, and the way it goes wrong is a typo that names a
# directory full of somebody's files -- at which point --uninstall would go
# through that directory removing every name this script knows how to install.
# $HOME itself is the one that would hurt most and is one slip away from the
# default.
#
# THE LIST DEPENDS ON THE LAYOUT, and it has to: /usr/local is on it, and
# /usr/local is precisely where --system is FOR. The refusal was never about the
# directory being important, it was about --root treating a shared directory as
# though satellite owned it outright -- an --uninstall would then walk that
# directory removing every name this script knows. A prefix layout does not make
# that mistake by construction: it writes bin/satl and share/icons/... , which
# are named files in directories it shares and does not own, and 060's uninstall
# removes exactly those names and rmdirs only what it emptied.
if [ "$layout" = root ]; then
    case $root in
        "$HOME" | / | /home | /usr | /usr/local | /etc | /var | /opt)
            die "--root $root is a directory that belongs to something else.
       satellite installs into a directory of its own -- $HOME/.satl by
       default -- so that removing it removes satellite and nothing else.
       To install INTO one of these, you want the other layout:
           $(quoted "$self") --system              (that is --prefix /usr/local)
           $(quoted "$self") --prefix $root"
            ;;
    esac
else
    # / is still refused, because "$root/bin" is then /bin and "$root/share" is
    # /share, neither of which is a place anybody meant.
    case $root in
        /) die "--prefix / is not a prefix. Use --prefix /usr/local, which is
       what --system does." ;;
        /usr) cat <<EOF >&2
install.sh: note -- --prefix /usr writes into the directory your package
            manager believes it is authoritative over, so these files will be
            unknown to rpm and dpkg and will not be replaced or removed by
            them. /usr/local exists for exactly this and is what --system uses.
            Carrying on, because you named it.
EOF
            ;;
    esac
fi
