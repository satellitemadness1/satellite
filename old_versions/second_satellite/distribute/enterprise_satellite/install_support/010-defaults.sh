# satellite -- where the install goes, and the names that must not drift.
#
# READ FIRST, BEFORE ANY OPTION IS PARSED, so that 030-arguments.sh has
# something to override and --help has something to print. Every path this
# script writes is derived from $prefix, and $prefix has exactly one default.

# ~/.local, AND THE REASON IS THAT IT IS THE ONLY PREFIX THAT NEEDS NO PASSWORD
# AND IS STILL READ BY THE DESKTOP. Those two properties do not usually come
# together and here they do: the XDG base directory spec makes
# ~/.local/share the user's data directory, and GNOME searches it for
# applications, icons and mime packets exactly as it searches /usr/share. So an
# install that touches nothing outside $HOME still puts an icon in the apps
# grid, which is the whole point of this package.
#
# ~/.local/bin IS NOT IN THE XDG SPEC -- the spec defines no directory for
# executables at all -- and is used anyway, because it is the de-facto standard
# and because AlmaLinux's own /etc/skel/.bashrc puts it on PATH at lines 9-10.
# That is a FILE THE SHELL READS AND A DESKTOP DOES NOT, which is why
# 070-desktop.sh writes an absolute path into the launcher and why
# 080-report.sh checks PATH separately and reports on it. The two halves of
# "typing satl works" and "clicking the icon works" have different causes here
# and are verified separately for that reason.
#
# NOT $HOME/.satl, WHICH IS WHAT THE OTHER TWO INSTALLERS USE. That is a private
# root with symlinks pointing out of it into ~/.local, and it earns its keep on
# a machine where an older satellite already owns those names: a symlink can be
# recognised as ours and a copied file cannot, so the other installers can
# refuse to overwrite what they did not create. This package is for a machine
# that has never had satellite on it. Real files in one standard prefix are
# fewer moving parts, and 060-install-tree.sh can still remove exactly what it
# put there because it removes what it DECLARES rather than what it finds.
prefix=$HOME/.local

# THE FOUR SUBDIRECTORIES ARE DERIVED AND NOT CONFIGURABLE, because the desktop
# specifications say where each of them is relative to a data directory, and a
# flag to move one would be a flag to break it. They are named here rather than
# spelled out at each use so that --prefix moves all of them together.
bindir=$prefix/bin
datadir=$prefix/share

# THE NINE SIZES. Not a wildcard over the package: this is the list the artwork
# is expected to have, so a package that is missing a size fails 050-payload.sh
# by name instead of installing eight sizes and looking fine.
#
# 16 through 512 with 22 and 24 among them, which look like odd sizes to keep
# and are the ones a GTK file manager asks for in a list view and a tree view.
# The set matches ../../satellite_enterprise/icons/hicolor exactly and is
# copied from there by make-package.sh.
icon_sizes='16x16 22x22 24x24 32x32 48x48 64x64 128x128 256x256 512x512'

# THE TWO NAMES THAT MUST NOT DRIFT, and they are spelled once here and used
# everywhere else through these variables.
#
# org.satellite.terminal is the GApplication id that src/window/window.cpp
# registers. GTK puts that id on the toplevel -- as the Wayland app_id -- and a
# desktop shell then looks for <that id>.desktop, so the launcher file name, the
# StartupWMClass inside it, and the Icon= inside it all have to be this string
# or the window arrives with no launcher and no icon.
#
# application-x-satellite is the mime type application/x-satellite with the '/'
# replaced by '-'. update-mime-database indexes the packet under the name it is
# installed as, and a file manager constructs the icon name from the type by the
# same substitution, so the packet file name and the icon file name are both
# this string by construction rather than by accident. The XML explains the rest
# and its comments are worth reading before touching any of it.
app_id=org.satellite.terminal
mime_name=application-x-satellite

# WHAT THE PACKAGE HOLDS, relative to the directory install-satellite.sh is in.
# $here is set by that script before this fragment is sourced.
payload_programs=$here/programs
payload_share=$here/share
payload_example=$here/example

# Set by 030-arguments.sh, declared here so `set -u` cannot fire on a code path
# that reads one before the command line has been parsed. install unless
# --uninstall; do the work unless -n.
action=install
dry_run=no

# THE TWO THAT REACH OUTSIDE THE PREFIX, and both are ON by default.
#
# That is a deliberate reversal of what the other two installers in this tree
# do, made at the author's instruction for this package and its audience. Their
# defaults are for somebody who wants to know exactly what an installer touched;
# these are for somebody who wants satellite to work when it finishes. Both have
# an off switch, both announce every command before running it, and both are
# undone by --uninstall.
#
#   install_deps    run `sudo dnf install` for the packages satl-term needs.
#                   035-dependencies.sh, and it is the only thing here that
#                   asks for a password.
#   edit_shell_rc   add $bindir to PATH in ~/.bashrc, between two markers, so
#                   that `satl` works in any terminal window. 075-shell-path.sh.
install_deps=yes
edit_shell_rc=yes

# Set by 040-machine.sh and 050-payload.sh, for the same reason.
have_term=unknown
cpu_variant=unknown
