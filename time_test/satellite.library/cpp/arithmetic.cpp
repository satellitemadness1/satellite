// cpp/arithmetic.cpp -- the C++ equivalent of programs/arithmetic.satl: small whole-number
// arithmetic, 1,000,000 turns. Not line for line: it first prints the blank line satl prints after
// its dashes. counter * 3 is under 3,000,000 and total under 7,000,000, so long long holds them.
// satl reads counter * 3 % 7 as (counter * 3) % 7, as C++ does; satl's answer (2999997) agrees.
//
// BUILD: clang++ -std=c++20 -O2 arithmetic.cpp -o arithmetic      (g++ works the same)

#include <cstdio>

int main()
{
    std::printf("\n");

    long long total = 0;
    long long counter = 0;
    while (counter < 1000000) {
        total = total + counter * 3 % 7;
        counter = counter + 1;
    }
    std::printf("%lld\n", total);
    return 0;
}
