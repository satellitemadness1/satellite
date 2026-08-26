# satellite -- the command line.
#
# Reads the defaults from 010-defaults.sh and die()/usage() from
# 020-saying-things.sh, which is why both are sourced before this one.

while [ $# -gt 0 ]; do
    case $1 in
        --root)
            [ $# -ge 2 ] || die "--root needs a directory"
            root=$2
            shift 2
            ;;
        --root=*)
            root=${1#--root=}
            shift
            ;;
        --link)
            link_bin=yes
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

# A trailing slash installs identically and reads worse in every message below.
while :; do
    case $root in
        /) break ;;
        */) root=${root%/} ;;
        *) break ;;
    esac
done

# REFUSED, rather than accepted and regretted. --root exists to rehearse this
# script somewhere harmless, and the way it goes wrong is a typo that names a
# directory full of somebody's files -- at which point --uninstall would go
# through that directory removing every name this script knows how to install.
# $HOME itself is the one that would hurt most and is one slip away from the
# default.
case $root in
    "$HOME" | / | /home | /usr | /usr/local | /etc | /var | /opt)
        die "--root $root is a directory that belongs to something else.
       satellite installs into a directory of its own -- $HOME/.satl by
       default -- so that removing it removes satellite and nothing else."
        ;;
esac
