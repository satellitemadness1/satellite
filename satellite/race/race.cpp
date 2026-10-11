// The author's bar: satellite-004 must run within x1.05 of compiled C++.
// This races display's numbered function, 1 5 1, against std::cout directly.
//
//     build/race            satellite-004 first, then std::cout
//     build/race cout-first the other order, because going first can cost a few percent
//
// DISPLAY'S ROW, FILLED BY THE FUNCTION satl FILES IT WITH (word_table.hpp), and not the whole
// number index any more (2026-10-10): a string's methods became numbered words that run satl's own
// strings that day, so every word together no longer links without the interpreter -- and display's
// row was all the race ever read. The pointer it calls is the one satl calls.

#include "../machine/machine_codes.hpp"
#include "../../satellite-numbers/word_table.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    using namespace satellite004;
    std::ios::sync_with_stdio(false);

    LibraryRow row;
    if (built_in::describe_1_5_1(&row) != success)
        return 1;
    const LibraryRow *display = &row;      // chosen once, before the race
    const std::string text = "hello, world!";
    const long long lines = 10000000;

    auto time_satellite = [&] {
        auto start = std::chrono::high_resolution_clock::now();
        for (long long i = 0; i < lines; i++)
            if (display->scenarios.text(text, true) != success)
                return -1LL;
        std::cout.flush();
        return (long long)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count();
    };
    auto time_cout = [&] {
        auto start = std::chrono::high_resolution_clock::now();
        for (long long i = 0; i < lines; i++)
            std::cout << text << '\n';
        std::cout.flush();
        return (long long)std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count();
    };

    long long satellite_ns, cout_ns;
    if (argc > 1 && std::strcmp(argv[1], "cout-first") == 0) {
        cout_ns = time_cout();
        satellite_ns = time_satellite();
    } else {
        satellite_ns = time_satellite();
        cout_ns = time_cout();
    }
    std::fprintf(stderr, "%lld %lld\n", satellite_ns, cout_ns);
}
