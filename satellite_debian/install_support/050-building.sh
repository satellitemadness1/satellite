# satellite -- build both, then ask the CPU which one it can run.
#
# Nothing here runs on an uninstall. Ends with $variant and $satl_source set,
# which 060-install-tree.sh installs and 080-report.sh reports.
#
# THE FRAGMENT THAT DIFFERS MOST FROM THE ENTERPRISE ONE, and all of it comes
# from one fact: this is an out-of-tree build. That installer runs make at the
# top of the tree and finds satl beside the Makefile. This one runs make in
# satellite_debian/ and finds all four binaries in build/, because that is where
# ../satellite_debian/make_support/030-output.mk puts them.

if [ "$action" = install ]; then
    [ -f "$here/Makefile" ] ||
        die "no Makefile at $here.
       This script installs the tree it travels in, and it expects to sit in
       satellite_debian/ beside the Makefile that builds it."

    [ -f "$repo/make_support/040-sources.mk" ] ||
        die "$here is not inside a satellite tree.
       satellite_debian/ is a build OF the tree above it: its Makefile reads
       ../src, ../pcg and ../make_support, and none of those is at $repo.
       Copying this folder out on its own does not carry the language with it."

    # A SEPARATE STEP, so that a compile failure is reported as a compile
    # failure before anything has been copied anywhere. `make` here builds all
    # four binaries -- satl, satl.haswell and satl-cpu-level on x86-64, satl
    # alone everywhere else, plus satl-term wherever vte-2.91-gtk4 is present.
    #
    # -C rather than a cd, because satellite_debian/Makefile spells its includes
    # relatively -- both to make_support/ beside it and to ../src above it -- and
    # says it must be run with satellite_debian as its working directory.
    #
    # STATIC= RESOLVED HERE, because `auto` is a question this run can answer and
    # make cannot. WHAT THIS MACHINE CAN LINK IS ASKED OF make, not worked out
    # here: which compiler is in use is 010-compiler.mk's decision, and a second
    # probe in this script would be a second decision, free to disagree with the
    # one that actually does the linking. `make -s static-available` answers
    # full, 1 or no.
    #
    # COMPILE AS THE HUMAN, COPY AS ROOT. Building under sudo leaves root-owned
    # object files behind, and worse: under sudo HOME is /root, so
    # 010-compiler.mk cannot find a toolchain installed in the user's home and
    # falls back to c++. The build SUCCEEDS and produces a different binary,
    # which `satl --version` then reports for the rest of its life. A
    # wrong-but-working build is not something a user is going to notice.
    #
    # THIS IS NOT THE sudo THE HEADER PROMISES NEVER TO RUN. That promise is
    # about ESCALATION: this script never acquires a privilege you did not give
    # it on the command line. Here it already has root and is handing it back
    # for the one step that must not have it. Dropping a privilege is the
    # opposite of taking one.
    #
    # THE BLAST RADIUS IS SMALLER HERE THAN IN THE ENTERPRISE TREE, and it is
    # worth writing down because it is a property of the out-of-tree build
    # rather than of this script's care. There, a `sudo ./install.sh` wrote
    # root-owned .o files into src/ -- the shared source tree, which the
    # developer then could not rebuild. Here every object this build makes is
    # under satellite_debian/build/, and ../src is only ever read. So the same
    # mistake dirties one disposable directory that `make clean` empties, and
    # can never touch the language. The de-escalation below is still done, for
    # the second reason above: root's compiler is a different compiler.

    # THE QUERY VARIANT. Same de-escalation, deliberately NOT through run():
    # run() exists so that --dry-run prints a command instead of performing it,
    # which is right for every command that has an EFFECT and wrong for one that
    # only has an ANSWER. This writes nothing, so it runs for real in a dry run
    # too -- a transcript that guessed here would be describing a build it had
    # not asked about.
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
            # BOTH LAYOUTS. The question is not which layout this is, it is
            # whether an installed program should depend on the toolchain that
            # happened to build it. It should not. On this family the answer is
            # almost always `full`, because libstdc++.a ships in g++ and libc.a
            # in libc6-dev and build-essential depends on both -- so unlike the
            # enterprise tree, where the two are separate packages and one is in
            # a repository that is off by default, reaching `no` here means
            # somebody installed a bare compiler on purpose.
            _can=$(query_make -s -C "$here" static-available 2>/dev/null ||
                   printf 'no')
            case $_can in
                full | 1) static_arg=STATIC=$_can ;;
            esac
            ;;
    esac

    # A ROOT-OWNED OBJECT FILE FROM AN EARLIER RUN, caught here rather than left
    # to the compiler. The error it produces otherwise names an errno and a path
    # and nothing about why, and the fix -- `make clean`, which works WITHOUT
    # sudo because the directories are yours even when the files in them are not
    # -- is not guessable from it.
    #
    # ONE DIRECTORY IS SEARCHED AND NOT TWO. The enterprise installer has to
    # look in src/ as well, because that is where its objects land; this build
    # puts every one of them under build/, so that is the whole of the answer.
    #
    # CHECKED IN A DRY RUN TOO, unlike the missing-source check in
    # 060-install-tree.sh, and the difference is worth stating because the two
    # look alike. That one skips under -n because a real run BUILDS the file it
    # is missing. This one is the opposite: a real run does nothing whatever
    # about a root-owned object file, so -n staying quiet would let the one
    # command whose whole job is "tell me what would happen" answer with a
    # transcript of an install that cannot occur.
    if [ "$(id -u)" != 0 ] && [ -d "$build" ]; then
        _foreign=$(find "$build" \( -name '*.o' -o -name 'satl' \
                        -o -name 'satl.haswell' -o -name 'satl-term' \
                        -o -name 'satl-cpu-level' \) ! -writable 2>/dev/null | head -5)
        if [ -n "$_foreign" ] && [ "$dry_run" = yes ]; then
            printf 'install.sh: note -- build products in %s are not writable\n' "$build"
            printf '            by you, so a REAL run would stop here. %s -C %s clean\n' \
                "$MAKE" "$(quoted "$here")"
            printf '            clears them, and needs no sudo. Continuing the preview.\n'
        elif [ -n "$_foreign" ]; then
            die "there are build products in $build that you cannot write:

