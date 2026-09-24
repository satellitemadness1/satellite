// cpp/strings.cpp -- the C++ equivalent of programs/strings.satl: a string joined to itself four
// times and compared with a second join, 300,000 turns. Not line for line: it first prints the
// blank line satl prints after its dashes. The strings are std::u16string because satl's are
// 16-bit characters. same and counter never pass 300,000, so long long holds them.
//
// BUILD: clang++ -std=c++20 -O2 strings.cpp -o strings      (g++ works the same)

#include <cstdio>
#include <string>

int main()
{
    std::printf("\n");

    std::u16string piece = u"the satellite programming language races itself, compiled twice over.";
    std::u16string joined = u"";
    long long same = 0;
    long long counter = 0;
    while (counter < 300000) {
        joined = piece + piece + piece + piece;
        if (joined == piece + piece + piece + piece) {
            same = same + 1;
        }
        counter = counter + 1;
    }
    std::printf("%lld\n", same);
    return 0;
}
