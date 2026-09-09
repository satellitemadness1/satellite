# satellite -- the command line.
#
# FIVE OPTIONS AND NO REQUIRED ONE. Running this script with no arguments at all
# is the intended way to use it, and it is the case every default in
# 010-defaults.sh is chosen for: install everything, into ~/.local, for the
# person who typed it, with no password. Every flag below either moves that
# install somewhere else or declines to do it.
#
# THERE IS NO --desktop FLAG, and its absence is a decision rather than an
# omission. satellite_enterprise/install.sh has one, off by default, because
# installing the launcher and the icons also rebuilds three indexes that cover
# every application on the machine, and it judged that a fair thing to ask for
# and an unfair thing to assume. This package exists BECAUSE somebody wants the
# icon in their apps grid; making that opt-in would be shipping the reason for
# the package behind a flag most people would never find. The three indexes are
# rebuilt, and 080-report.sh names them so the machine-wide effect is visible
# rather than silent.

usage() {
    cat <<USAGE
usage: $me                                install for you, into ~/.local
       $me --system                        install for everyone, into /usr/local
       $me --prefix DIR                    install somewhere else
       $me --uninstall [--prefix DIR]      take it back out
       $me -n | --dry-run                  print what it would do, do nothing
       $me --help

Installs the satellite programming language on AlmaLinux 10: the interpreter
satl, the window satl-term, the artwork for both, the .satl file type, and the
launcher that puts satellite in your applications grid.

Nothing is compiled and nothing is downloaded -- the programs are already
built, in programs/ beside this script.

  --system       install into /usr/local instead of your home directory, so
                 that every account on the machine has satl. Needs a writable
                 prefix; this script never calls sudo itself, it checks and
                 tells you the command to run. Shorthand for
                 --prefix /usr/local.
  --prefix DIR   install into DIR, in the bin/ and share/ shape. The default is
                 \$HOME/.local, which is the only prefix that needs no password
                 and is still read by the desktop.
  --no-deps      do NOT run \`sudo dnf install\` for the system packages that
                 satl-term needs. They are installed by default, and that is
                 the only step that asks for a password. Declining it means
                 the window works only if those libraries are already here.
  --no-path      do NOT add the install directory to PATH in ~/.bashrc. It is
                 added by default, between two markers, so that \`satl\` works
                 in any terminal window. The icon in the applications grid
                 works either way -- the launcher holds a full path.
  --uninstall    remove every path this script installs, the directories it
                 created that are left empty, and the block it added to
                 ~/.bashrc. Give it the same --prefix you installed with. It
                 does not remove the hicolor index.theme, which other
                 applications' icons need too, and it does not remove the
                 packages dnf installed -- other programs may want them.
  -n, --dry-run  print every command that would change the machine, indented,
                 and run none of them. The transcript is exact and can be
                 pasted back into a shell.
  --help         this.
USAGE
}

# THE LOOP IS A while/case AND NOT getopts, because getopts does not do long
# options and every option here is long except one. Written out rather than
# clever: an installer is read by people who are deciding whether to run it.
while [ $# -gt 0 ]; do
    case $1 in
        --system)
            # /usr/local AND NOT /usr. /usr belongs to the package manager, and
            # a file this script writes there is a file dnf does not know about
            # and will happily overwrite or leave behind. /usr/local is the
            # directory the Filesystem Hierarchy Standard sets aside for exactly
            # this -- software installed by the administrator rather than by the
            # distribution -- and it is on XDG_DATA_DIRS on every desktop, so
            # the launcher and the icons are found there.
            prefix=/usr/local
            ;;
        --prefix)
            [ $# -ge 2 ] || die "--prefix needs a directory after it."
            prefix=$2
            shift
            ;;
        --prefix=*)
            prefix=${1#--prefix=}
            [ -n "$prefix" ] || die "--prefix= needs a directory after the =."
            ;;
        --no-deps)
            # DECLINES THE ONE STEP THAT ASKS FOR A PASSWORD. The libraries
            # satl-term needs are not installed and not checked for by dnf;
            # 040-machine.sh still asks ldd whether they are there, so a machine
            # that already has them is unaffected and one that does not gets the
            # interpreter plus an explanation.
            install_deps=no
            ;;
        --no-path)
            # DECLINES THE EDIT TO ~/.bashrc. The programs are still installed
            # and the launcher still holds an absolute path, so the icon in the
            # apps grid works either way; what is given up is `satl` working as
            # a bare word in a terminal.
            edit_shell_rc=no
            ;;
        --uninstall)
            action=uninstall
            ;;
        -n|--dry-run)
            dry_run=yes
            ;;
        --help|-h)
            # usage() writes to stdout and exits 0, because here it was ASKED
            # for. The same text goes to stderr with a non-zero status below,
            # where it is a complaint. Getting this backwards is why `--help |
            # less` sometimes shows nothing.
            usage
            exit 0
            ;;
        --)
            shift
            break
            ;;
        -*)
            usage >&2
            die "unknown option: $1"
            ;;
        *)
            usage >&2
            die "this script takes no arguments, only options: $1"
            ;;
    esac
    shift
done

[ $# -eq 0 ] || die "this script takes no arguments, only options: $1"

# THE PREFIX IS MADE ABSOLUTE HERE, ONCE, and every path in the install tree is
# derived from it afterwards. A relative --prefix would otherwise be resolved
# against whatever directory each command happened to run in, and the uninstall
# would be resolved against a different one on a different day.
#
# THE DIRECTORY MAY NOT EXIST YET, which is why this cannot simply be `cd "$1"
# && pwd`: --prefix ~/opt/satellite on a machine with no ~/opt is a perfectly
# ordinary request. So the deepest EXISTING ancestor is resolved and the rest is
# appended to it, which canonicalises the part that can be canonicalised and
# leaves the part that cannot alone.
absolute_path() {
    _p=$1
    case $_p in
        /*) ;;
        *) _p=$PWD/$_p ;;
    esac

    _tail=
    while [ ! -d "$_p" ]; do
        _leaf=$(basename -- "$_p")
        _p=$(dirname -- "$_p")
        _tail=$_leaf${_tail:+/$_tail}
        # dirname bottoms out at / or . and would spin forever otherwise.
        case $_p in
            /|.) break ;;
        esac
    done

    if [ -d "$_p" ]; then
        _p=$(cd -- "$_p" && pwd)
    fi
    printf '%s\n' "$_p${_tail:+/$_tail}"
}

prefix=$(absolute_path "$prefix")
bindir=$prefix/bin
datadir=$prefix/share
