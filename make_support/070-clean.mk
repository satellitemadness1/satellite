# satellite -- removing what a build made.
#
# Named one by one rather than `rm -rf` on a directory. A clean that deletes by
# pattern deletes what it was aimed at; a clean that deletes a directory deletes
# whatever else ended up in it.

# $(SRC)/*/*.o covers the .haswell.o objects as well, since they differ from
# the baseline ones by a suffix and not by a directory -- which is one of the
# reasons 045-microarchitecture.mk chose a suffix.
clean:
	rm -f satl satl.haswell satl-cpu-level $(SRC)/*/*.o \
	      .cxxflags-stamp .cxxflags-stamp-haswell
