LLVM_BIN = $(HOME)/.local/llvm/bin
CXX      = $(LLVM_BIN)/clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2

all: compare

compare: compare.cpp
	$(CXX) $(CXXFLAGS) -o $@ compare.cpp

run: compare
	./compare

clean:
	rm -f compare .python.out .satellite.out

.PHONY: all run clean
