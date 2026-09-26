# satellite -- the launcher, the app icon, and the icon on every .satl file.
#
# LAST, and it is the tail of the script: the two notes here are the last thing
# printed. Everything in it is advice -- the interpreter is unaffected either
# way, which the text says out loud.
#
# Moved out of the 634-line install.sh on 2026-08-24, byte for byte.

# The desktop half of the install -- the launcher, the app icon, and the icon on
# every .satl file -- has one prerequisite that a prefix outside /usr cannot
# satisfy on its own, and it fails silently.
#
# An icon directory is only a THEME if it contains index.theme. That file
# belongs to the hicolor-icon-theme package, which installs it into the system
# prefix and nowhere else, so $prefix/share/icons/hicolor is a directory full of
# correctly named PNGs that GTK will not look inside: has_icon() answers false
# for every one of them, at every size, and the shell falls back to a generic
# page. gtk-update-icon-cache does not reveal this -- the Makefile runs it with
# -t, which means --ignore-theme-index, so the cache builds successfully over a
# theme that is not yet a theme.
#
# This is reported rather than repaired, for the reason at the top of this file:
# index.theme describes hicolor, not satellite. Copying one in would put a file
# outside the install tree, owned by no package, that `--uninstall` must then
# either orphan or delete out from under whatever else has since installed an
# icon beside ours. Neither is a decision an installer gets to make quietly, and
# the fix is one line the user can read before running it.
# An icon directory is only a theme if it holds index.theme, and GTK ignores one
# that does not: every icon just installed is invisible, at every size. The
# install target copies the system's in when the prefix has none, which covers
# every ordinary case -- including /usr/local, which has no index.theme of its
# own and is this script's default, so the earlier version of this check
# skipping anything under /usr was skipping the commonest install of all.
#
# Reaching this branch therefore means the copy could not happen: no
# hicolor-icon-theme on the machine, or a prefix the install could not write
# that far into. Reported rather than repaired, because at that point there is
# no correct file to put there and inventing one would be declaring someone
# else's theme.
hicolor=$prefix/share/icons/hicolor
if [ -d "$hicolor" ] && [ ! -f "$hicolor/index.theme" ]; then
    cat <<EOF

install.sh: note — $hicolor has no index.theme,
            so the desktop will not find the icons that were just installed
            there. That file ships with hicolor-icon-theme; this machine
            appears not to have it. Installing that package, or copying an
            index.theme in by hand, is what makes the icons visible:

                gtk-update-icon-cache -qtf $(quoted "$hicolor")

            The interpreter is unaffected either way; this is only about the
            launcher and the icon shown on .satl files.
EOF
fi

# GNOME reads the desktop, icon and mime indexes once at session start. The
# Makefile has already rebuilt all three, so a new session picks everything up
# with no further action -- but the CURRENT one will not, and an icon that is
# correct on disk and absent on screen reads as an install that failed.
printf 'install.sh: the launcher and the .satl file icon appear at your next\n'
printf '            login; the indexes they come from are already rebuilt.\n'
