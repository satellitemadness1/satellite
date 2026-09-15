// The author's bar: satellite-004 must run within x1.05 of compiled C++.
// This races display through the number index against std::cout directly.
//
//     build/race            satellite-004 first, then std::cout
//     build/race cout-first the other order, because going first can cost a few percent

#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"
#include "../../satellite-numbers/call_number.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    using namespace satellite004;
    std::ios::sync_with_stdio(false);

    MachineState state;
    NumberIndex index;
    if (index.load("build/satellite-numbers", state) != number_vector_defined)
        return 1;
    const NumberRow *display = index.find("satellite.console.display");      // chosen once, before the race
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
