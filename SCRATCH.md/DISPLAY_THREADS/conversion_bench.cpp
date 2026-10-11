// What the interpreter pays per display: to_utf8() today, against a copy or a move
// of the satellite_string into a buffer.
#include "satellite/satellite_variable_string/satellite_string.hpp"
#include <chrono>
#include <cstdio>
#include <vector>
using namespace satellite004;
using clk = std::chrono::steady_clock;

static void race(const char *name, const std::string &utf8, int rounds)
{
    satellite_string s; std::size_t bad = 0;
    satellite_string::from_utf8(utf8, s, bad);
    std::vector<satellite_string> buffer; buffer.reserve(rounds);
    std::vector<std::string> bytes; bytes.reserve(rounds);
    double best_conv = 1e9, best_copy = 1e9, best_move = 1e9;
    for (int trial = 0; trial < 5; trial++) {
        bytes.clear(); buffer.clear();
        auto a = clk::now();
        for (int i = 0; i < rounds; i++) bytes.push_back(s.to_utf8());
        auto b = clk::now();
        for (int i = 0; i < rounds; i++) buffer.push_back(s);
        auto c = clk::now();
        std::vector<satellite_string> temps(rounds, s); buffer.clear();
        auto d = clk::now();
        for (int i = 0; i < rounds; i++) buffer.push_back(std::move(temps[i]));
        auto e = clk::now();
        best_conv = std::min(best_conv, std::chrono::duration<double, std::nano>(b - a).count() / rounds);
        best_copy = std::min(best_copy, std::chrono::duration<double, std::nano>(c - b).count() / rounds);
        best_move = std::min(best_move, std::chrono::duration<double, std::nano>(e - d).count() / rounds);
    }
    std::printf("%-34s to_utf8 %10.1f ns   copy %10.1f ns (x%.1f faster)   move %6.1f ns\n", name, best_conv,
                best_copy, best_conv / best_copy, best_move);
}

int main()
{
    race("\"hello world\" (11 chars)", "hello world", 1000000);
    race("an 80-char ASCII line", std::string(79, 'x') + ".", 1000000);
    race("an 80-char line with accents", std::string(40, 'x') + std::string(20, 'x') + "éééééééééééééééééééé", 1000000);
    race("a 1 MB ASCII string", std::string(1 << 20, 'q'), 200);
    race("a 1 MB string with one emoji", std::string((1 << 20) - 4, 'q') + "😀", 200);
}
