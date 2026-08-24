# satellite -- the two speed comparisons, and compressing the man pages.
#
# Moved out of the 1232-line Makefile on 2026-08-24, byte for byte.

# satellite against compiled C++ with the compiler's own time counted, which is
# the comparison that changes the answer. Depends on `satl` because the
# driver refuses to guess at an interpreter that is not built; the build of the
# interpreter is deliberately not part of what it measures.
compare: satl
	$(MAKE) -C example/cxx_compare run

# satellite against CPython. Two interpreters, so there is no build step on
# either side and the number is simply the whole-process wall clock.
python: satl
	$(MAKE) -C example/py_compare run

# gzip -9n, not -9c on a named file: -n keeps the source file name and the
# current time out of the gzip header, which is what makes two builds of the
# same page byte-identical and is why dh_compress uses it. dh_compress cannot
# repair ours, because it skips what is already .gz. Compressing here rather
# than in the install recipe is what puts the modes under install(1) instead of
# under the caller's umask; the temporary file is so that a failed gzip cannot
# leave a truncated page behind under a name make would then believe in.
dist/%.1.gz: dist/%.1
	gzip -9nc $< > $@.tmp && mv $@.tmp $@
