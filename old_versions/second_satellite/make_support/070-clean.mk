# satellite -- removing what a build made.
#
# Named one by one rather than `rm -rf` on a directory. A clean that deletes by
# pattern deletes what it was aimed at; a clean that deletes a directory deletes
# whatever else ended up in it.

# $(SRC)/*/*.o covers the .haswell.o objects as well, since they differ from
# the baseline ones by a suffix and not by a directory -- which is one of the
# reasons 045-microarchitecture.mk chose a suffix.
#
# $(SRC)/*/*/*.o IS THE SECOND LEVEL AND IT IS NOT REDUNDANT. A shell glob does
# not descend: `src/*/*.o` matches src/programs/window.o and cannot match
# src/programs/satl-term/window.o, so when satl-term's two sources moved into their
# own directory on 2026-09-06 their objects left the reach of this line. The
# pattern rule in 060-compile.mk keeps compiling them, which is the failure this
# guards against -- a clean that misses an object does not fail, it leaves one
# behind, and the next link takes it. A THIRD level would need a third glob;
# there is no third level today.
# satl-term is named unconditionally even though it is built conditionally: a
# clean on a machine that has since lost its gtk4 must still remove the binary
# an earlier build left behind, and `rm -f` on a name that is not there is
# already the no-op this needs.
# $(TESTBINS) IS DERIVED FROM TESTNAMES and never spelled out, for the same
# reason 065-tests.mk derives the build list: a clean that names test binaries
# by hand goes stale the first time one is added, and leaves a stale binary that
# `make test` will happily run.
#
# $(STARTUP_FLOOR) IS THE ONE STALE FILE HERE THAT WOULD LIE RATHER THAN FAIL,
# and it is named through 067-startup.mk's variable for the reason above. A left
# behind test binary at least runs its own out-of-date assertions; a left behind
# floor is an empty program linked the way the tree was linked SOME OTHER TIME,
# and `make startup` would subtract it from a satl linked this way and print the
# difference as a milestone's fault. 067 makes it depend on both stamps so that
# a rebuild is triggered rather than needed -- this line is the second lock on
# the same door, and 040-sources.mk records what it costs when neither is there.
# $(ZLIB)/*.o IS THE VENDORED LIBRARY AND IT IS NAMED FOR THE SAME REASON THE
# TWO src GLOBS ARE. It is not under $(SRC), so neither of them reaches it, and
# a zlib object surviving a `clean` is the exact failure this file's header
# describes one case up: a rebuild that looks complete while linking bytes
# compiled some other time, against flags 010-compiler.mk has since changed.
# There is no .cxxflags-stamp on those objects -- 060-compile.mk says why -- so
# this rule is the ONLY thing that makes them go.
clean:
	rm -f satl satl.haswell satl-cpu-level satl-term $(SRC)/*/*.o $(SRC)/*/*/*.o \
	      $(ZLIB)/*.o \
	      $(TESTBINS) $(STARTUP_FLOOR) \
	      .cxxflags-stamp .cxxflags-stamp-haswell .ldflags-stamp \
	      .version-stamp
