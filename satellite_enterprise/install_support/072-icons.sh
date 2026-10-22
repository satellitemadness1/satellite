# satellite 004 -- satl's icon and the .satl file type, for $HOME/.satl only.
#
# THE AUTHOR, 2026-10-20: "CAn you make a plain "make" ask if you want to install the icons?
# There's an icon for the .satl filetype, and there's an icon for the satl program too
# somewhere, I dunno where though..." Both are in icons/ beside this folder, and were all
# along: hicolor/<size>/apps/org.satellite.terminal.png, the photograph the launcher's
# Icon=org.satellite.terminal names, and hicolor/<size>/mimetypes/application-x-satellite.png,
# the page a .satl file is drawn as -- with application-x-satellite.xml, the packet that makes
# a .satl file a type of its own for that page to belong to. No 004 install had put down any
# of them: the launcher named an icon that was not there, and a .satl file was plain text.
#
# WHICH FILES, AND WHERE THEY GO, IS 010-defaults.sh's LIST (each_icon_file), because
# --icons-current reads it before anything else runs. They go into
# ${XDG_DATA_HOME:-$HOME/.local/share}, THE DESKTOP'S FOLDER FOR ONE PERSON: icons/hicolor/ is
# the theme every icon theme falls back to, and mime/packages/ is where update-mime-database
# reads a person's own types -- no sudo, no system folder, and the folder 070-desktop.sh puts
# the launcher in. FOR $HOME/.satl ONLY, as the launcher is: they belong to the satl the
# launcher starts, and a --root install is a second satl that nothing on the desktop starts.
#
# PORTED FROM 003 (old_versions/second_satellite/satellite_debian/install_support/
# 070-desktop.sh, measured on Debian 13 on 2026-08-28), as copies where 003 made links into
# its root's share/: 004's root has no share/ to point into.
#
# COMPARED BEFORE ANYTHING IS WRITTEN: when every file is there already, byte for byte, and,
# where update-mime-database is on PATH, the mime database knows the type (icons_current),
# nothing is written, no tool runs, and one line says so. Otherwise each file that differs is
# written beside itself and renamed into place, so a desktop reading it mid-install finds the
# old picture or the new one, never half of one.
#
# THE TYPE IS PROVEN, NOT ASSUMED. update-mime-database rebuilds the database from every packet
# in mime/packages/, and one it cannot parse is skipped with a message and EXIT 0 -- the
# packet's own comment says so, and it was measured again on 2026-10-20: the compiled
# mime/application/x-satellite.xml and the type's line in mime/types both went. So that
# compiled file is looked for afterwards, and its absence is said.
#
# THE ICON CACHE IS NOT MADE WHERE THERE WAS NONE, and here 004 parts from 003, which always
# made one. GTK trusts an icon-theme.cache that is not older than the theme's folder, and a
# file arriving in a size folder does not change that folder: measured 2026-10-20 with GTK
# 4.18, an icon another program put in hicolor/48x48/apps/ after a cache was built over ours
# was image-missing, until the theme folder was newer than the cache. So this does what
# xdg-icon-resource does (xdg-utils, from freedesktop.org): the theme folder is TOUCHED,
# which makes any cache there out of date, so GTK reads the folders themselves; and a cache
# that was already there is rebuilt with ours in it, by gtk4-update-icon-cache or
# gtk-update-icon-cache -- Debian ships both names, from one package (003 measured it) --
# with -q, quiet; -t, because one person's hicolor/ has no index.theme of its own; and -f, to
# rebuild a cache that looks up to date.
#
# update-desktop-database IS NOT RUN HERE: 070-desktop.sh runs it on every install, after it
# writes the launcher whose MimeType= names this type, and it reads the launcher, not the
# type -- the menu's mimeinfo.cache named application/x-satellite before the type existed.
#
# A MISSING TOOL IS SAID, AND SO IS A FILE THAT COULD NOT BE WRITTEN; NEITHER UNDOES THE
# INSTALL (070-desktop.sh's rule): satl runs by its path whatever its icons are.

