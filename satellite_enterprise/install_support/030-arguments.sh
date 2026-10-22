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
        --icons-current)
            # ARE satl'S ICON AND THE .satl FILE TYPE IN PLACE ALREADY? 0 when every file is, byte
            # for byte, and, where update-mime-database is on PATH, the mime database there knows
            # the type (icons_current, 010-defaults.sh), and 1 when not, printing nothing and
            # building and writing nothing: the bare make asks this before it asks the person
            # (050-build.mk, 2026-10-20), so someone who has them is not asked again.
            [ $# -eq 1 ] && [ "$root_given" = no ] || refuse_command_line "$1 is the whole command line"
            icons_current && exit 0
            exit 1
            ;;
        --link | --desktop | --system | --no-link | --prefix | --prefix=* | --uninstall | --static | --no-static)
            refuse_command_line "$1 is satellite 003's installer's (old_versions/second_satellite/satellite_enterprise/install.sh). This one installs into \$HOME/.satl or a --root, and for \$HOME/.satl alone it writes the launcher, and satl's icon and the .satl file type unless SATELLITE_ICONS=no, into \${XDG_DATA_HOME:-\$HOME/.local/share}"
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

# THE ICONS ARE SAID IN THE ENVIRONMENT (2026-10-20): SATELLITE_ICONS=no leaves them out -- the
# bare make's second question says n that way (050-build.mk) -- and unset, empty or yes puts them
# in, so `make install` and this script alone install them, asking nothing: the launcher they
# already install names the icon. Anything else is refused here, before make runs.
case ${SATELLITE_ICONS:-yes} in
    yes | no) icons_wanted=${SATELLITE_ICONS:-yes} ;;
    *) refuse_command_line "SATELLITE_ICONS=$SATELLITE_ICONS is not a word this installer takes: yes puts the icons in, and no leaves them out" ;;
esac
