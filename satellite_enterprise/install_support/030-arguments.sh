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
            refuse_command_line "$1 is satellite 003's installer's (old_versions/second_satellite/satellite_enterprise/install.sh). Where 004 installs is not decided yet (PLAN D0.5.1), so this one installs only into a --root, and does nothing outside it"
            ;;
        *)
            refuse_command_line "\"$1\" is not a word this installer takes"
            ;;
    esac
done

[ "$root_given" = yes ] || refuse_command_line "--root <folder> is required: where satellite 004 installs is not decided yet (PLAN D0.5.1)"
[ -n "$root" ] || refuse_command_line "--root was given an empty folder name"
