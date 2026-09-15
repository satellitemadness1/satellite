# satellite -- what this machine is, and whether what is in the package can run
# on it.
#
# FOUR QUESTIONS, AND ONLY ONE OF THEM CAN STOP THE INSTALL. This fragment
# reports far more than it refuses, which is deliberate: a check that turns into
# a refusal is a policy, and the only policy worth having here is that the
# script must not write a file it cannot write. Everything else -- the wrong
# distribution, no GTK, no desktop at all -- produces a warning and a correct
# install of the parts that do work.

# ---------------------------------------------------------------------------
# 1. THE OPERATING SYSTEM, which is checked and never enforced.
#
# The package is BUILT on AlmaLinux 10.2 and satl-term is linked against that
# machine's GTK4 and VTE, so this is the one place where naming a distribution
# is a fact about the binaries rather than a preference. It is still not a
# refusal: Rocky, CentOS Stream, RHEL and Oracle 10 are the same libraries with
# a different name in /etc/os-release, and a machine that can run the binaries
# can install them. What this does is make a mismatch VISIBLE, so that a failure
# later has something to point at.
os_id=unknown
os_name=unknown
os_version=unknown
if [ -r /etc/os-release ]; then
    # SOURCED IN A SUBSHELL AND READ BACK THROUGH echo, not sourced into this
    # one. /etc/os-release is shell syntax by specification, but it is also a
    # file this script does not own, and sourcing it directly would let it set
    # $prefix or $action. The three values wanted come back as text.
    os_id=$( . /etc/os-release 2>/dev/null; printf '%s' "${ID:-unknown}" )
    os_name=$( . /etc/os-release 2>/dev/null; printf '%s' "${NAME:-unknown}" )
    os_version=$( . /etc/os-release 2>/dev/null; printf '%s' "${VERSION_ID:-unknown}" )
fi

os_family_ok=no
case $os_id in
    almalinux|rocky|centos|rhel|ol|fedora)
        os_family_ok=yes
        ;;
    *)
        # ID_LIKE is the second chance, and it is how a derivative says what it
        # is compatible with. Read the same guarded way.
        case $( . /etc/os-release 2>/dev/null; printf '%s' "${ID_LIKE:-}" ) in
            *rhel*|*fedora*|*centos*) os_family_ok=yes ;;
        esac
        ;;
esac

# ---------------------------------------------------------------------------
# 2. THE ARCHITECTURE, which CAN stop the install, because nothing here is
# fixable by carrying on. Every program in programs/ is an x86-64 ELF. On an
# aarch64 machine they do not run, cannot be made to run, and installing them
# would put four files on PATH that produce "cannot execute binary file".
machine_arch=$(uname -m 2>/dev/null || printf 'unknown')
case $machine_arch in
    x86_64|amd64) ;;
    *)
        die "this package holds x86-64 programs and this machine is $machine_arch.
       Nothing in programs/ can run here. satellite builds on this
       architecture from source -- see satellite_enterprise/install.sh in
       the source tree, which compiles rather than copies."
        ;;
esac

# ---------------------------------------------------------------------------
# 3. WHETHER THE WINDOW CAN RUN, asked of the binary itself rather than of the
# package manager.
#
# `ldd` AND NOT `rpm -q vte291-gtk4`, and the difference matters. The question
# is not whether a package is installed, it is whether the loader can satisfy
# every library this exact binary names -- which is also the question on a
# machine where the libraries came from somewhere other than dnf, and on one
# where the package is installed but the wrong soname is present. ldd answers
# the real question, is in glibc-common so it is always there, and prints "not
# found" against each library it cannot resolve.
#
# THE INTERPRETER IS NOT CHECKED THIS WAY because there is nothing to check:
# satl is fully static, ldd answers "not a dynamic executable", and it has no
# runtime dependency to be missing. 050-payload.sh verifies that property
# instead, which is a different question and belongs there.
missing_libs=
if [ -f "$payload_programs/satl-term" ]; then
    if command -v ldd >/dev/null 2>&1; then
        # `|| :` because ldd exits non-zero exactly when something IS missing,
        # which is the case this needs the output for. set -e would otherwise
        # end the script at the moment it learned the thing it came to learn.
        missing_libs=$(ldd "$payload_programs/satl-term" 2>/dev/null |
                       sed -n 's/^[[:space:]]*\([^[:space:]]*\) => not found$/\1/p' || :)
        if [ -n "$missing_libs" ]; then
            have_term=no
        else
            have_term=yes
        fi
    else
        # No ldd is not a reason to drop the window. It is a reason to say that
        # the check did not happen.
        have_term=unchecked
    fi
else
    have_term=absent
fi

# ---------------------------------------------------------------------------
# 4. WHETHER THE PREFIX CAN BE WRITTEN, which is the one check that exists to
# stop the script, and which never escalates.
#
# THE TEST IS ON THE DEEPEST EXISTING ANCESTOR, not on the prefix, because the
# prefix usually does not exist yet -- /usr/local/share/satellite is three
# directories that this install would create. What decides whether they can be
# created is whether the first one that DOES exist is writable.
writable_ancestor() {
    _p=$1
    while [ ! -d "$_p" ]; do
        _p=$(dirname -- "$_p")
        case $_p in
            /|.) break ;;
        esac
    done
    [ -w "$_p" ]
}

if [ "$dry_run" = no ]; then
    if ! writable_ancestor "$prefix"; then
        # THE COMMAND IS PRINTED AND NOT RUN. A script that calls sudo on your
        # behalf is a script whose effects you cannot predict from the line you
        # typed; this one stops and hands the line back, with every option
        # preserved so it can be pasted as-is.
        _cmd="sudo sh $(quoted "$self")"
        [ "$prefix" = /usr/local ] && _cmd="$_cmd --system"
        [ "$prefix" = /usr/local ] || _cmd="$_cmd --prefix $(quoted "$prefix" | sed 's/^ //')"
        [ "$action" = uninstall ] && _cmd="$_cmd --uninstall"
        die "$prefix is not writable by $(id -un).
       This script does not call sudo for you. To install there, run:
           $_cmd
       Or leave the option off entirely and install into your own home
       directory, which needs no password:
           sh $(quoted "$self" | sed 's/^ //')"
    fi
fi
