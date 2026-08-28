# satellite -- build both, then ask the CPU which one it can run.
#
# Nothing here runs on an uninstall. Ends with $variant and $satl_source set,
# which 060-install-tree.sh installs and 080-report.sh reports.

if [ "$action" = install ]; then
    [ -f "$repo/Makefile" ] ||
        die "no Makefile at $repo.
       This script installs the tree it travels in, and it expects to sit in
       satellite_enterprise/ at the top of that tree."

    # A SEPARATE STEP, so that a compile failure is reported as a compile
    # failure before anything has been copied anywhere. `make` here builds all
    # three binaries -- satl, satl.haswell and satl-cpu-level on x86-64, and
    # satl alone everywhere else -- because 050-build.mk puts them all in `all`.
    #
    # -C rather than a cd, because the top-level Makefile spells its includes
    # relatively and says it must be run with the tree as its working directory.
    # STATIC= RESOLVED HERE, because `auto` is a question about which layout
    # this run is and make cannot see that. 048-static.mk explains why the two
    # layouts get different answers; the short of it is that a home install is
    # run by the account whose home the libraries came out of and a system
    # install is not.
    #
    # WHAT THIS MACHINE CAN LINK IS ASKED OF make, not worked out here. Which
    # compiler is in use is 010-compiler.mk's decision, and a second probe in
    # this script would be a second decision, free to disagree with the one
    # that actually does the linking. `make -s static-available` answers full,
    # 1 or no, and `no` is not an error -- it is a machine without
    # libstdc++-static, which gets the build it has always got plus the note in
    # 075-system.sh saying what that costs a system install.
    # COMPILE AS THE HUMAN, COPY AS ROOT -- inherited from the first satellite's
    # installer, whose comment names the failure exactly: building under sudo
    # "leaves root-owned .o files in a source tree the developer then cannot
    # rebuild without sudo forever after." Reported here on 2026-08-28 by the
    # author, whose next plain ./install.sh died on
    #     error: unable to open output file 'src/programs/main.o':
    #            'Operation not permitted'
    # with sixteen root-owned files in the tree. --system introduced that
    # regression on the same day: the old installer never ran as root at all,
    # and its header's boast of "no root-owned object file left in a source tree
    # afterwards" was deleted along with the rule, without anything replacing it.
    # This replaces it.
    #
    # THE SECOND FAILURE IS QUIETER AND WORSE. Under sudo, HOME is /root, so
    # 010-compiler.mk cannot find a toolchain installed in the user's home and
    # falls back to c++. The build SUCCEEDS and produces a different binary --
    # on this machine gcc 14 at the baseline instead of clang 24 at
    # x86-64-v3 -- which `satl --version` then reports for the rest of its life.
    # A wrong-but-working build is not something a user is going to notice.
    #
    # THIS IS NOT THE sudo THE HEADER PROMISES NEVER TO RUN. That promise is
    # about ESCALATION: this script never acquires a privilege you did not give
    # it on the command line. Here it already has root and is handing it back
    # for the one step that must not have it. Dropping a privilege is the
    # opposite of taking one, and it is what keeps the promise about your source
    # tree that running as root would otherwise break.
    # THE QUERY VARIANT. Same de-escalation, deliberately NOT through run():
    # run() exists so that --dry-run prints a command instead of performing it,
    # which is right for every command that has an EFFECT and wrong for one that
    # only has an ANSWER. This writes nothing, so it runs for real in a dry run
    # too -- the same argument this file already makes for satl-cpu-level a few
    # lines down, and for the same reason: a transcript that guessed here would
    # be describing a build it had not asked about.
    query_make() {
        if [ "$(id -u)" = 0 ] && [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != root ] &&
           command -v sudo >/dev/null 2>&1; then
            sudo -H -u "$SUDO_USER" "$MAKE" "$@"
        else
            "$MAKE" "$@"
        fi
    }

    make_as_builder() {
        if [ "$(id -u)" = 0 ] && [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != root ] &&
           command -v sudo >/dev/null 2>&1; then
            run sudo -H -u "$SUDO_USER" "$MAKE" "$@"
        else
            run "$MAKE" "$@"
        fi
    }

    # ASKED THE SAME WAY THE BUILD IS RUN, and not with a bare $MAKE. This query
    # asks the COMPILER what it can link, so asking it as root asks about root's
    # compiler -- which, per the note above, is a different compiler. Answering
    # STATIC=full for a toolchain that is not the one about to build would be a
    # wrong answer arrived at carefully.
    static_arg=
    case $static in
        yes)  static_arg=STATIC=full ;;
        auto)
            if [ "$layout" = prefix ]; then
                _can=$(query_make -s -C "$repo" static-available 2>/dev/null ||
                       printf 'no')
                case $_can in
                    full | 1) static_arg=STATIC=$_can ;;
                esac
            fi
            ;;
    esac

    # A ROOT-OWNED OBJECT FILE FROM AN EARLIER RUN, caught here rather than left
    # to clang. The error it produces otherwise names an errno and a path and
    # nothing about why, and the fix -- `make clean`, which works WITHOUT sudo
    # because the directories are yours even when the files in them are not --
    # is not guessable from it. Cheap: one find over a tree of a few dozen
    # objects, and only when there is something to find.
    #
    # CHECKED IN A DRY RUN TOO, unlike the missing-source check in
    # 060-install-tree.sh, and the difference is worth stating because the two
    # look alike. That one skips under -n because a real run BUILDS the file it
    # is missing, so reporting it would be describing a problem that fixes
    # itself one step later. This one is the opposite: a real run does nothing
    # whatever about a root-owned object file, so -n staying quiet would let the
    # one command whose whole job is "tell me what would happen" answer with a
    # transcript of an install that cannot occur. It reports and carries on
    # under -n, and stops a real run.
    if [ "$(id -u)" != 0 ]; then
        _foreign=$(find "$repo/src" "$repo" -maxdepth 2 \( -name '*.o' -o -name 'satl' \
                        -o -name 'satl.haswell' -o -name 'satl-term' \
                        -o -name 'satl-cpu-level' \) ! -writable 2>/dev/null | head -5)
        if [ -n "$_foreign" ] && [ "$dry_run" = yes ]; then
            printf 'install.sh: note -- build products in %s are not writable\n' "$repo"
            printf '            by you, so a REAL run would stop here. %s -C %s clean\n' \
                "$MAKE" "$(quoted "$repo")"
            printf '            clears them, and needs no sudo. Continuing the preview.\n'
        elif [ -n "$_foreign" ]; then
            die "there are build products in $repo that you cannot write:

