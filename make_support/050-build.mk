# satellite -- the default goal and the binaries.
#
# THE FIRST FRAGMENT THAT DECLARES A TARGET, which is what makes `all` the first
# target make sees. .DEFAULT_GOAL below says it outright anyway, because after a
# split like this one the order of the rules is the order of the includes, and a
# rule that lands in the wrong file should not be able to change what `make`
# does.

.DEFAULT_GOAL := all

all: satl

# $(LDFLAGS) BEFORE the objects, which is where a linker wants its options.
satl: $(SATL_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(SATL_OBJS)

.PHONY: all clean
