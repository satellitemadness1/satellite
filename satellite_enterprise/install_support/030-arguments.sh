# satellite 004 -- the command line.
#
# --root takes the next word literally, so a folder named -x or with a space or a
# newline in it is still a folder. --root=<folder> is the same.

root_given=no
while [ $# -gt 0 ]; do
    case $1 in
        --root)
            [ $# -ge 2 ] || refuse_command_line "--root needs a folder after it"
            [ "$root_given" = no ] || refuse_command_line "--root is given twice"
            root=$2
            root_given=yes
            shift 2
            ;;
        --root=*)
            [ "$root_given" = no ] || refuse_command_line "--root is given twice"
            root=${1#--root=}
            root_given=yes
            shift
            ;;
        --help | -h)
            [ $# -eq 1 ] && [ "$root_given" = no ] || refuse_command_line "$1 is the whole command line"
            usage
            exit 0
            ;;
        --link | --desktop | --system | --no-link | --prefix | --prefix=* | --uninstall | --static | --no-static)
            refuse_command_line "$1 is satellite 003's installer's (old_versions/second_satellite/satellite_enterprise/install.sh). This one installs into \$HOME/.satl or a --root, and does nothing outside it"
            ;;
        *)
            refuse_command_line "\"$1\" is not a word this installer takes"
            ;;
    esac
done

# NO --root IS $HOME/.satl (D0.5.1, ruled 2026-09-22).
if [ "$root_given" = no ]; then
    [ -n "${HOME:-}" ] || refuse_command_line "HOME is not set, so there is no \$HOME/.satl to install into; name a folder with --root"
    root=$HOME/.satl
fi
[ -n "$root" ] || refuse_command_line "--root was given an empty folder name"
