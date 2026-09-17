#!/bin/sh
# install.sh -- install satellite 004 (satl, its libraries and satl-term) into a
# folder you name.
#
#     sh satellite_enterprise/install.sh --root <folder>
#
# WHERE 004 INSTALLS IS NOT DECIDED YET (PLAN M0.5, D0.5.1), so this installs
# ONLY into a --root it is given, and refuses the places that already mean
# something: $HOME/.satl and /usr/local, which are satellite 003's; any folder on
# PATH, where a file named satl would change what the word `satl` runs; the
# repository's top folder, which is on the author's PATH; and a root holding a satl
# this installer did not put there. It refuses 003's --link, --desktop and
# --system rather than half-doing them. 003's installer is unchanged in
# old_versions/second_satellite/satellite_enterprise/, and 003's install is not
# touched by this one.
#
# WHAT IT INSTALLS, together, because each finds the others beside its own path:
#
#     <root>/satl                   the interpreter
#     <root>/satellite-numbers/     every numbered library, which satl loads
#     <root>/satl-term              the window, when this machine could build it
#     <root>/.satellite-004-install what this installer put there, and each file's sha256
#
# The record is how a later install knows a satl in the root is its own: it is
# never learnt by running the file.
#
# satellite-numbers/ IS WRITTEN AS A FRESH FOLDER AND RENAMED INTO PLACE, so a
# library a word no longer has cannot survive an install: satl loads every .so
# it finds there.
#
# THE INSTALL IS PROVEN by running the installed satl on
# examples/hello_world.satl, which loads every library -- never with --version,
# which answers before any library loads.
#
# POSIX sh (003's rule): the installer is the program that runs before anything
# is installed. THIS FILE IS AN INDEX; the fragments below are sourced in order,
# and the order is the script:
#
#     010-defaults.sh ....... where things are, and what hello_world says
#     020-saying-things.sh .. die, refuse, usage
#     030-arguments.sh ...... the command line
#     040-root.sh ........... the root, and every root that is refused
#     050-building.sh ....... make
#     060-install-tree.sh ... the copy, the rename and the record
#     080-report.sh ......... the proof and the title lines
#
# NOT PORTED from 003: 040-machine.sh (the Enterprise Linux checks and the
# --system prefix), 070-desktop.sh (the ~/.local links, launcher, icons and .satl
# type) and 075-system.sh -- a 004 launcher under org.satellite.terminal would
# take 003's (D0.5.1). The artwork in icons/ and icon_artwork/ is 003's and stays
# as it is.

set -eu

self=$0
here=$(dirname -- "$self")
here=$(cd -- "$here" && pwd)
repo=$(cd -- "$here/.." && pwd)

support=$here/install_support
if [ ! -d "$support" ]; then
    printf 'install.sh: %s\n' "install_support/ is not beside $self. Copy the whole satellite_enterprise/ folder." >&2
    exit 1
fi

. "$support/010-defaults.sh"
. "$support/020-saying-things.sh"
. "$support/030-arguments.sh"
. "$support/040-root.sh"
. "$support/050-building.sh"
. "$support/060-install-tree.sh"
. "$support/080-report.sh"
