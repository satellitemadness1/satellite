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
    # only mentions rhel in ID_LIKE. AlmaLinux 10.2 answers ID=almalinux and
    # ID_LIKE="rhel centos fedora" -- measured on the machine this was written
    # on, 2026-08-26.
    #
    # A NOTE AND NOT A REFUSAL. This folder is called satellite_enterprise
    # because that is what it was written and tested for, not because the script
    # would break elsewhere: everything below is make, install, ln and a
    # directory under $HOME, all of which Debian has too. Refusing on the
    # strength of a name in a text file would be a policy dressed up as a check.
    case " $os_id $os_like " in
        *" rhel "* | *" almalinux "* | *" rocky "* | *" centos "*)
            ;;
        *)
            cat <<EOF
install.sh: note -- this installer is written for the Enterprise Linux family
            (RHEL, AlmaLinux, Rocky, CentOS Stream) and this machine reports
            itself as something else. Nothing below is specific to it, so this
            will very likely work; the package names in any advice further down
            are the dnf ones and may not be what your system calls them.
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
        missing="$missing gcc-c++"
    fi

    if [ -n "$missing" ]; then
        die "no C++ compiler or no make on this machine.
       satellite is built from source by this script, so it needs both:

           sudo dnf install$missing

       Those are the Enterprise Linux package names. C++20 is required, which
       every gcc-c++ shipped with EL9 or EL10 provides."
    fi
fi

# satl-term, at M11, will need gtk4-devel and vte291-gtk4-devel, and the second
# of those lives in the CRB repository:
#
#     sudo dnf install --enablerepo=crb gtk4-devel vte291-gtk4-devel
#
# Deliberately NOT checked and not printed today. There is no satl-term in this
# tree to build, and an installer that warns about a missing dependency of a
# binary it is not building has taught the user to ignore its warnings.