$(printf '%s\n' "$_foreign" | sed 's/^/           /')

       They were almost certainly left by an earlier run of this script under
       sudo, which is a bug this script has since fixed -- it now builds as the
       invoking user and uses root only for the copy. Nothing is wrong with your
       tree and nothing needs privilege to repair it:

           $(quoted "$MAKE") -C $(quoted "$here") clean

       That works as you, without sudo: removing a file needs write permission
       on its DIRECTORY, which is yours, not on the file, which is root's.
       Nothing under $repo/src was touched -- this build never writes there."
        fi
    fi

    if [ "$dry_run" = no ]; then
        step "building in $here"
        [ -n "$static_arg" ] && step "linking the C++ runtime in ($static_arg)"
        if [ "$(id -u)" = 0 ] && [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != root ]; then
            step "building as $SUDO_USER; root is used only for the copy"
        fi
    fi
    make_as_builder -C "$here" ${static_arg:+"$static_arg"}

    # THE CHOICE. satl-cpu-level is compiled at the baseline precisely so that
    # it can run here, before anything is known about the machine, and it prints
    # one word: haswell or baseline. See ../src/programs/cpu_level.cpp.
    #
    # It is run for real even in a dry run, because it writes nothing and
    # reading the CPU is the only way this transcript can name the file it would
    # actually install. When it has not been built yet -- a dry run on a clean
    # tree -- there is nothing to ask, and the transcript says so rather than
    # guessing.
    if [ -x "$build/satl-cpu-level" ]; then
        variant=$("$build/satl-cpu-level")
    elif [ "$dry_run" = yes ]; then
        variant=undecided
    else
        # No detector after a successful build means this is not x86-64, where
        # 045-microarchitecture.mk builds one satl on purpose.
        variant=baseline
    fi

    case $variant in
        haswell)   satl_source=$build/satl.haswell ;;
        baseline)  satl_source=$build/satl ;;
        undecided) satl_source="$build/satl[.haswell]" ;;
        *) die "satl-cpu-level printed \"$variant\", which is not a build.
       It prints exactly one of haswell or baseline. Something else means the
       two halves of this mechanism have drifted apart -- see
       ../src/programs/cpu_level.cpp and
       ../make_support/045-microarchitecture.mk." ;;
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
        satl_source=$build/satl
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
    # vte-2.91-gtk4 being installed, which 047-window.mk asks pkg-config and the
    # build answers by dropping the target with a note.
    #
    # ASKED OF THE FILE AND NOT OF pkg-config, deliberately. The build has just
    # run; whether the binary is there is the only fact that matters to a copy,
    # and re-asking pkg-config would be this script forming its own opinion
    # about a question make has already answered.
    if [ -x "$build/satl-term" ]; then
        have_term=yes
    elif [ "$dry_run" = yes ] && [ ! -f "$build/satl-term" ]; then
        # A dry run on a clean tree has nothing to look at, and the honest
        # answer is that a real run would build it and then know.
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
            step "satl-term was not built -- no vte-2.91-gtk4 -- so no window installs"
            ;;
        undecided)
            step "whether satl-term installs is decided by whether the build makes one"
            ;;
    esac
fi
