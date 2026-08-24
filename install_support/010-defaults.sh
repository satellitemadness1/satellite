# satellite -- the default prefix, and the four things a run can be told.
#
# Sourced first by install.sh. Sets the variables every fragment after this one
# reads; runs nothing. See install.sh for the whole arrangement.
#
# Moved out of the 634-line install.sh on 2026-08-24, byte for byte.

# The default prefix follows who is running the script, because there is only
# ever one right answer and it is decided by that.
#
# /usr/local is the correct place for an install from source, and it needs root.
# Defaulting to it unconditionally meant that plain `./install.sh` -- the
# command anyone tries first -- could not succeed for a normal user: it printed
# a permission error and the two flags to choose between, and installed
# nothing. Nobody who ran it wanted that outcome. Someone with root wants the
# system install; someone without wants the one that needs no privileges, and
# ~/.local is on PATH and in the XDG data search path on any current desktop, so
# it works with no further setup.
#
# --prefix still wins over both, which is what keeps this a default rather than
# a policy, and the chosen value is printed before anything is written so it is
# never a surprise. This does NOT run sudo: escalating on the user's behalf is
# the thing the note further down refuses to do, and choosing a writable
# directory instead is the opposite of that, not a version of it.
if [ "$(id -u)" = 0 ]; then
    prefix=/usr/local
else
    prefix=${HOME:?HOME is not set, so there is no home prefix to default to}/.local
fi
# DESTDIR is read from the environment rather than given a flag because that is
# the interface every packaging system already drives: `DESTDIR=$PWD/stage
# ./install.sh`. It stages a tree and is never baked into a path or a binary,
# which is why it is not part of the writability advice below — staging into a
# scratch directory has nothing to do with whether /usr/local is yours.
destdir="${DESTDIR:-}"
action=install
dry_run=no
: "${MAKE:=make}"
