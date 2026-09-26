// The C++ counterpart of example/hello_world.satl, for the small-program half
// of the comparison in compare.cpp.
//
// It exists because the interesting question is not "which is faster once
// running" — a compiler wins that and everyone knows it — but where the
// crossover is once the compiler's own time is counted. For a program this
// size, satellite finishes before clang has finished reading <iostream>.

#include <iostream>

int main()
{
    std::cout << "hello, world!\n";
    return 0;
}
