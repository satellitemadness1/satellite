# satellite -- the command line, and what a prefix has to look like.
#
# Reads the defaults from 010-defaults.sh and die()/usage() from
# 020-saying-things.sh, which is why both are sourced before this one.
#
# Moved out of the 634-line install.sh on 2026-08-24, byte for byte.

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
