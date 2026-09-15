# satellite -- die, say, and the one function that prints a command before
# running it.
#
# EVERY COMMAND THAT CHANGES THE MACHINE GOES THROUGH run(), which is what makes
# --dry-run a complete and honest transcript rather than an approximation of
# one. quoted() is why that transcript can be pasted back into a shell and do
# the same thing. Both are taken from satellite_enterprise/install_support
# unchanged, because they were right there and the argument for them does not
# move at a package boundary.

die() {
    printf '%s: %s\n' "$me" "$1" >&2
    exit 1
}

# The name this script was invoked as, for every message. Not hardcoded: someone
# who renames the file should see the name they typed.
me=$(basename -- "$self")

# say() exists so that the report in 080 and the progress here are the same
# shape, and so that a future --quiet has one place to be implemented.
say() {
    printf '%s\n' "$*"
}

# QUOTING FOR REPLAY, not for display. A path with a space in it is the common
# case that breaks a transcript -- /run/media/mom/My USB/ is a real directory
# name -- and single quotes with the embedded-quote dance are the only form that
# is safe for every byte a filename may contain.
quoted() {
    for _arg in "$@"; do
        case $_arg in
            *[!A-Za-z0-9._/=@:+-]*)
                printf " '%s'" "$(printf '%s' "$_arg" | sed "s/'/'\\\\''/g")"
                ;;
            *)
                printf ' %s' "$_arg"
                ;;
        esac
    done
}

# run() -- print it, then do it, unless --dry-run said only to print it.
#
# THE LEADING SPACE quoted() emits is why the format string here has none. A
# transcript line is indented by four so that it reads as a command inside the
# output rather than as more prose.
run() {
    if [ "$dry_run" = yes ]; then
        printf '   %s\n' "$(quoted "$@")"
        return 0
    fi
    "$@"
}

# install_file() -- one copy, with the mode set, and the parent made first.
#
# `install -D` WOULD BE ONE COMMAND AND IS NOT USED. coreutils' install has -D,
# BSD's does not, and the busybox one takes it and ignores the mode; this script
# is POSIX sh precisely so that it does not have to be right about which
# userland it landed in. Two commands that exist everywhere beat one that mostly
# does.
#
# THE COPY IS NOT `cp -p`. Timestamps and ownership from the build machine are
# not facts about the installed file, and preserving them makes a reinstall look
# like it did nothing. The mode is set explicitly on the next line instead,
# because that IS a fact about the installed file: 755 for a program, 644 for
# artwork, and never whatever umask the package was unpacked under.
install_file() {
    _src=$1
    _dst=$2
    _mode=$3

    _dir=$(dirname -- "$_dst")
    [ -d "$_dir" ] || run mkdir -p "$_dir"

    # THE DESTINATION IS REMOVED FIRST when it is already there, which matters
    # for exactly one case and that case is a reinstall over a RUNNING program.
    # Writing into an executable that a process has mapped gives ETXTBSY on
    # Linux and the copy fails; unlinking it first replaces the directory entry
    # and leaves the running process holding the old inode, which is what every
    # package manager does and is why an upgrade does not kill what is open.
    [ ! -e "$_dst" ] || run rm -f -- "$_dst"

    run cp -- "$_src" "$_dst"
    run chmod "$_mode" -- "$_dst"
}

# remove_file() -- the mirror image, and deliberately silent about absence.
#
# A path that is not there is the NORMAL case on an uninstall: 060 declares the
# whole tree and removes what exists of it, so a package installed without the
# window has three fewer files than the declaration names and that is not an
# error to report. What would be an error is removing something else, and the
# protection against that is that this is only ever called on paths the
# declaration produced.
remove_file() {
    [ -e "$1" ] || [ -L "$1" ] || return 0
    run rm -f -- "$1"
}

# remove_dir_if_empty() -- tidy up on the way out without ever taking anything
# with it.
#
# `rmdir` AND NOT `rm -r`, and that is the whole safety argument. rmdir refuses
# a directory that still has something in it, so a shared directory like
# ~/.local/bin -- which certainly holds other people's programs -- survives, and
# a directory this install created and then emptied goes away. There is no test
# to get wrong because rmdir is the test.
remove_dir_if_empty() {
    [ -d "$1" ] || return 0
    run rmdir -- "$1" 2>/dev/null || :
}
