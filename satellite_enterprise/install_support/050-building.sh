# satellite 004 -- make.
#
# THE INSTALL IS OF WHAT make BUILDS NOW, never of whatever happens to be in
# build/: a make with nothing to do says nothing and costs a second, and a make
# with something to do raises the build number, which is the point of one.
# MAKE may carry flags: MAKE="make -j4" on a busy machine.

# FROM `make install` (080-install.mk), make has just built everything, and running
# it again from inside its own recipe would only ask the same question twice: a make
# inside a make, or one that says INSTALL_AFTER_BUILD=no as this one does, is never
# the bare make that builds every time (050-build.mk's BARE_MAKE).
# ALWAYS make, AND NO "JUST BUILT" SHORTCUT (the author, 2026-10-07: "just remove the code that
# checks if it just built it and delete it altogether"). When make itself runs this installer, that
# make is done and this one finds nothing to rebuild, so it costs a moment and prints what it did.
step "building: $MAKE in $repo"
# shellcheck disable=SC2086
(cd -- "$repo" && $MAKE INSTALL_AFTER_BUILD=no) || die "make failed, so nothing was installed"

[ -f "$build/satl" ] && [ -x "$build/satl" ] || die "make finished without $build/satl, so nothing was installed"
# NO LIBRARIES TO LOOK FOR SINCE 2026-10-07: every word is inside satl (satellite-numbers/word_table.hpp).
# NO satl-term SINCE 2026-09-22: satl opens its own console (GTK-17), and the
# author said "we are getting rid of satl-term". 060 still removes one an
# earlier install left, once 040 has proved it was this installer's.