$(printf '%s\n' "$_foreign" | sed 's/^/           /')

       They were almost certainly left by an earlier run of this script under
       sudo, which is a bug this script has since fixed -- it now builds as the
       invoking user and uses root only for the copy. Nothing is wrong with your
       tree and nothing needs privilege to repair it:

           $(quoted "$MAKE") -C $(quoted "$repo") clean

       That works as you, without sudo: removing a file needs write permission
       on its DIRECTORY, which is yours, not on the file, which is root's."
        fi
    fi

    if [ "$dry_run" = no ]; then
        step "building in $repo"
        [ -n "$static_arg" ] && step "linking the C++ runtime in ($static_arg)"
        if [ "$(id -u)" = 0 ] && [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != root ]; then
            step "building as $SUDO_USER; root is used only for the copy"
        fi
    fi
    make_as_builder -C "$repo" ${static_arg:+"$static_arg"}

    # THE CHOICE. satl-cpu-level is compiled at the baseline precisely so that
    # it can run here, before anything is known about the machine, and it prints
    # one word: haswell or baseline. See src/programs/cpu_level.cpp.
    #
    # It is run for real even in a dry run, because it writes nothing and
    # reading the CPU is the only way this transcript can name the file it would
    # actually install. When it has not been built yet -- a dry run on a clean
    # tree -- there is nothing to ask, and the transcript says so rather than
    # guessing.
    if [ -x "$repo/satl-cpu-level" ]; then
        variant=$("$repo/satl-cpu-level")
    elif [ "$dry_run" = yes ]; then
        variant=undecided
    else
        # No detector after a successful build means this is not x86-64, where
        # 045-microarchitecture.mk builds one satl on purpose.
        variant=baseline
    fi

    case $variant in
        haswell)   satl_source=$repo/satl.haswell ;;
        baseline)  satl_source=$repo/satl ;;
        undecided) satl_source="$repo/satl[.haswell]" ;;
        *) die "satl-cpu-level printed \"$variant\", which is not a build.
       It prints exactly one of haswell or baseline. Something else means the
       two halves of this mechanism have drifted apart -- see
       src/programs/cpu_level.cpp and make_support/045-microarchitecture.mk." ;;
    esac

    # A HASWELL ANSWER WITH NO HASWELL BINARY is not fatal and is worth saying
    # out loud. It happens when the detector survives from an older build that
    # made one, so falling back to the baseline build is both correct and quiet
    # enough to be worth a line.
    if [ "$variant" = haswell ] && [ ! -x "$satl_source" ] && [ "$dry_run" = no ]; then
        printf 'install.sh: note -- this CPU can run the haswell build, but\n'
        printf '            %s was not built. Installing the baseline one.\n' \
            "$satl_source"
        variant=baseline
        satl_source=$repo/satl
    fi

    case $variant in
        haswell)
            step "this CPU has x86-64-v3, so it gets the haswell build"
            ;;
        baseline)
            step "installing the baseline build, which runs on any x86-64"
            ;;
        undecided)
            step "the build to install is chosen after the build, by satl-cpu-level"
            ;;
    esac

    # THE OTHER TWO PROGRAMS, and this is where the run learns whether there are
    # two of them. satl-cpu-level is unconditional on x86-64 -- the choice above
    # could not have been made without it -- but satl-term is conditional on
    # gtk4 and vte-2.91-gtk4 being installed, which 047-window.mk asks
    # pkg-config and 050-build.mk answers by dropping the target from `all` with
    # a note. A machine without them gets a correct install of three programs,
    # not a failed install of four.
    #
    # ASKED OF THE FILE AND NOT OF pkg-config, deliberately. The build has just
    # run; whether the binary is there is the only fact that matters to a copy,
    # and re-asking pkg-config would be this script forming its own opinion
    # about a question make has already answered. §9's rule about verifying
    # through the real code path is the same argument.
    if [ -x "$repo/satl-term" ]; then
        have_term=yes
    elif [ "$dry_run" = yes ] && [ ! -f "$repo/satl-term" ]; then
        # A dry run on a clean tree has nothing to look at, and the honest
        # answer is that a real run would build it and then know. The
        # transcript says so rather than guessing either way.
        have_term=undecided
    else
        have_term=no
    fi

    case $have_term in
        yes)
            step "satl-term was built, so the window and its launcher install too"
            ;;
        no)
            # NO COUNT IN THIS LINE, on purpose: how many programs install
            # depends on the architecture as well as on the libraries, and a
            # number here would be wrong on the machine that has neither.
            step "satl-term was not built -- no gtk4/vte -- so no window installs"
            ;;
        undecided)
            step "whether satl-term installs is decided by whether the build makes one"
            ;;
    esac
fi
