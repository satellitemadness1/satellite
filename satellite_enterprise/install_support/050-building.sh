# satellite 004 -- make.
#
# THE INSTALL IS OF WHAT make BUILDS NOW, never of whatever happens to be in
# build/: a make with nothing to do says nothing and costs a second, and a make
# with something to do raises the build number, which is the point of one.
# MAKE may carry flags: MAKE="make -j4" on a busy machine.

step "building: $MAKE in $repo"
# shellcheck disable=SC2086
(cd -- "$repo" && $MAKE) || die "make failed, so nothing was installed"

[ -f "$build/satl" ] && [ -x "$build/satl" ] || die "make finished without $build/satl, so nothing was installed"
for _library in "$build"/satellite-numbers/*.so; do
    [ -f "$_library" ] || die "make finished without a library in $build/satellite-numbers/, so nothing was installed"
    break
done
# NO satl-term SINCE 2026-09-22: satl opens its own console (GTK-17), and the
# author said "we are getting rid of satl-term". 060 still removes one an
# earlier install left, once 040 has proved it was this installer's.
