// cpp/empty.cpp -- the C++ equivalent of programs/empty.satl: the loop alone, 200,000 turns.
// Not line for line: it first prints the blank line satl prints after its dashes, so the
// answer is byte for byte what satl's is. counter never passes 200,000, so a long long holds it.
// THE LOOP IS KEPT: clang++ and g++ at -O2 would fold it away and print 200000 without
// counting, which races nothing. The empty asm below tells the compiler the value is read
// each turn -- it adds no instruction, and the loop runs all 200,000 turns.
//
// BUILD: clang++ -std=c++20 -O2 empty.cpp -o empty      (g++ works the same)

#include <cstdio>

int main()
{
    std::printf("\n");

    long long counter = 0;
    while (counter < 200000) {

        counter = counter + 1;
        asm volatile("" : "+r"(counter));
    }
    std::printf("%lld\n", counter);
    return 0;
}
