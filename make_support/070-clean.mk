# satellite -- removing what a build made.
#
# Named one by one rather than `rm -rf` on a directory. A clean that deletes by
# pattern deletes what it was aimed at; a clean that deletes a directory deletes
# whatever else ended up in it.

# $(SRC)/*/*.o covers the .haswell.o objects as well, since they differ from
# the baseline ones by a suffix and not by a directory -- which is one of the
# reasons 045-microarchitecture.mk chose a suffix.
# satl-term is named unconditionally even though it is built conditionally: a
# clean on a machine that has since lost its gtk4 must still remove the binary
# an earlier build left behind, and `rm -f` on a name that is not there is
# already the no-op this needs.
clean:
	rm -f satl satl.haswell satl-cpu-level satl-term $(SRC)/*/*.o \
	      .cxxflags-stamp .cxxflags-stamp-haswell
