// time_test/satellite.library/cpp/long_add.cpp -- the C++ equivalent of
// programs/long_add.satl: t = t + third, 200,000 turns, third being 1/3 at 128 places.
// Not line for line: a float is a bignum (bignum.hpp) scaled by 10^128, because C++ has no
// 128-place float; satl also splits each answer into whole and fraction parts, this keeps
// the one scaled number. counter stays below 200,000, so it is long long.
//
// BUILD: clang++ -std=c++20 -O2 long_add.cpp -o long_add   (or g++)

#include "bignum.hpp"

#include <cstdio>

int main()
{
    bignum third = scaled_float::from_whole(1);
    third = scaled_float::divided(third, 3);
    bignum t = scaled_float::from_whole(0); // 0.0
    long long counter = 0;
    while (counter < 200000) {
        t = t + third;
        counter = counter + 1;
    }
    std::printf("\n"); // satl's own empty line after its dashes: the race's answer starts there
    std::printf("%s\n", scaled_float::text(t).c_str());
    return 0;
}
