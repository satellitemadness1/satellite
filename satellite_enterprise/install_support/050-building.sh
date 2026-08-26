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
    if [ "$dry_run" = no ]; then
        step "building in $repo"
    fi
    run "$MAKE" -C "$repo"

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
fi
