// Like satl: one display every ~2 us of other work. Strings, then numbers made the way expression.cpp makes them.
#include "satellite/display/display_buffer.hpp"
#include "satellite/machine/machine_codes.hpp"
#include <charconv>
#include <chrono>
#include <cstdio>
#include <iostream>
using namespace satellite004;
using clk = std::chrono::steady_clock;
int main()
{
    std::ios::sync_with_stdio(false);
    display_takes_std_cout();
    satellite_string line; std::size_t bad = 0;
    satellite_string::from_utf8("hello world, this is one line of text", line, bad);
    double s_ns = 0, n_ns = 0;
    const int N = 300000;
    for (int i = 0; i < N; ++i) {
        const auto until = clk::now() + std::chrono::nanoseconds(2000);
        while (clk::now() < until) {}
        satellite_string copy = line;
        const auto a = clk::now();
        display_string(std::move(copy), true);
        s_ns += std::chrono::duration<double, std::nano>(clk::now() - a).count();
    }
    for (int i = 0; i < N; ++i) {
        const auto until = clk::now() + std::chrono::nanoseconds(2000);
        while (clk::now() < until) {}
        const auto a = clk::now();
        satellite_string digits; char text[24];
        const auto made = std::to_chars(text, text + 24, static_cast<unsigned long long>(i));
        for (const char *at = text; at < made.ptr; ++at) digits.append_code(satellite_string::code_of(static_cast<unsigned char>(*at)));
        display_string(std::move(digits), true);
        n_ns += std::chrono::duration<double, std::nano>(clk::now() - a).count();
    }
    display_drain();
    std::fprintf(stderr, "string %.0f ns, number %.0f ns\n", s_ns / N, n_ns / N);
}