# Writes $2 from $1, beside itself and renamed into place, when the two differ. Counts what it
# wrote and what it could not, and never stops: one file that fails leaves the rest to try.
# NOT EVEN BY ITS rm (the review, 2026-10-20): set -e is live for the packet, each_icon_file's
# last call, and an rm that failed there -- in a packages/ that cannot be searched or written,
# or on a folder named like its .installing -- ended the install, with no reason and no proof.
# _icon_packet is yes when the packet was written, failed when it could not be -- then there is
# no packet for update-mime-database to take -- and no when it was there already.
put_icon_file() {
    _icons_all=$((_icons_all + 1))
    same_file "$1" "$2" && return 0
    _put=failed
    if [ ! -f "$1" ]; then
        [ -n "$_icons_first_failure" ] || _icons_first_failure="$1, which is not in the tree"
    elif mkdir -p -- "${2%/*}" 2> /dev/null && cp -- "$1" "$2.installing" 2> /dev/null &&
        mv -fT -- "$2.installing" "$2" 2> /dev/null; then
        _put=yes
        _icons_written=$((_icons_written + 1))
    else
        rm -f -- "$2.installing" 2> /dev/null || :
        [ -n "$_icons_first_failure" ] || _icons_first_failure=$2
    fi
    [ "$_put" = yes ] || _icons_failed=$((_icons_failed + 1))
    case $2 in
        *.xml) _icon_packet=$_put ;;
        *) [ "$_put" = failed ] || _icon_pictures=yes ;;
    esac
    return 0
}

if [ "$home_root" != yes ]; then
    step "no icons for --root: they go in with \$HOME/.satl's install alone, as the launcher does"
elif [ "$icons_wanted" = no ]; then
    step "left out satl's icon and the .satl file type, as SATELLITE_ICONS=no says; without it, make install puts them in"
elif icons_current; then
    step "satl's icon and the .satl file type are already installed in $icons_data"
else
    _icons_all=0 _icons_written=0 _icons_failed=0 _icons_first_failure= _icon_pictures=no _icon_packet=no
    each_icon_file put_icon_file
    if [ "$_icons_failed" -gt 0 ]; then
        step "could not install $_icons_failed of the $_icons_all icon files, the first $_icons_first_failure; satl is installed, and runs, without them"
    elif [ "$_icons_written" = "$_icons_all" ]; then
        step "installed satl's icon, and the .satl file type with its icon, into $icons_data"
    elif [ "$_icons_written" -gt 0 ]; then
        step "brought satl's icon and the .satl file type up to date in $icons_data: $_icons_written of their $_icons_all files had changed"
    else
        step "satl's icon and the .satl file type were in place in $icons_data; the mime database there is rebuilt to know the type"
    fi

    _hicolor=$icons_data/icons/hicolor
    if [ "$_icon_pictures" = yes ]; then
        touch -c -- "$_hicolor" 2> /dev/null || :
        if [ -f "$_hicolor/icon-theme.cache" ]; then
            _cache_tool=
            for _tool in gtk4-update-icon-cache gtk-update-icon-cache; do
                if command -v "$_tool" > /dev/null 2>&1; then
                    _cache_tool=$_tool
                    break
                fi
            done
            if [ -z "$_cache_tool" ]; then
                step "no gtk-update-icon-cache here (Debian: gtk-update-icon-cache) to rebuild $_hicolor/icon-theme.cache; it is out of date now, so GTK reads the folders instead"
            elif ! "$_cache_tool" -qtf -- "$_hicolor"; then
                step "$_cache_tool could not rebuild $_hicolor/icon-theme.cache; it is out of date now, so GTK reads the folders instead"
            fi
        fi
    fi

    _mime=$icons_data/mime
    if [ "$_icon_packet" = yes ] || { [ "$_icon_packet" = no ] && [ ! -f "$_mime/application/x-satellite.xml" ]; }; then
        if ! command -v update-mime-database > /dev/null 2>&1; then
            step "no update-mime-database here (Debian: shared-mime-info), so a .satl file is not a type of its own yet"
        elif ! update-mime-database -- "$_mime" || [ ! -f "$_mime/application/x-satellite.xml" ]; then
            step "update-mime-database did not take $_mime/packages/application-x-satellite.xml (it says why above), so a .satl file is not a type of its own"
        fi
    fi
    unset _icons_all _icons_written _icons_failed _icons_first_failure _icon_pictures _icon_packet \
        _put _hicolor _cache_tool _tool _mime _size _icon
fi
