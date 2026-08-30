# satellite -- removing what this build made, and nothing the parent made.
#
# THE PARENT'S 070-clean.mk NAMES ITS FILES ONE BY ONE rather than deleting a
# directory, on the ground that a clean which deletes by pattern deletes what it
# was aimed at while a clean that deletes a directory deletes whatever else
# ended up in it. That argument is even stronger here and points the other way
# for one line, so both halves are written out.

# THE BINARIES, THE OBJECTS AND THE STAMPS, EVERY ONE OF THEM DERIVED. Not a
# single name below is spelled as a literal: the binaries are the four variables
# 030-output.mk declares, the objects are the parent's $(OBJS) resolved through
# this build's map, and the stamps are the three names 030 gave them. A clean
# that names files by hand goes stale the first time the build gains one, and
# the file it stops removing is the one that then survives into the next build.
#
# $(OBJS) COVERS THE .haswell.o OBJECTS TOO, since they differ from the baseline
# ones by a suffix and not by a directory. That is one of the reasons
# 045-microarchitecture.mk chose a suffix and 030-output.mk kept the choice.
#
# $(SATL_TERM) is named unconditionally even though it is built conditionally: a
# clean on a machine that has since lost its gtk4 must still remove the binary
# an earlier build left behind, and `rm -f` on a name that is not there is
# already the no-op this needs.
clean:
	rm -f $(SATL) $(SATL_HASWELL) $(CPU_LEVEL) $(SATL_TERM) \
	      $(OBJS) \
	      $(LDFLAGS_STAMP) $(CXXFLAGS_STAMP) $(CXXFLAGS_STAMP_HASWELL)
	rm -rf $(OBJ)
	-rmdir $(BUILD) 2>/dev/null || true

# THE TWO rm LINES DO DIFFERENT JOBS AND THE SECOND IS NOT REDUNDANT. The first
# removes the files this build is known to make. The second removes
# build/objects/ as a tree, and it is `rm -rf` on a directory -- the thing the
# paragraph above says a clean should not do -- for a reason that does not apply
# to the parent: this directory is ENTIRELY the property of this build. The
# parent's clean cannot delete src/programs/ because the sources live there;
# build/objects/programs/ holds nothing but output. Deleting the tree is also
# what takes the empty directories away, which the first line cannot do.
#
# rmdir AND NOT rm -rf ON build/ ITSELF, and that asymmetry is the point. rmdir
# removes a directory only when it is EMPTY, so a clean leaves build/ behind the
# moment anything unexpected is in it -- a core dump, a log somebody redirected,
# a binary from a version of this build that has since been edited. `rm -rf
# build` would take those with it without a word. The `-` and the `|| true`
# together mean a refusal is silent and not an error: a clean that could not
# empty build/ has still done its job.
#
# WHAT THIS DELIBERATELY DOES NOT TOUCH: anything under ../src, ../pcg or
# ../make_support, and in particular the parent's own objects, which sit beside
# their sources and are its clean's business. Running `make clean` in this
# directory must never make the root build stale, and the way that is guaranteed
# is that every path above begins with $(BUILD).

.PHONY: clean
