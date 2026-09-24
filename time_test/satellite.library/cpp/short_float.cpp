// cpp/short_float.cpp -- the C++ equivalent of programs/short_float.satl: f = f + 0.5, 200,000 turns.
// Not line for line: it first prints the blank line satl prints after its dashes. f is a double,
// not satl's 128-place float: every value is a multiple of 0.5 under 2^53, so a double holds each
// one exactly. display() prints it the satl way: 32 places, trailing zeros dropped, one digit
// kept after the point (satl shows 2.0, 0.25, 100000.5). counter never passes 200,000.
//
// BUILD: clang++ -std=c++20 -O2 short_float.cpp -o short_float      (g++ works the same)

#include <cstdio>
#include <cstring>

namespace {

// satl's float display: rounded to 32 places, then the zeros at the end of them removed,
// leaving at least one digit after the point. %.32f is exact for these values (steps of 0.5).
void display(double f)
{
    char text[400];
    std::snprintf(text, sizeof text, "%.32f", f);
    char *point = std::strchr(text, '.');
    char *end = text + std::strlen(text);
    while (end - point > 2 && end[-1] == '0') --end;
    *end = '\0';
    std::printf("%s\n", text);
}

} // namespace

int main()
{
    std::printf("\n");

    double f = 0.5;
    long long counter = 0;
    while (counter < 200000) {
        f = f + 0.5;
        counter = counter + 1;
    }
    display(f);
    return 0;
}
