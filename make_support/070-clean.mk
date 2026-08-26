# satellite -- removing what a build made.
#
# Named one by one rather than `rm -rf` on a directory. A clean that deletes by
# pattern deletes what it was aimed at; a clean that deletes a directory deletes
# whatever else ended up in it.

clean:
	rm -f satl $(SRC)/*/*.o .cxxflags-stamp
