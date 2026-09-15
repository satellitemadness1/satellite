# satellite -- is the package intact, and which build of satl does this CPU want.
#
# THE PACKAGE IS CHECKED BEFORE ANYTHING IS WRITTEN, and that ordering is the
# point of this fragment. An install that copies eleven files and then discovers
# the twelfth is missing has left a half-installed satellite behind and no way
# to tell which half. Everything the tree declares is confirmed present first,
# and then the copying starts.

# ---------------------------------------------------------------------------
# WHAT MUST BE THERE. Named one by one and not counted, so a package missing a
# file fails with that file's name in the message.
#
# satl-term IS NOT IN THIS LIST, and satl.haswell is. The window is genuinely
# optional -- 040-machine.sh has already decided whether it can run, and a
# machine with no GTK gets a correct install of the interpreter rather than a
# failed install of everything. The second interpreter is not optional in the
# same way: it is the whole reason there is a choice to make below, and a
# package without it is a package that was built wrong.
missing=
if [ "$action" = install ]; then
    for _f in \
        "$payload_programs/satl" \
        "$payload_programs/satl.haswell" \
        "$payload_programs/satl-cpu-level" \
        "$payload_share/applications/$app_id.desktop" \
        "$payload_share/mime/packages/$mime_name.xml"
    do
        [ -f "$_f" ] || missing="$missing
    $_f"
    done

    # THE ARTWORK, BY THE LIST IN 010 RATHER THAN BY WHAT IS THERE. Eighteen
    # files: nine sizes of the application icon and nine of the file-type icon.
    # A package that lost one size installs eight and looks correct until
    # something asks for the size that is gone.
    for _size in $icon_sizes; do
        for _pair in "apps/$app_id.png" "mimetypes/$mime_name.png"; do
            _f=$payload_share/icons/hicolor/$_size/$_pair
            [ -f "$_f" ] || missing="$missing
    $_f"
        done
    done

    if [ -n "$missing" ]; then
        die "this package is incomplete. These files are named by the install
       tree and are not in it:$missing
       Unpack the package again -- a partial copy is the usual cause, and a
       tarball extracted while it was still downloading is the usual partial
       copy."
    fi
fi

# ---------------------------------------------------------------------------
# THE ONE PROPERTY THAT MAKES A SHIPPED BINARY SHIPPABLE, verified rather than
# assumed.
#
# satl is built fully static so that it depends on nothing at all on the target
# machine -- no libstdc++, no libc, no version of either to match. That is the
# promise the package makes, and it is also a property that a build can lose
# silently: ../../make_support/048-static.mk defaults STATIC to full, and
# `make STATIC=0` produces a satl that looks identical and is not.
#
# WHY A DYNAMIC satl WOULD BE WORSE THAN IT LOOKS ON THIS PARTICULAR TREE. The
# build environment exports LD_RUN_PATH, and GNU ld silently turns that into an
# RPATH when no -rpath is given, so a dynamically linked satl comes out pointing
# at /home/<the developer>/opt/gcc-17/lib64 -- a hand-built toolchain in one
# person's home directory, which does not exist here. It would resolve against
# the system libstdc++ instead, or fail, and `ldd` on the build machine would
# have said everything was fine.
#
# CHECKED HERE AND NOT ONLY IN make-package.sh because this is the copy that
# reaches the target. A check on the build machine proves what was built; this
# proves what arrived.
if [ "$action" = install ] && command -v ldd >/dev/null 2>&1; then
    for _p in satl satl.haswell satl-cpu-level; do
        # ldd exits 1 on a static binary, hence `|| :` -- the message is the
        # answer, not the status.
        case $(ldd "$payload_programs/$_p" 2>&1 || :) in
            *'not a dynamic executable'*|*'statically linked'*) ;;
            *)
                die "$_p in this package is dynamically linked and should not be.
       A shipped satl is built with STATIC=full so that it needs nothing
       from the machine it lands on. This one would need libraries that
       may not be here, and on this project's build machine it would
       carry an RPATH into a developer's home directory. Rebuild the
       package with make-package.sh, which checks the same thing."
                ;;
        esac
    done
fi

# ---------------------------------------------------------------------------
# WHICH satl. Four files are shipped and three programs are installed, which is
# not an omission.
#
# On x86-64 the build compiles the same sources twice: once for the baseline
# instruction set that every x86-64 chip has, and once for x86-64-v3, the set
# Haswell introduced in 2013 -- AVX2, FMA, BMI. satl-cpu-level asks THIS CPU
# which of them it can execute and prints the answer. This script installs that
# one under the name satl. So satl.haswell is not a fourth program, it is satl
# compiled a second time, and the package ships both because the package is
# built before it knows what machine it will land on.
#
# THE INSTALLED BINARY IS ITS OWN RECORD AND NEEDS NO MANIFEST: `satl --version`
# prints the flags its objects were actually compiled with, so the install
# cannot end up with a note that disagrees with the file it describes. See
# ../../make_support/045-microarchitecture.mk and PLAN.md sec 4.2.
#
# THE DETECTOR IS RUN FROM THE PACKAGE, not from the prefix, because it has to
# answer before anything is installed. It is static, so it runs anywhere the
# programs it is choosing between would run.
satl_source=$payload_programs/satl
cpu_variant=baseline
if [ "$action" = install ]; then
    if [ -x "$payload_programs/satl-cpu-level" ]; then
        # `|| :` and then a case: a detector that fails to run is a reason to
        # install the baseline, which every x86-64 CPU can execute, and not a
        # reason to fail the install. The baseline is the safe answer to a
        # question that did not get asked.
        case $("$payload_programs/satl-cpu-level" 2>/dev/null || :) in
            haswell)
                satl_source=$payload_programs/satl.haswell
                cpu_variant=haswell
                ;;
            *)
                cpu_variant=baseline
                ;;
        esac
    else
        cpu_variant=undetected
    fi
fi
