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
