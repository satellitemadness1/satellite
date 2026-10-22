# satellite 004 -- where things are.
#
# Found relative to install.sh, never to the caller's directory, so `sh
# /elsewhere/satellite/satellite_enterprise/install.sh --root x` installs the
# /elsewhere tree.

build=$repo/build

# The record this installer writes in every root it installs into. Its name
# says which satellite wrote it, so a 005 installer can tell.
record_name=.satellite-004-install

# What examples/hello_world.satl prints, which is how the install is proven.
# check.sh holds the same four lines.
hello_wanted='Hello, World!
a // inside a string is not a comment
42
true'

# Machine codes (satellite/machine/machine_codes.hpp): a command line this
# installer does not take is 23, as it is for satl. Anything else
# that stops it is 1.
command_line_not_understood=23

MAKE=${MAKE:-make}
root=

# satl'S ICON AND THE .satl FILE TYPE (2026-10-20), which 072-icons.sh puts in place. LISTED
# HERE, ONCE, because two things read the list: 072, which installs them, and --icons-current
# (030-arguments.sh), which the bare make asks before it asks the person, and which runs before
# anything else does. They go into the desktop's folder for one person,
# ${XDG_DATA_HOME:-$HOME/.local/share} -- the one 070-desktop.sh writes the launcher into --
# and icons_data is empty when neither is set.
#
# NINE SIZES, A LIST AND NOT A find (003's 060-install-tree.sh): a size the tree has lost is a
# file that is not current, and is said, rather than an icon quietly missing at one size.
#
# THE SVG IS NOT INSTALLED, and must not be (003 said so, and it was measured 2026-10-20 with
# GTK 4.18): icons/org.satellite.terminal.svg is an older line drawing, not the photograph the
# PNGs carry, and hicolor's scalable/apps answers every size from 1 to 256. Installed beside
# the PNGs, GTK drew the drawing at most sizes up to 192 pixels -- 40, 96 and 192 among them,
# and 48 on a doubled screen -- and the photograph only at and near the nine, and above 192:
# two pictures of one program, by size.
icon_sizes='16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256 512x512'
icons_data=${XDG_DATA_HOME:-${HOME:+$HOME/.local/share}}

# Calls $1 with each file's place in the tree and its place under $icons_data, stopping at the
# first that answers no. The pictures keep their hicolor/<size>/<kind>/ paths on both sides;
# the type's packet goes where update-mime-database reads one person's packets.
each_icon_file() {
    for _size in $icon_sizes; do
        for _icon in apps/org.satellite.terminal.png mimetypes/application-x-satellite.png; do
            "$1" "$here/icons/hicolor/$_size/$_icon" "$icons_data/icons/hicolor/$_size/$_icon" || return 1
        done
    done
    "$1" "$here/icons/application-x-satellite.xml" "$icons_data/mime/packages/application-x-satellite.xml"
}

same_file() { cmp -s -- "$1" "$2"; }

# CURRENT: every file there, byte for byte the tree's, and the mime database there built with
# the type in it -- on a machine that has update-mime-database, which is what builds it, so a
# machine without one is not asked about the icons on every make. Prints nothing.
icons_current() {
    [ -n "$icons_data" ] && each_icon_file same_file || return 1
    ! command -v update-mime-database > /dev/null 2>&1 || [ -f "$icons_data/mime/application/x-satellite.xml" ]
}
