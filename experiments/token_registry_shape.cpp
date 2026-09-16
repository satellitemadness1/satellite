// Three shapes for bytecode_registry, same content: 100,000 lines x 10 tokens.
#include <bitset>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <fstream>

static const size_t kLines = 100000, kPerLine = 10;

static long rss_kb() {
    std::ifstream f("/proc/self/status"); std::string k;
    while (f >> k) { if (k == "VmRSS:") { long v; f >> v; return v; } }
    return -1;
}
template <class F> static double ms(F f) {
    auto a = std::chrono::steady_clock::now(); f();
    auto b = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(b - a).count();
}

int main() {
    long base = rss_kb();

    std::vector<std::vector<std::bitset<16>>> bits;
    double build_bits = ms([&]{
        bits.resize(kLines);
        for (size_t l = 0; l < kLines; ++l) { bits[l].reserve(kPerLine);
            for (size_t t = 0; t < kPerLine; ++t) bits[l].push_back(std::bitset<16>(256 + (t * 7 + l) % 300)); }
    });
    long after_bits = rss_kb();
    unsigned long long sum1 = 0;
    double scan_bits = ms([&]{ for (auto &line : bits) for (auto &t : line) sum1 += t.to_ulong(); });

    std::vector<std::vector<uint16_t>> words;
    double build_words = ms([&]{
        words.resize(kLines);
        for (size_t l = 0; l < kLines; ++l) { words[l].reserve(kPerLine);
            for (size_t t = 0; t < kPerLine; ++t) words[l].push_back(uint16_t(256 + (t * 7 + l) % 300)); }
    });
    long after_words = rss_kb();
    unsigned long long sum2 = 0;
    double scan_words = ms([&]{ for (auto &line : words) for (auto &t : line) sum2 += t; });

    std::vector<uint16_t> flat; std::vector<uint32_t> line_start;
    double build_flat = ms([&]{
        flat.reserve(kLines * kPerLine); line_start.reserve(kLines + 1);
        for (size_t l = 0; l < kLines; ++l) { line_start.push_back((uint32_t)flat.size());
            for (size_t t = 0; t < kPerLine; ++t) flat.push_back(uint16_t(256 + (t * 7 + l) % 300)); }
        line_start.push_back((uint32_t)flat.size());
    });
    long after_flat = rss_kb();
    unsigned long long sum3 = 0;
    double scan_flat = ms([&]{ for (uint16_t t : flat) sum3 += t; });

    printf("1,000,000 tokens over 100,000 lines. Sums agree: %s\n\n",
           (sum1 == sum2 && sum2 == sum3) ? "yes" : "NO");
    printf("%-38s %10s %10s %10s\n", "shape", "build ms", "scan ms", "RSS MB");
    printf("%-38s %10.1f %10.2f %10.1f\n", "vector<vector<bitset<16>>>", build_bits, scan_bits, (after_bits - base) / 1024.0);
    printf("%-38s %10.1f %10.2f %10.1f\n", "vector<vector<uint16_t>>",   build_words, scan_words, (after_words - after_bits) / 1024.0);
    printf("%-38s %10.1f %10.2f %10.1f\n", "flat vector<uint16_t> + line starts", build_flat, scan_flat, (after_flat - after_words) / 1024.0);
    return 0;
}
