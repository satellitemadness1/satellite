// cpp/ints.cpp -- the C++ equivalent of programs/ints.satl: n = n + 1, 200,000 turns.
// Not line for line: it first prints the blank line satl prints after its dashes, so the
// answer is byte for byte what satl's is. n and counter never pass 200,000, so long long holds them.
// THE LOOP IS KEPT: clang++ and g++ at -O2 would fold it away and print 200000 without
// counting, which races nothing. The empty asm below tells the compiler the value is read
// each turn -- it adds no instruction, and the loop runs all 200,000 turns.
//
// BUILD: clang++ -std=c++20 -O2 ints.cpp -o ints      (g++ works the same)

#include <cstdio>

int main()
{
    std::printf("\n");

    long long n = 0;
    long long counter = 0;
    while (counter < 200000) {
        n = n + 1;
        asm volatile("" : "+r"(n));
        counter = counter + 1;
    }
    std::printf("%lld\n", n);
    return 0;
}
