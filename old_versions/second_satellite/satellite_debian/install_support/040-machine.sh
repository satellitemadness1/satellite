# satellite -- what this machine is, and whether it can build.
#
# Nothing here runs on an uninstall: removing files needs no compiler and does
# not care what distribution this is.

# /etc/os-release is shell syntax, and the documented way to read it is to
# source it. This does not, because sourcing a file executes it, and a file in
# /etc is exactly the kind of thing that is one bad line away from running
# something in the middle of an installer. sed reads the one field asked for and
# can do nothing else.
os_field() {
    [ -r /etc/os-release ] || return 1
    _v=$(sed -n "s/^$1=//p" /etc/os-release | head -n 1)
    [ -n "$_v" ] || return 1
    case $_v in
        \"*\") _v=${_v#\"}; _v=${_v%\"} ;;
        \'*\') _v=${_v#\'}; _v=${_v%\'} ;;
    esac
    printf '%s' "$_v"
}

if [ "$action" = install ]; then
    os_name=$(os_field PRETTY_NAME || printf 'an unrecognised system')
    os_id=$(os_field ID || printf '')
    os_like=$(os_field ID_LIKE || printf '')

    step "$os_name"

    # NAMED, then the family, because a derivative sets ID to its own name and
    # only mentions debian in ID_LIKE. Debian 13 answers ID=debian and sets no
    # ID_LIKE at all; Ubuntu answers ID=ubuntu with ID_LIKE=debian; Mint and Pop
    # answer their own name with ID_LIKE="ubuntu debian". Measured on Debian 13
    # (trixie), 2026-08-28, on the machine this was written on.
    #
    # BOTH debian AND ubuntu ARE MATCHED DIRECTLY, and not only through
    # ID_LIKE, because Debian itself sets no ID_LIKE -- a check that only read
    # that field would print the not-my-family note on the distribution this
    # folder is named after.
    #
    # A NOTE AND NOT A REFUSAL, the same as the enterprise installer's: nothing
    # below is specific to this family except the package names in the advice,
    # and refusing on the strength of a name in a text file would be a policy
    # dressed up as a check.
    case " $os_id $os_like " in
        *" debian "* | *" ubuntu "*)
            ;;
        *)
            cat <<EOF
install.sh: note -- this installer is written for the Debian family (Debian,
            Ubuntu and their derivatives) and this machine reports itself as
            something else. Nothing below is specific to it, so this will very
            likely work; the package names in any advice further down are the
            apt ones and may not be what your system calls them.
            On the Enterprise Linux family there is a second installer written
            for it, with the dnf names: ../satellite_enterprise/install.sh
EOF
            ;;
    esac

    # THE BUILD IS THE REAL CHECK, and this is only here so that the commonest
    # way to be missing one produces a sentence naming a package instead of
    # sixty lines of make output. Which compiler actually gets used is
    # 010-compiler.mk's decision and is deliberately not re-derived here: it
    # prefers a clang under $HOME and falls back to c++, so a check that
    # insisted on c++ would fail on a machine where the build works.
    missing=
    command -v "$MAKE" >/dev/null 2>&1 || missing="$missing make"
    if ! command -v c++ >/dev/null 2>&1 &&
       ! command -v g++ >/dev/null 2>&1 &&
       ! command -v clang++ >/dev/null 2>&1 &&
       [ ! -x "${HOME:-}/opt/clang-current/bin/clang++" ]; then
        missing="$missing g++"
    fi

    # ONE PACKAGE ANSWERS BOTH, which is why the advice does not simply echo
    # $missing back. build-essential depends on g++, make and libc6-dev, so
    # naming the parts would send somebody to install two of the three things
    # one apt line gives them -- and libc6-dev is the one that is not in the
    # list above and is needed by --static later.
    if [ -n "$missing" ]; then
        die "no C++ compiler or no make on this machine.
       satellite is built from source by this script, so it needs both, and
       one package carries them:

           sudo apt install build-essential

       That is g++, make and libc6-dev together. C++20 is required, which
       every g++ shipped with Debian 13 or Ubuntu 24.04 provides -- and the
       same package is what makes --static work, because libstdc++.a lives in
       g++ and libc.a in libc6-dev.

       If you want the window as well:

           sudo apt install pkg-config libgtk-4-dev libvte-2.91-gtk4-dev"
    fi
fi

# CAN THIS USER WRITE THERE. Asked of the deepest ancestor that actually
# exists, because the prefix itself is routinely the directory being created:
# testing -w on /usr/local/lib/satellite before it is there answers no for the
# wrong reason and would send a root user away with a sudo instruction.
#
# WRITABILITY AND NOT `id -u`. A prefix under /opt or /srv that the installing
# user owns is a legitimate --prefix and needs no privilege at all, and a check
# that insisted on uid 0 would refuse it. On /usr/local the two questions have
# the same answer, which is the case this exists for.
writable_ancestor() {
    _p=$1
    while [ ! -e "$_p" ]; do
        _parent=${_p%/*}
        [ -z "$_parent" ] && _parent=/
        [ "$_parent" = "$_p" ] && break
        _p=$_parent
    done
    [ -w "$_p" ]
}

# Checked on an uninstall too. Removing /usr/local/bin/satl needs exactly the
# privilege installing it did, and finding that out one rm at a time leaves a
# half-removed install behind.
#
# NOT CHECKED IN A DRY RUN, which is not a loosening -- it is what makes the
# advice below possible. That message tells the reader to look at the command
# before running it under sudo, and the way to look at what this script would do
# is -n; refusing to print a transcript because the run it describes would need
# a privilege makes the one safe way to inspect it the one thing you cannot do
# without granting it. A dry run writes nothing, so there is nothing to permit.
if [ "$layout" = prefix ] && [ "$dry_run" = yes ] &&
   ! writable_ancestor "$root"; then
    cat <<EOF
install.sh: note -- $root is not writable by $(id -un), so the run
            described below would have to be made under sudo. Nothing is
            written by a dry run, so it is printed rather than refused.
EOF
elif [ "$layout" = prefix ] && ! writable_ancestor "$root"; then
    die "$root is not writable by $(id -un).
       That is what --system means: it installs INTO the operating system, over
       whatever satl is there now, and those files are root's.

           sudo sh $(quoted "$self") --prefix $(quoted "$root")

       THIS SCRIPT WILL NOT RUN sudo FOR YOU, which is the one part of its
       no-root rule that never depended on where the files went: a program that
       escalates on your behalf is a program you cannot audit by reading the
       command you typed. Run that line yourself, or read it first and then run
       it, or install into your home directory instead, which needs nothing:

           sh $(quoted "$self")"
fi
