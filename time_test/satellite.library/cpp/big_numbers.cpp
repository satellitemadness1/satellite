// time_test/satellite.library/cpp/big_numbers.cpp -- the C++ equivalent of
// programs/big_numbers.satl: x = x * 3, 150,000 turns, so x grows past a machine word within
// 40 turns and is a many-limb multiply (about 3,700 limbs at the end) after that.
// Not line for line: x is bignum (bignum.hpp), a hand-written big whole number, because C++
// has none; counter stays below 150,000, so it is long long.
//
// BUILD: clang++ -std=c++20 -O2 big_numbers.cpp -o big_numbers   (or g++)

#include "bignum.hpp"

#include <cstdio>

int main()
{
    bignum x(1);
    long long counter = 0;
    while (counter < 150000) {
        x = x * 3ull;
        counter = counter + 1;
    }
    std::printf("\n"); // satl's own empty line after its dashes: the race's answer starts there
    std::printf("%llu\n", x % 1000000007ull);
    return 0;
}
