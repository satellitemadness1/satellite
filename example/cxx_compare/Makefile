# The C++ side of the compile-inclusive comparison.
#
# compare.cpp is the driver; it shells out to THIS makefile to build the two
# programs, and times the build. So `bench` and `hello` are deliberately not
# dependencies of `all` — building them here would make the driver measure a
# no-op make and report a compile time of nothing.
#
#   make run     build the driver and run the whole comparison
#   make         build the driver only
#
# The two programs under test live one directory up, next to their satellite
# counterparts, so there is exactly one copy of each program and the two
# languages' versions sit side by side.

LLVM_BIN = $(HOME)/.local/llvm/bin
CXX      = $(LLVM_BIN)/clang++
CXXFLAGS = -std=c++20 -O2

all: compare

compare: compare.cpp
	$(CXX) $(CXXFLAGS) -o $@ compare.cpp

# The targets the driver builds and times. -O2 because that is what anyone
# shipping this program would use, and the run time has to be the real one for
# the compile time to be a fair thing to add to it.
bench: bench.cpp
	$(CXX) $(CXXFLAGS) -o $@ bench.cpp

hello: hello.cpp
	$(CXX) $(CXXFLAGS) -o $@ hello.cpp

run: compare
	./compare

clean:
	rm -f compare bench hello

.PHONY: all run clean
